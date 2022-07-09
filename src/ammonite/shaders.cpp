#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <GL/glew.h>

#include "utils/extension.hpp"
#include "utils/cacheManager.hpp"
#include "internal/fileManager.hpp"

namespace ammonite {
  namespace {
    //Vector to store all existing shaders
    std::vector<int> shaderIds(0);
  }

  //Static helper functions
  namespace {
    static void deleteCacheFile(std::string cacheFilePath) {
      //Delete the cache and cacheinfo files
      std::cout << "Clearing '" << cacheFilePath << "'" << std::endl;

      ammonite::utils::files::deleteFile(cacheFilePath);
      ammonite::utils::files::deleteFile(cacheFilePath + "info");
    }

    static bool checkProgram(GLuint programId) {
      //Test whether the program linked
      GLint success = GL_FALSE;
      glGetProgramiv(programId, GL_LINK_STATUS, &success);

      //If the program failed to link, print a log
      if (success == GL_TRUE) {
        return true;
      }

      GLint maxLength = 0;
      glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &maxLength);

      std::vector<GLchar> errorLog(maxLength);
      glGetProgramInfoLog(programId, maxLength, &maxLength, &errorLog[0]);
      //Extra std::endl used to work around Intel driver bug
      std::cerr << &errorLog[0] << std::endl << std::endl;

      return false;
    }

    //Set by updateGLCacheSupport(), when GLEW loads
    bool isBinaryCacheSupported = false;
  }

  //Internally exposed only
  namespace shaders {
    void updateGLCacheSupport() {
      //Get number of supported formats
      GLint numBinaryFormats = 0;
      glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormats);

      //Check support for collecting the program binary
      if (!ammonite::utils::checkExtension("GL_ARB_get_program_binary", "GL_VERSION_4_1")) {
        std::cerr << "Program caching unsupported" << std::endl;
        isBinaryCacheSupported = false;
      } else if (numBinaryFormats < 1) {
        std::cerr << "Program caching unsupported (no supported formats)" << std::endl;
        isBinaryCacheSupported = false;
      }

      isBinaryCacheSupported = true;
    }
  }

  namespace shaders {
    void deleteShader(const GLuint shaderId) {
      //Search for and delete shaderId
      auto shaderPosition = std::find(shaderIds.begin(), shaderIds.end(), shaderId);
      if (shaderPosition != shaderIds.end()) {
        const int intShaderPosition = std::distance(shaderIds.begin(), shaderPosition);
        glDeleteShader(shaderIds[intShaderPosition]);
        shaderIds.erase(shaderPosition);
      }
    }

    //Delete every remaining shader
    void eraseShaders() {
      while (shaderIds.size() != 0) {
        deleteShader(shaderIds[0]);
      }
    }

    GLuint loadShader(const char* shaderPath, const GLenum shaderType, bool* externalSuccess) {
      //Check for compute shader support if needed
      if (shaderType == GL_COMPUTE_SHADER) {
        if (!ammonite::utils::checkExtension("GL_ARB_compute_shader", "GL_VERSION_4_3")) {
          std::cerr << "Compute shaders unsupported" << std::endl;
          *externalSuccess = false;
          return 0;
        }
      }

      //Check for tessellation shader support if needed
      if (shaderType == GL_TESS_CONTROL_SHADER or shaderType == GL_TESS_EVALUATION_SHADER) {
        if (!ammonite::utils::checkExtension("GL_ARB_tessellation_shader", "GL_VERSION_4_0")) {
          std::cerr << "Tessellation shaders unsupported" << std::endl;
          *externalSuccess = false;
          return 0;
        }
      }

      //Create the shader
      GLuint shaderId = glCreateShader(shaderType);

      std::string shaderCode;
      std::ifstream shaderCodeStream(shaderPath, std::ios::in);
      std::stringstream sstr;

      //Read the shader's source code
      if (shaderCodeStream.is_open()) {
        sstr << shaderCodeStream.rdbuf();
        shaderCode = sstr.str();
        shaderCodeStream.close();
      } else {
        std::cerr << "Failed to open '" << shaderPath << "'" << std::endl;
        *externalSuccess = false;
        return 0;
      }

      //Provide a shader source and compile the shader
      const char* shaderCodePointer = shaderCode.c_str();
      glShaderSource(shaderId, 1, &shaderCodePointer, NULL);
      glCompileShader(shaderId);

      //Test whether the shader compiled
      GLint success = GL_FALSE;
      glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

      //If the shader failed to compile, print a log
      if (success == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shaderId, maxLength, &maxLength, &errorLog[0]);
        std::cerr << "\n" << shaderPath << ":" << std::endl;
        std::cerr << &errorLog[0] << std::endl;

        //Clean up and exit
        glDeleteShader(shaderId); //Use glDeleteShader, as the shader never made it to shaderIds
        *externalSuccess = false;
        return 0;
      }

      //Add the shader to the shader id store
      shaderIds.push_back(shaderId);
      return shaderId;
    }

    GLuint createProgram(const GLuint shaderIds[], const int shaderCount, bool* externalSuccess) {
      //Create the program
      GLuint programId = glCreateProgram();

      //Attach all passed shader ids
      for (int i = 0; i < shaderCount; i++) {
        glAttachShader(programId, shaderIds[i]);
      }

      glLinkProgram(programId);

      if (!checkProgram(programId)) {
        *externalSuccess = false;
        return 0;
      }

      //Detach and remove all passed shader ids
      for (int i = 0; i < shaderCount; i++) {
        glDetachShader(programId, shaderIds[i]);
        deleteShader(shaderIds[i]);
      }

      return programId;
    }

    //Create program from set of shader paths
    GLuint createProgram(const char* shaderPaths[], const GLenum shaderTypes[], const int shaderCount, bool* externalSuccess) {
      GLuint shaderIds[shaderCount];
      for (int i = 0; i < shaderCount; i++) {
        shaderIds[i] = loadShader(shaderPaths[i], shaderTypes[i], externalSuccess);
      }

      //Create the program like normal, as a valid cache wasn't found
      GLuint programId = createProgram(shaderIds, shaderCount, externalSuccess);

      //Cleanup on failure
      if (!*externalSuccess) {
        ammonite::shaders::eraseShaders();
        return 0;
      }

      return programId;
    }
  }

  //Cached versions of program creation
  namespace shaders {
    GLuint createProgram(const char* shaderPaths[], const GLenum shaderTypes[], const int shaderCount, bool* externalSuccess, const char* programName) {
      //Used later as the return value
      GLuint programId;

      //Check for OpenGL and engine cache support
      const bool cacheSupported = isBinaryCacheSupported and ammonite::utils::cache::getCacheEnabled();

      if (cacheSupported) {
        bool cacheValid = false;
        std::string cacheFilePath = ammonite::utils::cache::requestCachedData(shaderPaths, shaderCount, programName, &cacheValid);
        std::string cacheFileInfoPath = cacheFilePath + "info";

        //Attempt to get the shader format and size
        GLenum cachedBinaryFormat = 0;
        GLsizei cachedBinaryLength = 0;

        if (cacheValid) {
          std::string line;
          std::ifstream cacheInfoFile(cacheFileInfoPath);
          if (cacheInfoFile.is_open()) {
            try {
              //Skip input files
              for (int i = 0; i < shaderCount; i++) {
                getline(cacheInfoFile, line);
              }

              //Get the binary format
              getline(cacheInfoFile, line);
              cachedBinaryFormat = std::stoi(line);

              //Get the length of the binary
              getline(cacheInfoFile, line);
              cachedBinaryLength = std::stoi(line);
            } catch (const std::out_of_range&) {
              cacheValid = false;
            } catch (const std::invalid_argument&) {
              cacheValid = false;
            }

            cacheInfoFile.close();
          }
        }

        //If cache is still valid, attempt to use it
        if (cacheValid) {
          //Read the cached data from file
          char cachedBinaryData[cachedBinaryLength];
          std::ifstream input(cacheFilePath, std::ios::binary);
          input.read(&cachedBinaryData[0], cachedBinaryLength);

          //Load the cached binary data
          programId = glCreateProgram();
          glProgramBinary(programId, cachedBinaryFormat, cachedBinaryData, cachedBinaryLength);

          //Return the program ID, unless the cache was faulty, then delete and carry on
          if (checkProgram(programId)) {
            return programId;
          } else {
            std::cerr << "Failed to process '" << cacheFilePath << "'" << std::endl;
            deleteCacheFile(cacheFilePath);
          }
        } else {
          //Shader source doesn't match cache, delete the old cache
          if (cacheFilePath != "") {
            deleteCacheFile(cacheFilePath);
          }
        }
      }

      //Fall back to non-cached program creation
      programId = createProgram(shaderPaths, shaderTypes, shaderCount, externalSuccess);

      //If caching is enabled, cache the binary
      if (cacheSupported) {
        std::string cacheFilePath = ammonite::utils::cache::requestNewCache(programName);
        std::string cacheFileInfoPath = cacheFilePath + "info";

        std::cout << "Caching '" << cacheFilePath << "'" << std::endl;
        int binaryLength;
        GLenum binaryFormat;

        //Get binary length and data of linked program
        glGetProgramiv(programId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
        char binaryData[binaryLength];
        glGetProgramBinary(programId, binaryLength, NULL, &binaryFormat, &binaryData);

        if (binaryLength == 0) {
          std::cerr << "Failed to cache '" << cacheFilePath << "'" << std::endl;
          return programId;
        }

        //Write the binary to cache directory
        std::ofstream binarySave(cacheFilePath, std::ios::binary);
        if (binarySave.is_open()) {
          binarySave.write(&binaryData[0], binaryLength);
        } else {
          std::cerr << "Failed to cache '" << cacheFilePath << "'" << std::endl;
          deleteCacheFile(cacheFilePath);
          return programId;
        }

        //Write the cache info to cache directory as programName.cacheinfo
        std::ofstream binaryInfo(cacheFileInfoPath);
        if (binaryInfo.is_open()) {
          for (int i = 0; i < shaderCount; i++) {
            long long int filesize, modificationTime;
            ammonite::utils::files::getFileMetadata(shaderPaths[i], &filesize, &modificationTime);

            binaryInfo << "input;" << shaderPaths[i] << ";" << filesize << ";" << modificationTime << "\n";
          }

          binaryInfo << binaryFormat << "\n";
          binaryInfo << binaryLength << "\n";

          binaryInfo.close();
        } else {
          std::cerr << "Failed to cache '" << cacheFileInfoPath << "'" << std::endl;
          deleteCacheFile(programName);
          return programId;
        }
      }

      return programId;
    }

    GLuint loadDirectory(const char* directoryPath, bool* externalSuccess) {
      const std::filesystem::path shaderDir{directoryPath};
      const auto it = std::filesystem::directory_iterator{shaderDir};

      //Find all shaders
      std::vector<std::string> shaders(0);
      std::vector<GLenum> types(0);
      for (auto const& fileName : it) {
        std::filesystem::path filePath{fileName};
        std::string extension = filePath.extension();

        if (extension == ".vs" or extension == ".vert") {
          shaders.push_back(std::string(filePath));
          types.push_back(GL_VERTEX_SHADER);
        } else if (extension == ".fs" or extension == ".frag") {
          shaders.push_back(std::string(filePath));
          types.push_back(GL_FRAGMENT_SHADER);
        } else if (extension == ".gs" or extension == ".geo") {
          shaders.push_back(std::string(filePath));
          types.push_back(GL_GEOMETRY_SHADER);
        }
      }

      //Repack shaders
      const char* shaderPaths[shaders.size()];
      GLenum shaderTypes[types.size()];
      const int shaderCount = sizeof(shaderPaths) / sizeof(shaderPaths[0]);

      for (unsigned int i = 0; i < shaders.size(); i++) {
        shaderPaths[i] = shaders[i].c_str();
        shaderTypes[i] = types[i];
      }

      //Get the final component of the path
      const char* directoryName = std::string(shaderDir.parent_path().filename()).c_str();

      //Create the program and return the ID
      GLuint programId = createProgram(shaderPaths, shaderTypes, shaderCount, externalSuccess, directoryName);
      return programId;
    }
  }
}

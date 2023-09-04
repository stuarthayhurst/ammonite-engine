#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <GL/glew.h>

#include "internal/internalExtension.hpp"
#include "../utils/internal/internalFileManager.hpp"
#include "../utils/internal/internalCacheManager.hpp"

#include "../utils/logging.hpp"
#include "../utils/cacheManager.hpp"

namespace ammonite {
  //Static helper functions
  namespace {
    static void deleteCacheFile(std::string cacheFilePath) {
      //Delete the cache and cacheinfo files
      ammonite::utils::status << "Clearing '" << cacheFilePath << "'" << std::endl;

      ammonite::utils::internal::deleteFile(cacheFilePath);
      ammonite::utils::internal::deleteFile(cacheFilePath + "info");
    }

    static void cacheProgram(const GLuint programId, const char* shaderPaths[], const int shaderCount) {
      std::string cacheFilePath =
        ammonite::utils::cache::internal::requestNewCachePath(shaderPaths, shaderCount);
      std::string cacheFileInfoPath = cacheFilePath + "info";

      ammonite::utils::status << "Caching '" << cacheFilePath << "'" << std::endl;
      int binaryLength;
      GLenum binaryFormat;

      //Get binary length and data of linked program
      glGetProgramiv(programId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
      char binaryData[binaryLength];
      glGetProgramBinary(programId, binaryLength, NULL, &binaryFormat, &binaryData);

      if (binaryLength == 0) {
        ammonite::utils::warning << "Failed to cache '" << cacheFilePath << "'" << std::endl;
        return;
      }

      //Write the binary to cache directory
      std::ofstream binarySave(cacheFilePath, std::ios::binary);
      if (binarySave.is_open()) {
        binarySave.write(&binaryData[0], binaryLength);
      } else {
        ammonite::utils::warning << "Failed to cache '" << cacheFilePath << "'" << std::endl;
        deleteCacheFile(cacheFilePath);
        return;
      }

      //Write the cache info to cache directory
      std::ofstream binaryInfo(cacheFileInfoPath);
      if (binaryInfo.is_open()) {
        for (int i = 0; i < shaderCount; i++) {
          long long int filesize = 0, modificationTime = 0;
          ammonite::utils::internal::getFileMetadata(shaderPaths[i], &filesize, &modificationTime);

          binaryInfo << "input;" << shaderPaths[i] << ";" << filesize << ";" << modificationTime << "\n";
        }

        binaryInfo << binaryFormat << "\n";
        binaryInfo << binaryLength << "\n";

        binaryInfo.close();
      } else {
        ammonite::utils::warning << "Failed to cache '" << cacheFileInfoPath << "'" << std::endl;
        deleteCacheFile(cacheFilePath);
      }
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
      ammonite::utils::warning << &errorLog[0] << std::endl << std::endl;

      return false;
    }

    static GLenum attemptIdentifyShaderType(const char* shaderPath) {
      std::string shaderPathString = std::string(shaderPath);

      std::map<std::string, GLenum> shaderMatches = {
        {"vert", GL_VERTEX_SHADER},
        {"frag", GL_FRAGMENT_SHADER},
        {"geom", GL_GEOMETRY_SHADER},
        {"tessc", GL_TESS_CONTROL_SHADER}, {"control", GL_TESS_CONTROL_SHADER},
        {"tesse", GL_TESS_EVALUATION_SHADER}, {"eval", GL_TESS_EVALUATION_SHADER},
        {"compute", GL_COMPUTE_SHADER}
      };

      std::string lowerShaderPath;
      for (unsigned int i = 0; i < shaderPathString.size(); i++) {
        lowerShaderPath += std::tolower(shaderPathString[i]);
      }

      //Try and match the filename to a supported shader
      for (auto it = shaderMatches.begin(); it != shaderMatches.end(); it++) {
        if (lowerShaderPath.contains(it->first)) {
          return it->second;
        }
      }

      return GL_FALSE;
    }

    //Set by updateCacheSupport(), when GLEW loads
    bool isBinaryCacheSupported = false;
  }

  //Shader compilation functions, local to this file
  namespace shaders {
    int loadShader(const char* shaderPath, const GLenum shaderType, bool* externalSuccess) {
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
        ammonite::utils::warning << "Failed to open '" << shaderPath << "'" << std::endl;
        *externalSuccess = false;
        return -1;
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
        ammonite::utils::warning << "\n" << shaderPath << ":" << std::endl;
        ammonite::utils::warning << &errorLog[0] << std::endl;

        //Clean up and exit
        glDeleteShader(shaderId);
        *externalSuccess = false;
        return -1;
      }

      return shaderId;
    }

    int createProgramObject(GLuint shaderIds[], const int shaderCount, bool* externalSuccess) {
      //Create the program
      GLuint programId = glCreateProgram();

      //Attach all passed shader ids
      for (int i = 0; i < shaderCount; i++) {
        glAttachShader(programId, shaderIds[i]);
      }

      glLinkProgram(programId);

      if (!checkProgram(programId)) {
        *externalSuccess = false;
        return -1;
      }

      //Detach and remove all passed shader ids
      for (int i = 0; i < shaderCount; i++) {
        glDetachShader(programId, shaderIds[i]);
        glDeleteShader(shaderIds[i]);
      }

      return programId;
    }

    //Attempt to find cached program or hand off to loadShader and createProgramObject
    int createProgramCached(const char* shaderPaths[], const GLenum shaderTypes[],
                      const int shaderCount, bool* externalSuccess) {
      //Used later as the return value
      GLuint programId;

      //Check for OpenGL and engine cache support
      const bool isCacheSupported = isBinaryCacheSupported and
                                    ammonite::utils::cache::getCacheEnabled();

      if (isCacheSupported) {
        bool cacheValid = false;
        std::string cacheFilePath =
          ammonite::utils::cache::internal::requestCachedDataPath(shaderPaths,
                                                                  shaderCount,
                                                                  &cacheValid);
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
          glProgramBinary(programId, cachedBinaryFormat,
                          cachedBinaryData, cachedBinaryLength);

          //Return the program ID, unless the cache was faulty, then delete and carry on
          if (checkProgram(programId)) {
            return programId;
          } else {
            ammonite::utils::warning << "Failed to process '" << cacheFilePath << "'" << std::endl;
            deleteCacheFile(cacheFilePath);
          }
        } else {
          //Shader source doesn't match cache, delete the old cache
          if (cacheFilePath != "") {
            deleteCacheFile(cacheFilePath);
          }
        }
      }

      //Since cache wasn't available, generate fresh shaders
      GLuint shaderIds[shaderCount];
      for (int i = 0; i < shaderCount; i++) {
        shaderIds[i] = loadShader(shaderPaths[i], shaderTypes[i], externalSuccess);
      }

      //Create the program like normal, as a valid cache wasn't found
      bool hasCreatedProgram = true;
      programId = createProgramObject(shaderIds, shaderCount, &hasCreatedProgram);

      //Cleanup on failure
      if (!hasCreatedProgram) {
        *externalSuccess = false;
        for (int i = 0; i < shaderCount; i++) {
          glDeleteShader(shaderIds[shaderCount]);
        }
        return -1;
      }

      //Cache the binary if enabled
      if (isCacheSupported) {
        cacheProgram(programId, shaderPaths, shaderCount);
      }

      return programId;
    }
  }

  //Internally exposed functions
  namespace shaders {
    namespace internal {
      void updateCacheSupport() {
        //Get number of supported formats
        GLint numBinaryFormats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormats);

        //Check support for collecting the program binary
        if (!graphics::internal::checkExtension("GL_ARB_get_program_binary", "GL_VERSION_4_1")) {
          ammonite::utils::warning << "Program caching unsupported" << std::endl;
          isBinaryCacheSupported = false;
        } else if (numBinaryFormats < 1) {
          ammonite::utils::warning << "Program caching unsupported (no supported formats)" << std::endl;
          isBinaryCacheSupported = false;
        }

        isBinaryCacheSupported = true;
      }

      //Find shader types and hand off to createProgramCached(paths, types)
      int createProgram(const char* inputShaderPaths[],
                        const int inputShaderCount, bool* externalSuccess) {
        //Convert file extensions to shader types
        std::map<std::string, GLenum> shaderExtensions = {
          {".vert", GL_VERTEX_SHADER}, {".vs", GL_VERTEX_SHADER},
          {".frag", GL_FRAGMENT_SHADER}, {".fs", GL_FRAGMENT_SHADER},
          {".geom", GL_GEOMETRY_SHADER}, {".gs", GL_GEOMETRY_SHADER},
          {".tessc", GL_TESS_CONTROL_SHADER}, {".tsc", GL_TESS_CONTROL_SHADER},
          {".tesse", GL_TESS_EVALUATION_SHADER}, {".tes", GL_TESS_EVALUATION_SHADER},
          {".comp", GL_COMPUTE_SHADER}, {".cs", GL_COMPUTE_SHADER},
          {".glsl", GL_FALSE} //Detect generic shaders, attempt to identify further
        };

        //Find all shaders
        std::vector<std::string> shaders(0);
        std::vector<GLenum> types(0);
        for (int i = 0; i < inputShaderCount; i++) {
          std::filesystem::path filePath{inputShaderPaths[i]};
          std::string extension = filePath.extension();

          if (shaderExtensions.contains(extension)) {
            GLenum shaderType = shaderExtensions[extension];

            //Shader can't be identified through extension, use filename
            if (shaderType == GL_FALSE) {
              GLenum newType = attemptIdentifyShaderType(inputShaderPaths[i]);
              if (newType == GL_FALSE) {
                ammonite::utils::warning << "Couldn't identify type of shader '"
                        << inputShaderPaths[i] << "'" << std::endl;
                continue;
              }

              //Found a type, so use it
              shaderType = newType;
            }

            //Check for compute shader support if needed
            if (shaderType == GL_COMPUTE_SHADER) {
              if (!graphics::internal::checkExtension("GL_ARB_compute_shader",
                                                      "GL_VERSION_4_3")) {
                ammonite::utils::warning << "Compute shaders unsupported" << std::endl;
                continue;
              }
            }

            //Check for tessellation shader support if needed
            if (shaderType == GL_TESS_CONTROL_SHADER or shaderType == GL_TESS_EVALUATION_SHADER) {
              if (!graphics::internal::checkExtension("GL_ARB_tessellation_shader",
                                                      "GL_VERSION_4_0")) {
                ammonite::utils::warning << "Tessellation shaders unsupported" << std::endl;
                continue;
              }
            }

            shaders.push_back(std::string(filePath));
            types.push_back(shaderType);
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

        //Create the program and return the ID
        return createProgramCached(shaderPaths, shaderTypes, shaderCount, externalSuccess);
      }

      //Load all shaders in a directory and hand off to createProgram(paths)
      int loadDirectory(const char* directoryPath, bool* externalSuccess) {
        //Create filesystem directory iterator
        std::filesystem::directory_iterator it;
        try {
          const std::filesystem::path shaderDir{directoryPath};
          it = std::filesystem::directory_iterator{shaderDir};
        } catch (const std::filesystem::filesystem_error&) {
          *externalSuccess = false;
          ammonite::utils::warning << "Failed to load '" << directoryPath << "'" << std::endl;
          return -1;
        }

        //Find files to send to next stage
        std::vector<std::string> shaders(0);
        for (auto const& fileName : it) {
          std::filesystem::path filePath{fileName};
          shaders.push_back(std::string(filePath));
        }

        //Repack shaders
        const char* shaderPaths[shaders.size()];
        const int shaderCount = sizeof(shaderPaths) / sizeof(shaderPaths[0]);

        for (unsigned int i = 0; i < shaders.size(); i++) {
          shaderPaths[i] = shaders[i].c_str();
        }

        //Create the program and return the ID
        return createProgram(shaderPaths, shaderCount, externalSuccess);
      }
    }
  }
}

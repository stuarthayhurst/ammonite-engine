#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <GL/glew.h>

#include "internal/internalExtensions.hpp"
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
    }

    static void cacheProgram(const GLuint programId,
                             const char* shaderPaths[], const int shaderCount) {
      std::string cacheFilePath =
        ammonite::utils::cache::internal::requestNewCachePath(shaderPaths, shaderCount);

      ammonite::utils::status << "Caching '" << cacheFilePath << "'" << std::endl;
      int binaryLength;
      GLenum binaryFormat;

      //Get binary length and data of linked program
      glGetProgramiv(programId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
      if (binaryLength == 0) {
        ammonite::utils::warning << "Failed to cache '" << cacheFilePath << "'" << std::endl;
        return;
      }

      char* binaryData = new char[binaryLength];
      glGetProgramBinary(programId, binaryLength, nullptr, &binaryFormat, binaryData);

      //Write the cache info and data to cache file
      std::ofstream cacheFile(cacheFilePath, std::ios::binary);
      if (cacheFile.is_open()) {
        for (int i = 0; i < shaderCount; i++) {
          long long int filesize = 0, modificationTime = 0;
          ammonite::utils::internal::getFileMetadata(shaderPaths[i], &filesize, &modificationTime);

          cacheFile << "input;" << shaderPaths[i] << ";" << filesize << ";" \
                    << modificationTime << "\n";
        }

        cacheFile << binaryFormat << "\n";
        cacheFile << binaryLength << "\n";

        //Write the data
        cacheFile.write(binaryData, binaryLength);

        cacheFile.close();
      } else {
        ammonite::utils::warning << "Failed to cache '" << cacheFilePath << "'" << std::endl;
        deleteCacheFile(cacheFilePath);
      }

      delete [] binaryData;
    }

    static bool checkProgram(GLuint programId) {
      //Test whether the program linked
      GLint success = GL_FALSE;
      glGetProgramiv(programId, GL_LINK_STATUS, &success);

      //If the program linked successfully, return
      if (success == GL_TRUE) {
        return true;
      }

      //Otherwise, print a log
      GLint maxLength = 0;
      glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &maxLength);

      //Not strictly required, but add an extra byte to be safe
      GLchar* errorLogBuffer = new GLchar[++maxLength];
      glGetProgramInfoLog(programId, maxLength, &maxLength, errorLogBuffer);
      errorLogBuffer[maxLength] = '\0';
      ammonite::utils::warning << "Failed to link shader program (ID " << programId \
                               << "):\n" << (char*)errorLogBuffer << std::endl;

      delete [] errorLogBuffer;
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

  //Shader compilation and cache functions, local to this file
  namespace shaders {
    //Take shader source code, compile it and load it
    static int loadShader(const char* shaderPath, const GLenum shaderType, bool* externalSuccess) {
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
      glShaderSource(shaderId, 1, &shaderCodePointer, nullptr);
      glCompileShader(shaderId);

      //Test whether the shader compiled
      GLint success = GL_FALSE;
      glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

      //If the shader failed to compile, print a log
      if (success == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);

        //Not strictly required, but add an extra byte to be safe
        GLchar* errorLogBuffer = new GLchar[++maxLength];
        glGetShaderInfoLog(shaderId, maxLength, &maxLength, errorLogBuffer);
        errorLogBuffer[maxLength] = '\0';
        ammonite::utils::warning << shaderPath << ":\n" \
                                 << (char*)errorLogBuffer << std::endl;

        //Clean up and exit
        delete [] errorLogBuffer;
        glDeleteShader(shaderId);
        *externalSuccess = false;
        return -1;
      }

      return shaderId;
    }

    //Take multiple shader files, hand off to loadShader and create a program
    static int createProgramObject(GLuint shaderIds[],
                                   const int shaderCount, bool* externalSuccess) {
      //Create the program
      GLuint programId = glCreateProgram();

      //Attach all passed shader ids
      for (int i = 0; i < shaderCount; i++) {
        glAttachShader(programId, shaderIds[i]);
      }

      glLinkProgram(programId);

      //Detach and remove all passed shader ids
      for (int i = 0; i < shaderCount; i++) {
        glDetachShader(programId, shaderIds[i]);
        glDeleteShader(shaderIds[i]);
      }

      if (!checkProgram(programId)) {
        glDeleteProgram(programId);
        *externalSuccess = false;
        return -1;
      }

      return programId;
    }

    //Attempt to find cached program or hand off to loadShader and createProgramObject
    static int createProgramCached(const char* shaderPaths[], const GLenum shaderTypes[],
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

        //Attempt to get the shader format and size
        GLenum cachedBinaryFormat = 0;
        GLsizei cachedBinaryLength = 0;

        std::ifstream cacheFile(cacheFilePath, std::ios::binary);
        if (cacheValid) {
          std::string line;
          if (cacheFile.is_open()) {
            try {
              //Skip input files, as they're handled by the cache manager
              for (int i = 0; i < shaderCount; i++) {
                getline(cacheFile, line);
              }

              //Get the binary format
              getline(cacheFile, line);
              cachedBinaryFormat = std::stoi(line);

              //Get the length of the binary
              getline(cacheFile, line);
              cachedBinaryLength = std::stoi(line);
            } catch (const std::out_of_range&) {
              cacheValid = false;
            } catch (const std::invalid_argument&) {
              cacheValid = false;
            }
          }
        }

        //If cache is still valid, attempt to use it
        if (cacheValid) {
          //Read the cached data from file
          char* cachedBinaryData = new char[cachedBinaryLength];
          cacheFile.read(cachedBinaryData, cachedBinaryLength);

          //Load the cached binary data
          programId = glCreateProgram();
          glProgramBinary(programId, cachedBinaryFormat,
                          cachedBinaryData, cachedBinaryLength);
          delete [] cachedBinaryData;

          //Return the program ID, unless the cache was faulty, then delete and carry on
          if (checkProgram(programId)) {
            return programId;
          } else {
            ammonite::utils::warning << "Failed to process '" << cacheFilePath << "'" << std::endl;
            glDeleteProgram(programId);
            deleteCacheFile(cacheFilePath);
          }
        } else {
          //Shader source doesn't match cache, delete the old cache
          if (cacheFilePath != "") {
            deleteCacheFile(cacheFilePath);
          }
        }
        cacheFile.close();
      }

      //Since cache wasn't available, generate fresh shaders
      GLuint* shaderIds = new GLuint[shaderCount];
      bool hasCreatedShaders = true;
      for (int i = 0; i < shaderCount; i++) {
        shaderIds[i] = loadShader(shaderPaths[i], shaderTypes[i], externalSuccess);
        if (shaderIds[i] == unsigned(-1)) {
          hasCreatedShaders = false;
          break;
        }
      }

      //Create the program like normal, as a valid cache wasn't found
      bool hasCreatedProgram = false;
      if (hasCreatedShaders) {
        hasCreatedProgram = true;
        programId = createProgramObject(shaderIds, shaderCount, &hasCreatedProgram);
      }

      //Cleanup on failure
      if (!hasCreatedProgram || !hasCreatedShaders) {
        *externalSuccess = false;
        for (int i = 0; i < shaderCount; i++) {
          if (shaderIds[shaderCount] != unsigned(-1)) {
            glDeleteShader(shaderIds[shaderCount]);
          }
        }
        delete [] shaderIds;
        return -1;
      }
      delete [] shaderIds;

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
          ammonite::utils::warning << "Program caching unsupported (no supported formats)" \
                                   << std::endl;
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
        const int shaderCount = shaders.size();
        const char** shaderPaths = new const char*[shaderCount];
        GLenum* shaderTypes = new GLenum[shaderCount];

        for (unsigned int i = 0; i < shaders.size(); i++) {
          shaderPaths[i] = shaders[i].c_str();
          shaderTypes[i] = types[i];
        }

        //Create the program and return the ID
        int programId = createProgramCached(shaderPaths, shaderTypes,
                                            shaderCount, externalSuccess);
        delete [] shaderTypes;
        delete [] shaderPaths;
        return programId;
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
        const int shaderCount = shaders.size();
        const char** shaderPaths = new const char*[shaderCount];

        for (unsigned int i = 0; i < shaders.size(); i++) {
          shaderPaths[i] = shaders[i].c_str();
        }

        //Create the program and return the ID
        int programId = createProgram(shaderPaths, shaderCount, externalSuccess);
        delete [] shaderPaths;
        return programId;
      }
    }
  }
}

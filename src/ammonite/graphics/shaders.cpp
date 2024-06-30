#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <GL/glew.h>

#include "../core/fileManager.hpp"
#include "../utils/logging.hpp"
#include "internal/internalExtensions.hpp"
#include "internal/internalShaderValidator.hpp"

namespace ammonite {
  //Static helper functions
  namespace {
    static void deleteCacheFile(std::string cacheFilePath) {
      //Delete the cache file
      ammonite::utils::status << "Clearing '" << cacheFilePath << "'" << std::endl;
      ammonite::files::internal::deleteFile(cacheFilePath);
    }

    static void cacheProgram(const GLuint programId, std::string* shaderPaths, int shaderCount) {
      std::string cacheFilePath = ammonite::files::internal::getCachedFilePath(shaderPaths,
                                                                               shaderCount);
      ammonite::utils::status << "Caching '" << cacheFilePath << "'" << std::endl;

      //Get binary length of linked program
      int binaryLength;
      glGetProgramiv(programId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
      if (binaryLength == 0) {
        ammonite::utils::warning << "Failed to cache '" << cacheFilePath << "'" << std::endl;
        return;
      }

      //Get binary format and binary data
      GLenum binaryFormat;
      int actualBytes = 0;
      char* binaryData = new char[binaryLength];
      glGetProgramBinary(programId, binaryLength, &actualBytes, &binaryFormat, binaryData);
      if (actualBytes != binaryLength) {
        ammonite::utils::warning << "Program length doesn't match expected length (ID " \
                                 << programId << ")" << std::endl;
        delete [] binaryData;
        return;
      }

      //Generate cache info to write
      std::string extraData;
      for (int i = 0; i < shaderCount; i++) {
        long long int filesize = 0, modificationTime = 0;
        ammonite::files::internal::getFileMetadata(shaderPaths[i], &filesize, &modificationTime);

        extraData.append("input;" + shaderPaths[i]);
        extraData.append(";" + std::to_string(filesize));
        extraData.append(";" + std::to_string(modificationTime) + "\n");
      }

      extraData.append(std::to_string(binaryFormat) + "\n");
      extraData.append(std::to_string(binaryLength) + "\n");

      int extraLength = extraData.length();
      char* fileData = new char[extraLength + binaryLength];

      //Write the cache info and binary data to the buffer
      extraData.copy(fileData, extraLength, 0);
      std::memcpy(fileData + extraLength, binaryData, binaryLength);

      //Write the cache info and data to the cache file
      if (!ammonite::files::internal::writeFile(cacheFilePath, (unsigned char*)fileData,
                                                extraLength + binaryLength)) {
        ammonite::utils::warning << "Failed to cache '" << cacheFilePath << "'" << std::endl;
        deleteCacheFile(cacheFilePath);
      }

      delete [] fileData;
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

    static GLenum attemptIdentifyShaderType(std::string shaderPath) {
      std::map<std::string, GLenum> shaderMatches = {
        {"vert", GL_VERTEX_SHADER},
        {"frag", GL_FRAGMENT_SHADER},
        {"geom", GL_GEOMETRY_SHADER},
        {"tessc", GL_TESS_CONTROL_SHADER}, {"control", GL_TESS_CONTROL_SHADER},
        {"tesse", GL_TESS_EVALUATION_SHADER}, {"eval", GL_TESS_EVALUATION_SHADER},
        {"compute", GL_COMPUTE_SHADER}
      };

      std::string lowerShaderPath;
      for (unsigned int i = 0; i < shaderPath.size(); i++) {
        lowerShaderPath += std::tolower(shaderPath[i]);
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
  namespace {
    //Take shader source code, compile it and load it
    static int loadShader(std::string shaderPath, const GLenum shaderType, bool* externalSuccess) {
      //Create the shader
      GLuint shaderId = glCreateShader(shaderType);

      //Read the shader's source code
      std::size_t shaderCodeSize;
      char* shaderCodePtr = (char*)ammonite::files::internal::loadFile(shaderPath,
                                                                       &shaderCodeSize);
      if (shaderCodePtr == nullptr) {
        ammonite::utils::warning << "Failed to open '" << shaderPath << "'" << std::endl;
        *externalSuccess = false;
        return -1;
      }

      glShaderSource(shaderId, 1, &shaderCodePtr, (int*)&shaderCodeSize);
      delete [] shaderCodePtr;
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
    static int createProgramObject(GLuint shaderIds[], const int shaderCount,
                                   bool* externalSuccess) {
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

    //Create a program from shader source with loadShader() and createProgramObject()
    static int createProgramUncached(std::string* shaderPaths, GLenum* shaderTypes,
                                     int shaderCount, bool* externalSuccess) {
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

      //Create the program from the shaders
      GLuint programId;
      bool hasCreatedProgram = false;
      if (hasCreatedShaders) {
        hasCreatedProgram = true;
        programId = createProgramObject(shaderIds, shaderCount, &hasCreatedProgram);
      }

      //Clean up
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

      return programId;
    }

    //Attempt to find cached program or hand off to createProgramUncached()
    static int createProgramCached(std::string* shaderPaths, GLenum* shaderTypes,
                                   int shaderCount, bool* externalSuccess) {
      //Check for OpenGL and engine cache support
      bool isCacheSupported = ammonite::files::internal::getCacheEnabled();
      isCacheSupported = isCacheSupported && isBinaryCacheSupported;

      //Try and fetch the cache, then try and load it into a program
      int programId;
      if (isCacheSupported) {
        std::size_t cacheDataSize;
        AmmoniteEnum cacheState;

        //Attempt to load the cached program
        ammonite::shaders::internal::CacheInfo cacheInfo;
        cacheInfo.fileCount = shaderCount;
        cacheInfo.filePaths = shaderPaths;
        std::string cacheFilePath = ammonite::files::internal::getCachedFilePath(shaderPaths,
                                                                                 shaderCount);
        unsigned char* cacheData = ammonite::files::internal::getCachedFile(cacheFilePath,
          ammonite::shaders::internal::validateCache, &cacheDataSize, &cacheState, &cacheInfo);
        unsigned char* dataStart = cacheData + cacheDataSize - cacheInfo.binaryLength;

        if (cacheState == AMMONITE_CACHE_HIT) {
          //Load the cached binary data into a program
          programId = glCreateProgram();
          glProgramBinary(programId, cacheInfo.binaryFormat, dataStart, cacheInfo.binaryLength);
          delete [] cacheData;

          //Return the program ID, unless the cache was faulty, then delete and carry on
          if (checkProgram(programId)) {
            return programId;
          } else {
            ammonite::utils::warning << "Failed to process '" << cacheFilePath \
                                     << "'" << std::endl;
            glDeleteProgram(programId);
            deleteCacheFile(cacheFilePath);
          }
        }
      }

      //Cache wasn't useable, compile a fresh program
      programId = createProgramUncached(shaderPaths, shaderTypes, shaderCount, externalSuccess);

      //Cache the binary if enabled
      if (isCacheSupported && programId != -1) {
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
      int createProgram(std::string* inputShaderPaths, int inputShaderCount,
                        bool* externalSuccess) {
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
        std::vector<std::string> shaderPaths(0);
        std::vector<GLenum> shaderTypes(0);
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

            shaderPaths.push_back(std::string(filePath));
            shaderTypes.push_back(shaderType);
          }
        }

        //Create the program and return the ID
        int programId = createProgramCached(&shaderPaths[0], &shaderTypes[0],
                                            shaderPaths.size(), externalSuccess);
        return programId;
      }

      //Load all shaders in a directory and hand off to createProgram()
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
        std::vector<std::string> shaderPaths(0);
        for (auto const& fileName : it) {
          std::filesystem::path filePath{fileName};
          shaderPaths.push_back(std::string(filePath));
        }

        //Create the program and return the ID
        int programId = createProgram(&shaderPaths[0], shaderPaths.size(), externalSuccess);
        return programId;
      }
    }
  }
}

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

extern "C" {
  #include <GL/glew.h>
}

#include "shaders.hpp"

#include "extensions.hpp"
#include "../enums.hpp"
#include "../utils/debug.hpp"
#include "../utils/files.hpp"
#include "../utils/logging.hpp"
#include "../utils/thread.hpp"

namespace ammonite {
  namespace {
    //Set by updateCacheSupport(), when renderer is set up
    bool isBinaryCacheSupported = false;

    //Identify shader types by extensions / contained strings
    std::map<std::string, GLenum> shaderMatches = {
      {".vert", GL_VERTEX_SHADER}, {".vs", GL_VERTEX_SHADER}, {"vert", GL_VERTEX_SHADER},
      {".frag", GL_FRAGMENT_SHADER}, {".fs", GL_FRAGMENT_SHADER}, {"frag", GL_FRAGMENT_SHADER},
      {".geom", GL_GEOMETRY_SHADER}, {".gs", GL_GEOMETRY_SHADER}, {"geom", GL_GEOMETRY_SHADER},
      {".tessc", GL_TESS_CONTROL_SHADER}, {".tsc", GL_TESS_CONTROL_SHADER},
        {"tessc", GL_TESS_CONTROL_SHADER}, {"control", GL_TESS_CONTROL_SHADER},
      {".tesse", GL_TESS_EVALUATION_SHADER}, {".tes", GL_TESS_EVALUATION_SHADER},
        {"tesse", GL_TESS_EVALUATION_SHADER}, {"eval", GL_TESS_EVALUATION_SHADER},
      {".comp", GL_COMPUTE_SHADER}, {".cs", GL_COMPUTE_SHADER}, {"compute", GL_COMPUTE_SHADER},
      {".glsl", GL_FALSE} //Don't ignore generic shaders
    };

    //Data required by cache worker
    struct CacheWorkerData {
      unsigned int shaderCount;
      std::string* shaderPaths;
      std::string cacheFilePath;
      int binaryLength;
      GLenum binaryFormat;
      char* binaryData;
    };

    //Thread pool work to cache a program
    void doCacheWork(void* userPtr) {
      CacheWorkerData* data = (CacheWorkerData*)userPtr;

      //Prepare user data required to load the cache again
      std::string userData = std::to_string(data->binaryFormat) + "\n";
      std::size_t userDataSize = userData.length();
      unsigned char* userBuffer = new unsigned char[userDataSize];
      userData.copy((char*)userBuffer, userDataSize, 0);

      //Write the cache file, failure messages are also handled by it
      ammonite::utils::files::writeCacheFile(data->cacheFilePath, data->shaderPaths,
        data->shaderCount, (unsigned char*)data->binaryData, data->binaryLength, userBuffer,
        userDataSize);

      delete [] userBuffer;
      delete [] data->binaryData;
      delete [] data->shaderPaths;
      delete data;
    }

    void cacheProgram(const GLuint programId, std::string* shaderPaths,
                      unsigned int shaderCount, std::string* cacheFilePath) {
      CacheWorkerData* data = new CacheWorkerData;
      data->shaderCount = shaderCount;

      data->cacheFilePath = *cacheFilePath;
      ammonite::utils::status << "Caching '" << data->cacheFilePath << "'" << std::endl;

      //Get binary length of linked program
      glGetProgramiv(programId, GL_PROGRAM_BINARY_LENGTH, &data->binaryLength);
      if (data->binaryLength == 0) {
        ammonite::utils::warning << "Failed to cache '" << data->cacheFilePath << "'" << std::endl;
        delete data;
        return;
      }

      //Get binary format and binary data
      int actualBytes = 0;
      data->binaryData = new char[data->binaryLength];
      glGetProgramBinary(programId, data->binaryLength, &actualBytes, &data->binaryFormat,
                         data->binaryData);
      if (actualBytes != data->binaryLength) {
        ammonite::utils::warning << "Program length doesn't match expected length (ID " \
                                 << programId << ")" << std::endl;
        delete [] data->binaryData;
        delete data;
        return;
      }

      //Pack shader paths into worker data
      data->shaderPaths = new std::string[shaderCount];
      for (unsigned int i = 0; i < shaderCount; i++) {
        data->shaderPaths[i] = shaderPaths[i];
      }

      ammonite::utils::thread::submitWork(doCacheWork, data, nullptr);
    }

    bool checkObject(GLuint objectId, const char* actionString, GLenum statusEnum,
                            void (*objectQuery)(GLuint, GLenum, GLint*),
                            void (*objectLog)(GLuint, GLsizei, GLsizei*, GLchar*)) {
      //Test whether the program linked
      GLint success = GL_FALSE;
      objectQuery(objectId, statusEnum, &success);

      //If the program linked successfully, return
      if (success == GL_TRUE) {
        return true;
      }

      //Get length of a log, if available
      GLsizei maxLength = 0;
      objectQuery(objectId, GL_INFO_LOG_LENGTH, &maxLength);
      if (maxLength == 0) {
        ammonite::utils::warning << "Failed to " << actionString << " (ID " << objectId \
                                 << "), no log available" << std::endl;
        return false;
      }

      /*
       - Fetch and print the log
       - The extra byte isn't strictly required, but some drivers are buggy
      */
      GLchar* errorLogBuffer = new GLchar[maxLength + 1];
      objectLog(objectId, maxLength, nullptr, errorLogBuffer);
      errorLogBuffer[maxLength] = '\0';
      ammonite::utils::warning << "Failed to " << actionString << " (ID " << objectId \
                               << "):\n" << (char*)errorLogBuffer << std::endl;

      delete [] errorLogBuffer;
      return false;
    }

    bool checkProgram(GLuint programId, bool isCached) {
      return checkObject(programId, isCached ? "upload shader program" : "link shader program",
                         GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog);
    }

    bool checkShader(GLuint shaderId) {
      return checkObject(shaderId, "compile shader stage", GL_COMPILE_STATUS,
                         glGetShaderiv, glGetShaderInfoLog);
    }

    GLenum attemptIdentifyShaderType(std::string shaderPath) {
      std::string lowerShaderPath;
      for (unsigned int i = 0; i < shaderPath.size(); i++) {
        lowerShaderPath += (char)std::tolower(shaderPath[i]);
      }

      //Try and match the filename to a supported shader
      for (auto it = shaderMatches.begin(); it != shaderMatches.end(); it++) {
        if (lowerShaderPath.contains(it->first)) {
          if (it->second != GL_FALSE) {
            return it->second;
          }
        }
      }

      return GL_FALSE;
    }
  }

  //Shader compilation and cache functions, local to this file
  namespace {
    //Take shader source code, compile it and load it
    GLuint loadShader(const std::string& shaderPath, const GLenum shaderType) {
      //Create the shader
      GLuint shaderId = glCreateShader(shaderType);

      //Read the shader's source code
      std::size_t shaderCodeSize = 0;
      char* shaderCodePtr = (char*)ammonite::utils::files::loadFile(shaderPath,
                                                                    &shaderCodeSize);
      if (shaderCodePtr == nullptr) {
        ammonite::utils::warning << "Failed to open '" << shaderPath << "'" << std::endl;
        return 0;
      }

      ammoniteInternalDebug << "Compiling '" << shaderPath << "'" << std::endl;
      glShaderSource(shaderId, 1, &shaderCodePtr, (int*)&shaderCodeSize);
      glCompileShader(shaderId);
      delete [] shaderCodePtr;

      //Check whether the shader compiled, log if relevant
      if (!checkShader(shaderId)) {
        glDeleteShader(shaderId);
        return 0;
      }

      return shaderId;
    }

    //Take multiple shader objects and create a program
    GLuint createProgramObject(GLuint* shaderIds, unsigned int shaderCount) {
      //Create the program
      GLuint programId = glCreateProgram();

      //Attach and link all passed shader IDs
      for (unsigned int i = 0; i < shaderCount; i++) {
        glAttachShader(programId, shaderIds[i]);
      }
      glLinkProgram(programId);

      //Detach and remove all passed shader IDs
      for (unsigned int i = 0; i < shaderCount; i++) {
        glDetachShader(programId, shaderIds[i]);
        glDeleteShader(shaderIds[i]);
      }

      //Check whether the program linked, log if relevant
      if (!checkProgram(programId, false)) {
        glDeleteProgram(programId);
        return 0;
      }

      return programId;
    }

    //Create a program from shader source with loadShader() and createProgramObject()
    GLuint createProgramUncached(std::string* shaderPaths, GLenum* shaderTypes,
                                        unsigned int shaderCount) {
      //Since cache wasn't available, generate fresh shaders
      GLuint* shaderIds = new GLuint[shaderCount];
      bool hasCreatedShaders = true;
      for (unsigned int i = 0; i < shaderCount; i++) {
        shaderIds[i] = loadShader(shaderPaths[i], shaderTypes[i]);
        if (shaderIds[i] == 0) {
          hasCreatedShaders = false;
          break;
        }
      }

      //Create the program from the shaders
      GLuint programId = 0;
      if (hasCreatedShaders) {
        programId = createProgramObject(shaderIds, shaderCount);
      }

      //Clean up shaders if linking failed
      if (programId == 0) {
        for (unsigned int i = 0; i < shaderCount; i++) {
          if (shaderIds[i] != 0) {
            glDeleteShader(shaderIds[i]);
          } else {
            break;
          }
        }
      }

      delete [] shaderIds;
      return programId;
    }

    //Attempt to use a cached program or hand off to createProgramUncached()
    GLuint createProgramCached(std::string* shaderPaths, GLenum* shaderTypes,
                                      unsigned int shaderCount) {
      //Check for OpenGL and engine cache support
      bool isCacheSupported = ammonite::utils::files::getCacheEnabled();
      isCacheSupported = isCacheSupported && isBinaryCacheSupported;

      //Try and fetch the cache, then try and load it into a program
      GLuint programId = 0;
      std::string cacheFilePath;
      if (isCacheSupported) {
        unsigned char* userData = nullptr;
        std::size_t cacheDataSize = 0;
        std::size_t userDataSize = 0;
        AmmoniteEnum cacheState = AMMONITE_CACHE_INVALID;

        //Attempt to load the cached program
        unsigned char* cacheData = ammonite::utils::files::getCachedFile(&cacheFilePath,
          shaderPaths, shaderCount, &cacheDataSize, &userData, &userDataSize, &cacheState);

        //Fetch and validate binary format
        GLenum binaryFormat = 0;
        if (cacheState == AMMONITE_CACHE_HIT && userDataSize != 0) {
          long long rawFormat = std::atoll((char*)userData);
          binaryFormat = rawFormat;

          if (binaryFormat == 0) {
            ammonite::utils::warning << "Failed to get binary format for cached program" \
                                     << std::endl;
            cacheState = AMMONITE_CACHE_INVALID;
            delete [] cacheData;
          }
        }

        if (cacheState == AMMONITE_CACHE_HIT) {
          //Load the cached binary data into a program
          programId = glCreateProgram();
          glProgramBinary(programId, binaryFormat, cacheData, (GLsizei)cacheDataSize);
          delete [] cacheData;

          //Return the program ID if the cache worked
          if (checkProgram(programId, true)) {
            return programId;
          }

          //Cache was faulty, delete it and carry on
          ammonite::utils::warning << "Failed to process '" << cacheFilePath \
                                   << "'" << std::endl;
          ammonite::utils::status << "Clearing '" << cacheFilePath << "'" << std::endl;
          glDeleteProgram(programId);
          ammonite::utils::files::deleteFile(cacheFilePath);
        }
      }

      //Cache wasn't usable, compile a fresh program
      programId = createProgramUncached(shaderPaths, shaderTypes, shaderCount);

      //Cache the binary if enabled and successful
      if (programId != 0 && isCacheSupported) {
        cacheProgram(programId, shaderPaths, shaderCount, &cacheFilePath);
      }

      return programId;
    }
  }

  //Internally exposed functions
  namespace shaders {
    namespace internal {
      //Set isBinaryCacheSupported according to OpenGL support
      void updateCacheSupport() {
        //Get number of supported formats
        GLint numBinaryFormats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormats);

        //Check support for collecting the program binary
        isBinaryCacheSupported = true;
        if (!graphics::internal::checkExtension("GL_ARB_get_program_binary", "GL_VERSION_4_1")) {
          ammonite::utils::warning << "Program caching unsupported" << std::endl;
          isBinaryCacheSupported = false;
        } else if (numBinaryFormats < 1) {
          ammonite::utils::warning << "Program caching unsupported (no supported formats)" \
                                   << std::endl;
          isBinaryCacheSupported = false;
        }
      }

      /*
       - Take an array of shader paths, create a program and return the ID
         - Shaders with unidentifiable types will be ignored
         - If possible, load and store a cache
       - Returns 0 on failure
      */
      GLuint createProgram(std::string* inputShaderPaths, unsigned int inputShaderCount) {
        //Don't attempt to load 0 shaders
        if (inputShaderCount == 0) {
          return 0;
        }

        //Find all shaders
        std::vector<std::string> shaderPaths(0);
        std::vector<GLenum> shaderTypes(0);
        for (unsigned int i = 0; i < inputShaderCount; i++) {
          std::filesystem::path filePath{inputShaderPaths[i]};
          const std::string& extension = filePath.extension();

          if (shaderMatches.contains(extension)) {
            GLenum shaderType = shaderMatches[extension];

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
            if (shaderType == GL_TESS_CONTROL_SHADER || shaderType == GL_TESS_EVALUATION_SHADER) {
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
        return createProgramCached(shaderPaths.data(), shaderTypes.data(), shaderPaths.size());
      }

      /*
       - Create a program from shaders in a directory and return the ID
         - The order of shaders may be changed without re-caching
         - If possible, load and store a cache
       - Returns 0 on failure
      */
      GLuint loadDirectory(const std::string& directoryPath) {
        //Create filesystem directory iterator
        std::filesystem::directory_iterator it;
        try {
          const std::filesystem::path shaderDir{directoryPath};
          it = std::filesystem::directory_iterator{shaderDir};
        } catch (const std::filesystem::filesystem_error&) {
          ammonite::utils::warning << "Failed to load '" << directoryPath << "'" << std::endl;
          return 0;
        }

        //Find files to send to next stage
        std::vector<std::string> shaderPaths(0);
        for (auto const& fileName : it) {
          const std::filesystem::path& filePath{fileName};
          shaderPaths.push_back(std::string(filePath));
        }

        //Ensure shaders don't get rebuilt due to a file order change
        std::ranges::sort(shaderPaths);

        //Create the program and return the ID
        return createProgram(shaderPaths.data(), shaderPaths.size());
      }
    }
  }
}

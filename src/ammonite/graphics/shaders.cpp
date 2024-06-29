#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

//Used by shader cache validator
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <GL/glew.h>

#include "../core/fileManager.hpp"
#include "../utils/logging.hpp"
#include "internal/internalExtensions.hpp"

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
          ammonite::files::internal::getFileMetadata(shaderPaths[i], &filesize, &modificationTime);

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

    struct CacheInfo {
      int fileCount;
      std::string* filePaths;
      GLenum binaryFormat;
      GLsizei binaryLength;
    };

    //Set by updateCacheSupport(), when GLEW loads
    bool isBinaryCacheSupported = false;
  }

  //Shader compilation and cache functions, local to this file
  namespace {
    //Take shader source code, compile it and load it
    static int loadShader(std::string shaderPath, const GLenum shaderType, bool* externalSuccess) {
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

    bool validateCache(unsigned char* data, std::size_t size, void* userPtr) {
      CacheInfo* cacheInfoPtr = (CacheInfo*)userPtr;
      unsigned char* dataCopy = new unsigned char[size];
      std::memcpy(dataCopy, data, size);
      dataCopy[size - 1] = '\0';

      /*
       - Decide whether the cache file can be used
       - Uses input files, sizes and timestamps
      */
      char* state;
      int fileCount = 1;
      char* token = strtok_r((char*)dataCopy, "\n", &state);
      do {
        //Give up if token is null, we didn't find enough files
        if (token == nullptr) {
          delete [] dataCopy;
          return false;
        }

        //Check first token is 'input'
        std::string currentFilePath = cacheInfoPtr->filePaths[fileCount - 1];
        char* nestedToken = strtok(token, ";");
        if ((nestedToken == nullptr) ||
            (std::string(nestedToken) != std::string("input"))) {
          delete [] dataCopy;
          return false;
        }

        //Check token matches shader path
        nestedToken = strtok(nullptr, ";");
        if ((nestedToken == nullptr) ||
            (std::string(nestedToken) != currentFilePath)) {
          delete [] dataCopy;
          return false;
        }

        //Get filesize and time of last modification of the shader source
        long long int filesize = 0, modificationTime = 0;
        if (!ammonite::files::internal::getFileMetadata(currentFilePath, &filesize,
                                                        &modificationTime)) {
          //Failed to get the metadata
          delete [] dataCopy;
          return false;
        }

        //Check token matches file size
        nestedToken = strtok(nullptr, ";");
        if ((nestedToken == nullptr) ||
            (std::atoll(nestedToken) != filesize)) {
          delete [] dataCopy;
          return false;
        }

        //Check token matches timestamp
        nestedToken = strtok(nullptr, ";");
        if ((nestedToken == nullptr) ||
            (std::atoll(nestedToken) != modificationTime)) {
          delete [] dataCopy;
          return false;
        }

        //Get the next line
        if (cacheInfoPtr->fileCount > fileCount) {
          token = strtok_r(nullptr, "\n", &state);
        }
        fileCount += 1;
      } while (cacheInfoPtr->fileCount >= fileCount);

      //Find binary format
      token = strtok_r(nullptr, "\n", &state);
      if (token != nullptr) {
        cacheInfoPtr->binaryFormat = std::atoll(token);
      } else {
        delete [] dataCopy;
        return false;
      }

      //Find binary length
      token = strtok_r(nullptr, "\n", &state);
      if (token != nullptr) {
        cacheInfoPtr->binaryLength = std::atoll(token);
      } else {
        delete [] dataCopy;
        return false;
      }

      if (cacheInfoPtr->binaryFormat == 0 || cacheInfoPtr->binaryLength == 0) {
        delete [] dataCopy;
        return false;
      }

      delete [] dataCopy;
      return true;
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
        CacheInfo cacheInfo;
        cacheInfo.fileCount = shaderCount;
        cacheInfo.filePaths = shaderPaths;
        std::string cacheFilePath = ammonite::files::internal::getCachedFilePath(shaderPaths,
                                                                                 shaderCount);
        unsigned char* cacheData = ammonite::files::internal::getCachedFile(cacheFilePath,
          validateCache, &cacheDataSize, &cacheState, &cacheInfo);
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

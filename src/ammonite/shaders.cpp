#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <sys/stat.h>

#include <GL/glew.h>

namespace ammonite {
  namespace {
    //Vector to store all existing shaders
    std::vector<int> shaderIds(0);

    //Toggle and path for binary caching, set by useProgramCache()
    bool cacheBinaries = false;
    std::string programCacheDir;
  }

  //Static helper functions
  namespace {
    static void deleteCacheFile(const char* cacheName) {
      const std::string targetFiles[2] = {
        programCacheDir + cacheName + ".cache",
        programCacheDir + cacheName + ".cacheinfo"
      };

      //Delete the cache and cachinfo files
      std::cout << "Clearing '" << targetFiles[0] << "'" << std::endl;
      for (int i = 0; i < 2; i++) {
        if (targetFiles[i] != "/") {
          std::remove(targetFiles[i].c_str());
        }
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
      std::cerr << &errorLog[0] << std::endl;

      return false;
    }

    //In C++ 20, the filesystem library can do this
    void getFileMetadata(std::string filePath, long* filesize, long* timestamp) {
      struct stat fileInfo;
      if (stat(filePath.c_str(), &fileInfo) != 0) {
        //Failed to open file, fail the shader
        *filesize = 0;
        *timestamp = 0;
        return;
      }

      *timestamp = fileInfo.st_mtime;
      *filesize = fileInfo.st_size;
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
        if (!glewIsSupported("GL_VERSION_4_3") and !GLEW_ARB_compute_shader) {
          std::cerr << "Compute shaders unsupported" << std::endl;
          *externalSuccess = false;
          return 0;
        }
      }

      //Check for tessellation shader support if needed
      if (shaderType == GL_TESS_CONTROL_SHADER or shaderType == GL_TESS_EVALUATION_SHADER) {
        if (!glewIsSupported("GL_VERSION_4_0") and !GLEW_ARB_tessellation_shader) {
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
  }

  //Binary caching related code
  namespace shaders {
    bool useProgramCache(const char* programCachePath) {
      //Check binary cache support
      if (!glewIsSupported("GL_VERSION_4_1") and !GLEW_ARB_get_program_binary) {
        std::cerr << "Binary caching unsupported" << std::endl;
        cacheBinaries = false;
        return false;
      }

      //Attempt to create a cache directory
      try {
        std::filesystem::create_directory(programCachePath);
      } catch (const std::filesystem::filesystem_error&) {
        std::cerr << "Failed to create cache directory: '" << programCachePath << "'" << std::endl;
        cacheBinaries = false;
        return false;
      }

      //If the cache directory doesn't exist, disable caching and exit
      if (!std::filesystem::is_directory(programCachePath)) {
        std::cerr << "Couldn't find cache directory: '" << programCachePath << "'" << std::endl;
        cacheBinaries = false;
        return false;
      }

      //Enable cache and ensure path has a trailing slash
      cacheBinaries = true;
      programCacheDir = programCachePath;
      if (programCacheDir.back() != '/') {
        programCacheDir.push_back('/');
      }

      std::cout << "Binary caching enabled ('" << programCacheDir << "')" << std::endl;
      return true;
    }

    GLuint createProgram(const std::string shaderPaths[], const int shaderTypes[], const int shaderCount, bool* externalSuccess, const char* programName) {
      //Used later as the return value
      GLuint programId;

      const std::string targetFiles[2] = {
        programCacheDir + programName + ".cache",
        programCacheDir + programName + ".cacheinfo"
      };

      //If caching is enabled, attempt to use a cached version instead
      if (cacheBinaries) {
        //Check the cache files are present
        bool filesExist = true;
        for (int i = 0; i < 2; i++) {
          if (!std::filesystem::exists(targetFiles[i])) {
            //File(s) missing, cache can't be used
            filesExist = false;
            i = 1;
          }
        }

        //Attempt to load the cached file
        bool cacheValid = true;
        if (filesExist) {
          GLenum cachedBinaryFormat = 0;
          GLsizei cachedBinaryLength = 0;

          std::string line;
          std::ifstream cachedBinaryInfoFile(targetFiles[1]);
          if (cachedBinaryInfoFile.is_open()) {
            //Get the binary format
            getline(cachedBinaryInfoFile, line);
            cachedBinaryFormat = stoi(line);
            //Get the length of the binary
            getline(cachedBinaryInfoFile, line);
            cachedBinaryLength = stoi(line);

            //Validate filesize and access times of shaders used
            for (int i = 0; i < shaderCount; i++) {
              //Get expected filename, filesize and timestamp
              std::vector<std::string> strings;
              getline(cachedBinaryInfoFile, line);
              std::stringstream rawLine(line);

              while (getline(rawLine, line, ';')) {
                strings.push_back(line);
              }

              if (strings.size() == 3) {
                if (strings[0] != shaderPaths[i]) {
                  //Program made from different shaders, invalidate
                  cacheValid = false;
                  i = shaderCount;
                }

                //Get filesize and time of last modification of the shader source
                long filesize, modificationTime;
                getFileMetadata(shaderPaths[i], &filesize, &modificationTime);

                if (stoi(strings[1]) != filesize or stoi(strings[2]) != modificationTime) {
                  //Shader source code has changed, invalidate
                  cacheValid = false;
                  i = shaderCount;
                }
              } else {
                //Cache info file broken, invalidate
                cacheValid = false;
                i = shaderCount;
              }
            }

            cachedBinaryInfoFile.close();
          }

          if (cacheValid) {
            //Read the cached data from file
            char cachedBinaryData[cachedBinaryLength];
            std::ifstream input(targetFiles[0], std::ios::binary);
            input.read(&cachedBinaryData[0], cachedBinaryLength);

            //Load the cached binary data
            programId = glCreateProgram();
            glProgramBinary(programId, cachedBinaryFormat, cachedBinaryData, cachedBinaryLength);

            //Return the program ID, unless the cache was faulty, then delete and carry on
            if (checkProgram(programId)) {
              return programId;
            } else {
              std::cerr << "Failed to process '" << targetFiles[0] << "'" << std::endl;
              deleteCacheFile(programName);
            }
          } else {
            //Shader source doesn't match cache, delete the old cache
            deleteCacheFile(programName);
          }
        }
      }

      GLuint shaderIds[shaderCount];
      for (int i = 0; i < shaderCount; i++) {
        shaderIds[i] = loadShader(shaderPaths[i].c_str(), shaderTypes[i], externalSuccess);
      }

      //Create the program like normal, as a valid cache wasn't found
      programId = createProgram(shaderIds, shaderCount, externalSuccess);

      if (!*externalSuccess) {
        ammonite::shaders::eraseShaders();
        return 0;
      }

      //If caching is enabled, cache the binary
      if (cacheBinaries) {
        std::cout << "Caching '" << targetFiles[0] << "'" << std::endl;
        int binaryLength;
        GLenum binaryFormat;

        //Get binary length and data of linked program
        glGetProgramiv(programId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
        char binaryData[binaryLength];
        glGetProgramBinary(programId, binaryLength, NULL, &binaryFormat, &binaryData);

        //Write the binary to cache directory
        std::ofstream binarySave(targetFiles[0], std::ios::binary);
        if (binarySave.is_open()) {
          binarySave.write(&binaryData[0], binaryLength);
        } else {
          std::cerr << "Failed to cache program" << std::endl;
          deleteCacheFile(programName);
          return programId;
        }

        //Write the cache info to cache directory as programName.cacheinfo
        std::ofstream binaryInfo(targetFiles[1]);
        if (binaryInfo.is_open()) {
          binaryInfo << binaryFormat << "\n";
          binaryInfo << binaryLength << "\n";

          for (int i = 0; i < shaderCount; i++) {
            long filesize, modificationTime;
            getFileMetadata(shaderPaths[i], &filesize, &modificationTime);

            binaryInfo << shaderPaths[i] << ";" << filesize << ";" << modificationTime << "\n";
          }

          binaryInfo.close();
        } else {
          std::cerr << "Failed to cache program" << std::endl;
          deleteCacheFile(programName);
          return programId;
        }
      }

      return programId;
    }
  }
}

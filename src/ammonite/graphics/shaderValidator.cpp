#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../core/fileManager.hpp"
#include "internal/internalShaderValidator.hpp"

namespace ammonite {
  namespace shaders {
    namespace internal {
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
    }
  }
}

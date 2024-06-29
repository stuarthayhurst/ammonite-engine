#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../core/fileManager.hpp"
#include "internal/internalShaderValidator.hpp"

namespace ammonite {
  namespace shaders {
    namespace internal {
      namespace {
        /*
         - Similar to strtok_r, except it only supports single character delimiters
         - Unsafe if called with both input and (save or *save) as nullptr / undefined
         - save must never be nullptr
        */
        static char* parseToken(char* input, char delim, char** save) {
          //Restore from next byte to search
          if (input == nullptr) {
            input = *save;
          }

          //Programmer error or no more tokens
          if (*input == '\0') {
            return nullptr;
          }

          char* end = input;
          //Search for delimiter or end of string
          while (*end != delim && *end != '\0') {
            end++;
          }

          //Last token found
          if (*end == '\0') {
            return input;
          }

          //Create a string, save state and return it
          *end = '\0';
          *save = end + 1;
          return input;
        }
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
        char* state = nullptr;
        int fileCount = 1;
        long long int filesize = 0, modificationTime = 0;
        char* token = parseToken((char*)dataCopy, '\n', &state);
        do {
          //Give up if token is null, we didn't find enough files
          if (token == nullptr) {
            goto failed;
          }

          //Check first token is 'input'
          std::string currentFilePath = cacheInfoPtr->filePaths[fileCount - 1];
          char* nestedState = nullptr;
          char* nestedToken = parseToken(token, ';', &nestedState);
          if ((nestedToken == nullptr) ||
              (std::string(nestedToken) != std::string("input"))) {
            goto failed;
          }

          //Check token matches shader path
          nestedToken = parseToken(nullptr, ';', &nestedState);
          if ((nestedToken == nullptr) || (std::string(nestedToken) != currentFilePath)) {
            goto failed;
          }

          //Get filesize and time of last modification of the shader source
          if (!ammonite::files::internal::getFileMetadata(currentFilePath, &filesize,
                                                          &modificationTime)) {
            goto failed;
          }

          //Check token matches file size
          nestedToken = parseToken(nullptr, ';', &nestedState);
          if ((nestedToken == nullptr) || (std::atoll(nestedToken) != filesize)) {
            goto failed;
          }

          //Check token matches timestamp
          nestedToken = parseToken(nullptr, ';', &nestedState);
          if ((nestedToken == nullptr) || (std::atoll(nestedToken) != modificationTime)) {
            goto failed;
          }

          //Get the next line
          if (cacheInfoPtr->fileCount > fileCount) {
            token = parseToken(nullptr, '\n', &state);
          }
          fileCount += 1;
        } while (cacheInfoPtr->fileCount >= fileCount);

        //Find binary format
        token = parseToken(nullptr, '\n', &state);
        if (token == nullptr) {
            goto failed;
        }
        cacheInfoPtr->binaryFormat = std::atoll(token);

        //Find binary length
        token = parseToken(nullptr, '\n', &state);
        if (token == nullptr) {
            goto failed;
        }
        cacheInfoPtr->binaryLength = std::atoll(token);

        if (cacheInfoPtr->binaryFormat != 0 && cacheInfoPtr->binaryLength != 0) {
          delete [] dataCopy;
          return true;
        }

failed:
        delete [] dataCopy;
        return false;
      }
    }
  }
}

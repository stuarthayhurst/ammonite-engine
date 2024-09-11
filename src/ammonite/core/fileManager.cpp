#include <cstddef>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__VAES__) && defined(__BMI2__)
  #define USE_VAES_AVX512
  #include <immintrin.h>
#endif

#include "../enums.hpp"
#include "../types.hpp"

#include "../utils/debug.hpp"
#include "../utils/logging.hpp"

#include "fileManager.hpp"

#define MAX_LOAD_ATTEMPTS 10

namespace ammonite {
  namespace files {
    namespace internal {
      namespace {
        //Whether or not this manager is ready for cache use
        bool cacheEnabled = false;
        std::string dataCachePath = std::string("");
      }

      namespace {
        /*
         - Hash together fileCount paths from filePaths
           - Use an AVX-512 + VAES implementation, if supported
           - The AVX-512 + VAES and generic versions produce different results, but both work
             - On a Ryzen 7 7700X, the AVX-512 + VAES accelerated version is around 64x faster
         - Don't use this for security, you'll lose your job
        */
#ifdef USE_VAES_AVX512
        static std::string generateCacheString(std::string* filePaths, unsigned int fileCount) {
          __m512i last = _mm512_setzero_epi32();
          for (unsigned int i = 0; i < fileCount; i++) {
            uint8_t* filePath = (uint8_t*)filePaths[i].c_str();
            int pathSize = filePaths[i].length();

            while (pathSize >= 64) {
              __m512i a = _mm512_loadu_epi8(filePath);
              last = _mm512_aesenc_epi128(last, a);

              pathSize -= 64;
              filePath += 64;
            }

            if (pathSize > 0) {
              __mmask64 mask = _bzhi_u64(0xFFFFFFFF, pathSize);
              __m512i a = _mm512_maskz_loadu_epi8(mask, filePath);
              last = _mm512_aesenc_epi128(last, a);
            }
          }

          return std::to_string((uint64_t)_mm512_reduce_add_epi64(last));
        }
#else
        static std::string generateCacheString(std::string* filePaths,
                                               unsigned int fileCount) {
          alignas(uint64_t) uint8_t output[8] = {0};
          uint8_t prev = 0;

          /*
           - XOR the first byte of the hash with the first character of the first path
           - Sequentially XOR every byte of the hash with the result of the previous
             operation of this stage
           - Repeat this process for every character of every path
          */
          for (unsigned int i = 0; i < fileCount; i++) {
            uint8_t* filePath = (uint8_t*)filePaths[i].c_str();
            int pathLength = filePaths[i].length();
            for (int i = 0; i < pathLength; i++) {
              output[0] ^= filePath[i];
              for (int j = 0; j < 8; j++) {
                output[j] ^= prev;
                prev = output[j];
              }
            }
          }

          return std::to_string(*(uint64_t*)output);
        }
#endif

        static std::string getCachedFilePath(std::string* filePaths, unsigned int fileCount) {
          return dataCachePath + generateCacheString(filePaths, fileCount) + std::string(".cache");
        }

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

        //Check paths, times and file sizes are correct
        static AmmoniteEnum validateInputs(std::string* filePaths, unsigned int fileCount,
                                           unsigned char* extraData, std::size_t extraDataSize) {
          unsigned char* extraDataCopy = new unsigned char[extraDataSize + 1];
          std::memcpy(extraDataCopy, extraData, extraDataSize);
          extraDataCopy[extraDataSize] = '\0';

          /*
           - Decide whether the cache file can be used
           - Uses input files, sizes and timestamps
          */
          AmmoniteEnum result = AMMONITE_CACHE_INVALID;
          char* state = nullptr;
          unsigned int internalFileCount = 1;
          long long int filesize = 0, modificationTime = 0;
          char* token = parseToken((char*)extraDataCopy, '\n', &state);
          do {
            //Give up if token is null, we didn't find enough files
            if (token == nullptr) {
              break;
            }

            //Check first token is 'input'
            std::string currentFilePath = filePaths[internalFileCount - 1];
            char* nestedState = nullptr;
            char* nestedToken = parseToken(token, ';', &nestedState);
            if ((nestedToken == nullptr) ||
                (std::string(nestedToken) != std::string("input"))) {
              break;
            }

            //Check token matches shader path
            nestedToken = parseToken(nullptr, ';', &nestedState);
            if ((nestedToken == nullptr) || (std::string(nestedToken) != currentFilePath)) {
              //Files are different, cache collision occurred
              result = AMMONITE_CACHE_COLLISION;
              break;
            }

            //Get filesize and time of last modification of the shader source
            if (!ammonite::files::internal::getFileMetadata(currentFilePath, &filesize,
                                                            &modificationTime)) {
              break;
            }

            //Check token matches file size
            nestedToken = parseToken(nullptr, ';', &nestedState);
            if ((nestedToken == nullptr) || (std::atoll(nestedToken) != filesize)) {
              break;
            }

            //Check token matches timestamp
            nestedToken = parseToken(nullptr, ';', &nestedState);
            if ((nestedToken == nullptr) || (std::atoll(nestedToken) != modificationTime)) {
              break;
            }

            //Get the next line
            if (fileCount > internalFileCount) {
              token = parseToken(nullptr, '\n', &state);
            } else {
              result = AMMONITE_CACHE_HIT;
            }
            internalFileCount += 1;
          } while (fileCount >= internalFileCount);

          delete [] extraDataCopy;
          return result;
        }

        static void deleteCacheFile(std::string filePath) {
          ammonite::utils::status << "Clearing '" << filePath << "'" << std::endl;
          deleteFile(filePath);
        }
      }

      void deleteFile(std::string filePath) {
        if (std::filesystem::exists(filePath)) {
          std::remove(filePath.c_str());
        }
      }

      bool getFileMetadata(std::string filePath, long long int* filesize,
                           long long int* timestamp) {
        //Give up if the file doesn't exist
        std::filesystem::path filesystemPath = std::string(filePath);
        if (!std::filesystem::exists(filesystemPath)) {
          return false;
        }

        //Get a time point for last write time of the file
        std::filesystem::file_time_type fileTimestamp =
          std::filesystem::last_write_time(filesystemPath);

        //Convert time point to unix time
        *timestamp =
          std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(fileTimestamp));

        //Get filesize of the file
        *filesize = std::filesystem::file_size(filesystemPath);
        return true;
      }

      //Attempt to setup targetCachePath for caching, and return whether it can be used
      bool useDataCache(std::string targetCachePath) {
        //Attempt to create the cache directory if it doesn't already exist
        if (!std::filesystem::is_directory(targetCachePath)) {
          ammonite::utils::warning << "Couldn't find cache directory '" << targetCachePath \
                                   << "', creating it instead" << std::endl;
          try {
            std::filesystem::create_directory(targetCachePath);
          } catch (const std::filesystem::filesystem_error&) {
            ammonite::utils::warning << "Failed to create cache directory '" \
                                     << targetCachePath << "'" << std::endl;
            cacheEnabled = false;
            dataCachePath = std::string("");
            return false;
          }
        }

        //Check for read and write permissions
        if (access(targetCachePath.c_str(), R_OK | W_OK)) {
          ammonite::utils::warning << "Insufficient permissions to use cache directory '" \
                                   << targetCachePath << "'" << std::endl;
          return false;
        }

        //Ensure path has a trailing slash
        dataCachePath = targetCachePath;
        if (dataCachePath.back() != '/') {
          dataCachePath.push_back('/');
        }

        ammonite::utils::status << "Data caching enabled ('" << dataCachePath << "')" << std::endl;
        cacheEnabled = true;
        return true;
      }

      //Return whether or not this manager is ready for cache use
      bool getCacheEnabled() {
        return cacheEnabled;
      }

      /*
       - Read in filePath, then return the data and write to size
         - Returned data must be freed
       - If the read fails, return null
         - In this case, nothing needs to be freed
      */
      unsigned char* loadFile(std::string filePath, std::size_t* size) {
        unsigned char* data = nullptr;
        int descriptor = open(filePath.c_str(), O_RDONLY);
        if (descriptor == -1) {
          ammonite::utils::warning << "Error while opening '" << filePath \
                                   << "' (" << -errno << ")" << std::endl;
          return nullptr;
        }

        struct stat statBuf;
        fstat(descriptor, &statBuf);
        *size = statBuf.st_size;
        data = new unsigned char[statBuf.st_size];

        if (posix_fadvise(descriptor, 0, 0, POSIX_FADV_SEQUENTIAL)) {
          ammonite::utils::warning << "Error while advising kernel, continuing" << std::endl;
        }

        off_t bytesRead = 0;
        while (bytesRead < statBuf.st_size) {
          off_t newBytesRead = read(descriptor, data + bytesRead, statBuf.st_size - bytesRead);
          if (newBytesRead == 0) {
            break;
          } else if (newBytesRead < 0) {
            ammonite::utils::warning << "Error while reading '" << filePath \
                                     << "' (" << -errno << ")" << std::endl;
            close(descriptor);
            delete [] data;
            return nullptr;
          }

          bytesRead += newBytesRead;
          if (bytesRead == statBuf.st_size) {
            break;
          }
        }

        if (bytesRead != statBuf.st_size) {
            ammonite::utils::warning << "Unexpected file size while reading '" << filePath \
                                     << "'" << std::endl;
            close(descriptor);
            delete [] data;
            return nullptr;
        }

        close(descriptor);
        return data;
      }

      /*
       - Write size bytes of data to filePath
         - Create the file if missing
         - Erase the file if present
       - Returns true on success, false on failure
      */
      bool writeFile(std::string filePath, unsigned char* data, std::size_t size) {
        int descriptor = creat(filePath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (descriptor == -1) {
          ammonite::utils::warning << "Error while opening '" << filePath \
                                   << "' (" << -errno << ")" << std::endl;
          return false;
        }

        if (posix_fadvise(descriptor, 0, 0, POSIX_FADV_SEQUENTIAL)) {
          ammonite::utils::warning << "Error while advising kernel, continuing" << std::endl;
        }

        std::size_t bytesWritten = 0;
        while (bytesWritten < size) {
          off_t newBytesWritten = write(descriptor, data + bytesWritten, size - bytesWritten);
          if (newBytesWritten == 0) {
            break;
          } else if (newBytesWritten < 0) {
            ammonite::utils::warning << "Error while writing to '" << filePath \
                                     << "' (" << -errno << ")" << std::endl;
            close(descriptor);
            return false;
          }

          bytesWritten += newBytesWritten;
          if (bytesWritten == size) {
            break;
          }
        }

        if (bytesWritten != size) {
            ammonite::utils::warning << "Unexpected file size while writing to '" << filePath \
                                     << "'" << std::endl;
            close(descriptor);
            return false;
        }

        close(descriptor);
        return true;
      }

      /*
       - Attempt to read a cached file, from file paths, checking timestamps and file sizes
       - If the cache was valid, write to dataSize and cacheState, then return cacheData
         - In this case, cacheData needs to be freed after use
       - If the cache was invalid write to cacheState and return nullptr
         - In this case, cache data, dataSize, *userData and userDataSize should be ignored,
           and the cache will be cleared
       - Write the address of any supplied user data to userData, and its size to userDataSize
         - If userDataSize is 0, no user data was supplied
         - *userData doesn't need to be freed, it's part of the return value's allocation
       - The (expected) cache path is written to cacheFilePath
      */
      unsigned char* getCachedFile(std::string* cacheFilePath, std::string* filePaths,
                                   unsigned int fileCount, std::size_t* dataSize,
                                   unsigned char** userData, std::size_t* userDataSize,
                                   AmmoniteEnum* cacheState) {
        //Generate a cache string
        *cacheFilePath = getCachedFilePath(filePaths, fileCount);

        //Attempt to load the cache file, try another string on collision
        unsigned char* cacheData = nullptr;
        int attempts = 0;
        bool collision = false;
        bool failed = false;
        do {
          collision = false;

          //Check cache file exists
          if (!std::filesystem::exists(*cacheFilePath)) {
            ammoniteInternalDebug << "Couldn't find " << *cacheFilePath << std::endl;
            *cacheState = AMMONITE_CACHE_MISS;
            return nullptr;
          }

          //Attempt to read the cache if it exists, writes to size
          std::size_t size;
          std::size_t blockSizes[3];
          cacheData = loadFile(*cacheFilePath, &size);
          if (cacheData == nullptr || size < sizeof(blockSizes)) {
            ammonite::utils::warning << "Failed to read '" << *cacheFilePath << "'" << std::endl;
            *cacheState = AMMONITE_CACHE_MISS;

            //Cache data may or may not have been returned, due to size check
            if (cacheData != nullptr) {
              delete [] cacheData;
              *cacheState = AMMONITE_CACHE_INVALID;
            }
            return nullptr;
          }

          //Get the sizes and start addresses of the data, user and extra blocks
          std::memcpy(blockSizes, cacheData + size - sizeof(blockSizes), sizeof(blockSizes));
          *dataSize = blockSizes[0];
          *userData = cacheData + blockSizes[0];
          *userDataSize = blockSizes[1];
          unsigned char* extraData = *userData + *userDataSize;
          std::size_t extraDataSize = blockSizes[2] - sizeof(blockSizes);

          //Check size of data is as expected, then validate the loaded cache
          if (blockSizes[0] + blockSizes[1] + blockSizes[2] != size) {
            ammonite::utils::warning << "Incorrect size information for '" << *cacheFilePath \
                                     << "'" << std::endl;
            failed = true;
          } else {
            AmmoniteEnum result = validateInputs(filePaths, fileCount, extraData, extraDataSize);
            if (result == AMMONITE_CACHE_COLLISION) {
              //Append the attempt counter to the file path and try again
              collision = true;
              cacheFilePath->erase(cacheFilePath->rfind('.'));
              *cacheFilePath += std::string("-") + std::to_string(attempts) +
                                std::string(".cache");

            } else if (result != AMMONITE_CACHE_HIT) {
              ammonite::utils::warning << "Failed to validate '" << *cacheFilePath \
                                       << "'" << std::endl;
              failed = true;
            }
          }

          //Increment attempts, and try again if we had a collision
          attempts++;
        } while (collision && attempts < MAX_LOAD_ATTEMPTS);

        //Handle too many collision resolution attempts
        if (attempts >= MAX_LOAD_ATTEMPTS && collision) {
          ammonite::utils::warning << "Maximum number of collision resolution attempts reached" \
                                   << std::endl;
          failed = true;
        }

        //Clean up after a failure
        if (failed) {
          ammonite::utils::status << "Clearing '" << *cacheFilePath << "'" << std::endl;
          deleteFile(*cacheFilePath);

          delete [] cacheData;
          *cacheState = AMMONITE_CACHE_INVALID;
          return nullptr;
        }

        *cacheState = AMMONITE_CACHE_HIT;
        return cacheData;
      }

      /*
       - Write dataSize bytes of data to cacheFilePath, using filePaths to generate the
         cache information
       - Also write userDataSize bytes of userData to the cache file
       - Returns true on success, false on failure
      */
      bool writeCacheFile(std::string cacheFilePath, std::string* filePaths,
                          unsigned int fileCount, unsigned char* data, std::size_t dataSize,
                          unsigned char* userData, std::size_t userDataSize) {
        //Generate data to write
        std::string extraData;
        std::size_t blockSizes[3];
        for (unsigned int i = 0; i < fileCount; i++) {
          long long int filesize = 0, modificationTime = 0;
          ammonite::files::internal::getFileMetadata(filePaths[i], &filesize, &modificationTime);
          extraData.append("input;" + filePaths[i]);
          extraData.append(";" + std::to_string(filesize));
          extraData.append(";" + std::to_string(modificationTime) + "\n");
        }

        std::size_t extraSize = extraData.length();
        std::size_t totalDataSize = dataSize + userDataSize + extraSize + sizeof(blockSizes);
        unsigned char* fileData = new unsigned char[totalDataSize];

        //blockSize and its size gets special handling, as it's not written to extraData
        blockSizes[0] = dataSize;
        blockSizes[1] = userDataSize;
        blockSizes[2] = extraSize + sizeof(blockSizes);

        /*
         - Write the binary data, user data and cache info to the buffer
         - The structure should be as follows:
           - Binary cache data block
           - User data block
           - Extra data block (for path, timestamp and size validation)
             - Includes sizes of each block
        */
        std::memcpy(fileData, data, dataSize);
        std::memcpy(fileData + dataSize, userData, userDataSize);
        extraData.copy((char*)fileData + dataSize + userDataSize, extraSize, 0);
        std::memcpy(fileData + dataSize + userDataSize + extraSize, blockSizes,
                    sizeof(blockSizes));

        //Write the data, user data and cache info to the cache file
        if (!ammonite::files::internal::writeFile(cacheFilePath, fileData, totalDataSize)) {
          ammonite::utils::warning << "Failed to cache '" << cacheFilePath << "'" << std::endl;
          deleteCacheFile(cacheFilePath);
          delete [] fileData;
          return false;
        }

        delete [] fileData;
        return true;
      }
    }
  }
}

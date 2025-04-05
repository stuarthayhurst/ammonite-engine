#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>

extern "C" {
  #include <errno.h>
  #include <fcntl.h>
  #include <sys/stat.h>
  #include <unistd.h>
}

#include "files.hpp"

#include "debug.hpp"
#include "hash.hpp"
#include "logging.hpp"
#include "../enums.hpp"

static constexpr unsigned int MAX_LOAD_ATTEMPTS = 10;

namespace ammonite {
  namespace utils {
    namespace files {
      namespace {
        //Whether or not this manager is ready for cache use
        bool cacheEnabled = false;
        std::string dataCachePath = std::string("");
      }

      namespace {
        std::string getCachedFilePath(std::string* filePaths, unsigned int fileCount) {
          return dataCachePath + internal::hashStrings(filePaths, fileCount).append(".cache");
        }

        /*
         - Similar to strtok_r, except it only supports single character delimiters
         - Unsafe if called with both input and (save or *save) as nullptr / undefined
         - save must never be nullptr
        */
        char* parseToken(char* input, char delim, char** save) {
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
        AmmoniteEnum validateInputs(std::string* filePaths, unsigned int fileCount,
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
          std::size_t filesize = 0;
          std::time_t modificationTime = 0;
          char* token = parseToken((char*)extraDataCopy, '\n', &state);
          while (internalFileCount <= fileCount) {
            //Give up if token is null, we didn't find enough files
            if (token == nullptr) {
              break;
            }

            //Check first token is 'input'
            const std::string& currentFilePath = filePaths[internalFileCount - 1];
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
            if (!ammonite::utils::files::getFileMetadata(currentFilePath, &filesize,
                                                         &modificationTime)) {
              break;
            }

            //Check token matches file size
            nestedToken = parseToken(nullptr, ';', &nestedState);
            if ((nestedToken == nullptr) || ((std::size_t)std::atoll(nestedToken) != filesize)) {
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
          }

          delete [] extraDataCopy;
          return result;
        }

        void deleteCacheFile(const std::string& filePath) {
          ammonite::utils::status << "Clearing '" << filePath << "'" << std::endl;
          deleteFile(filePath);
        }
      }

      void deleteFile(const std::string& filePath) {
        if (std::filesystem::exists(filePath)) {
          std::remove(filePath.c_str());
        }
      }

      bool getFileMetadata(const std::string& filePath, std::size_t* filesize, std::time_t* timestamp) {
        //Give up if the file doesn't exist
        const std::filesystem::path filesystemPath = filePath;
        if (!std::filesystem::exists(filesystemPath)) {
          return false;
        }

        //Get a time point for last write time of the file
        const std::filesystem::file_time_type fileTimestamp =
          std::filesystem::last_write_time(filesystemPath);

        //Convert time point from file time to system time, then save it
        *timestamp = std::chrono::system_clock::to_time_t(
          std::chrono::clock_cast<std::chrono::system_clock>(fileTimestamp));

        //Get filesize of the file
        *filesize = std::filesystem::file_size(filesystemPath);
        return true;
      }

      /*
       - Attempt to setup targetCachePath for caching, and return whether it can be used
       - This path will be used for all caches created by the engine, as well as by the user
      */
      bool useDataCache(const std::string& targetCachePath) {
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
        if ((access(targetCachePath.c_str(), R_OK | W_OK)) != 0) {
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
      unsigned char* loadFile(const std::string& filePath, std::size_t* size) {
        unsigned char* data = nullptr;
        const int descriptor = open(filePath.c_str(), O_RDONLY);
        if (descriptor == -1) {
          ammonite::utils::warning << "Error while opening '" << filePath \
                                   << "' (" << -errno << ")" << std::endl;
          return nullptr;
        }

        struct stat statBuf;
        fstat(descriptor, &statBuf);
        *size = statBuf.st_size;
        data = new unsigned char[statBuf.st_size];

        if ((posix_fadvise(descriptor, 0, 0, POSIX_FADV_SEQUENTIAL)) != 0) {
          ammonite::utils::warning << "Error while advising kernel, continuing" << std::endl;
        }

        intmax_t bytesRead = 0;
        while (bytesRead < statBuf.st_size) {
          const intmax_t newBytesRead = read(descriptor, data + bytesRead,
                                             statBuf.st_size - bytesRead);
          if (newBytesRead == 0) {
            break;
          }

          if (newBytesRead < 0) {
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
      bool writeFile(const std::string& filePath, unsigned char* data, std::size_t size) {
        const int descriptor = creat(filePath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (descriptor == -1) {
          ammonite::utils::warning << "Error while opening '" << filePath \
                                   << "' (" << -errno << ")" << std::endl;
          return false;
        }

        if ((posix_fadvise(descriptor, 0, 0, POSIX_FADV_SEQUENTIAL)) != 0) {
          ammonite::utils::warning << "Error while advising kernel, continuing" << std::endl;
        }

        std::size_t bytesWritten = 0;
        while (bytesWritten < size) {
          const intmax_t newBytesWritten = write(descriptor, data + bytesWritten,
                                                 size - bytesWritten);
          if (newBytesWritten == 0) {
            break;
          }

          if (newBytesWritten < 0) {
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
        unsigned int attempts = 0;
        bool collision = true;
        bool failed = false;
        while (collision && attempts < MAX_LOAD_ATTEMPTS) {
          collision = false;

          //Check cache file exists
          if (!std::filesystem::exists(*cacheFilePath)) {
            ammoniteInternalDebug << "Couldn't find " << *cacheFilePath << std::endl;
            *cacheState = AMMONITE_CACHE_MISS;
            return nullptr;
          }

          //Attempt to read the cache if it exists, writes to size
          std::size_t size = 0;
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
          const std::size_t extraDataSize = blockSizes[2] - sizeof(blockSizes);

          //Check size of data is as expected, then validate the loaded cache
          if (blockSizes[0] + blockSizes[1] + blockSizes[2] != size) {
            ammonite::utils::warning << "Incorrect size information for '" << *cacheFilePath \
                                     << "'" << std::endl;
            failed = true;
          } else {
            const AmmoniteEnum result = validateInputs(filePaths, fileCount,
                                                       extraData, extraDataSize);
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
        }

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
      bool writeCacheFile(const std::string& cacheFilePath, std::string* filePaths,
                          unsigned int fileCount, unsigned char* data, std::size_t dataSize,
                          unsigned char* userData, std::size_t userDataSize) {
        //Generate data to write
        std::string extraData;
        std::size_t blockSizes[3];
        for (unsigned int i = 0; i < fileCount; i++) {
          std::size_t filesize = 0;
          std::time_t modificationTime = 0;
          ammonite::utils::files::getFileMetadata(filePaths[i], &filesize, &modificationTime);
          extraData.append("input;" + filePaths[i]);
          extraData.append(";" + std::to_string(filesize));
          extraData.append(";" + std::to_string(modificationTime) + "\n");
        }

        const std::size_t extraSize = extraData.length();
        const std::size_t totalDataSize = dataSize + userDataSize + extraSize + sizeof(blockSizes);
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
        if (!ammonite::utils::files::writeFile(cacheFilePath, fileData, totalDataSize)) {
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

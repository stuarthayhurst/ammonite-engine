#include <cstddef>
#include <chrono>
#include <filesystem>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../enums.hpp"
#include "../types.hpp"

#include "../utils/debug.hpp"
#include "../utils/logging.hpp"

namespace ammonite {
  namespace files {
    namespace internal {
      namespace {
        //Whether or not this manager is ready for cache use
        bool cacheEnabled = false;
        std::string dataCachePath = std::string("");
      }

      namespace {
        //Hash input filenames together to create a unique cache string
        static std::string generateCacheString(std::string* filePaths,
                                               unsigned int fileCount) {
          std::string inputString = std::string("");
          for (unsigned int i = 0; i < fileCount; i++) {
            inputString += filePaths[i] + std::string(";");
          }

          return std::to_string(std::hash<std::string>{}(inputString));
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

      std::string getCachedFilePath(std::string* filePaths, unsigned int fileCount) {
        return dataCachePath + generateCacheString(filePaths, fileCount) + std::string(".cache");
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
                                   << "' (" << errno << ")" << std::endl;
        }
        posix_fadvise(descriptor, 0, 0, POSIX_FADV_SEQUENTIAL);

        struct stat statBuf;
        fstat(descriptor, &statBuf);
        *size = statBuf.st_size;
        data = new unsigned char[statBuf.st_size];

        off_t bytesRead = 0;
        while (bytesRead < statBuf.st_size) {
          off_t newBytesRead = read(descriptor, data + bytesRead,
                                          statBuf.st_size - bytesRead);
          if (newBytesRead == 0) {
            break;
          } else if (newBytesRead < 0) {
            ammonite::utils::warning << "Error while reading '" << filePath \
                                     << "' (" << errno << ")" << std::endl;
            close(descriptor);
            delete [] data;
            return nullptr;
          }
          bytesRead += newBytesRead;
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
       - Attempt to read a cached file, and validate it with validator
       - If the cache was valid, write to size and cacheState, then return cacheData
         - In this case, cacheData needs to be freed after use
       - If the cache was invalid write to cacheState and return nullptr
         - In this case, cacheData and size should be ignored, and the cache will be cleared
      */
      unsigned char* getCachedFile(std::string cacheFilePath, AmmoniteValidator validator,
                                   std::size_t* size, AmmoniteEnum* cacheState, void* userPtr) {
        //Check cache file exists
        if (!std::filesystem::exists(cacheFilePath)) {
          ammoniteInternalDebug << "Couldn't find " << cacheFilePath << std::endl;
          *cacheState = AMMONITE_CACHE_MISS;
          return nullptr;
        }

        //Attempt to read the cache if it exists, writes to size
        unsigned char* cacheData = loadFile(cacheFilePath, size);
        if (cacheData == nullptr) {
          ammonite::utils::warning << "Failed to read '" << cacheFilePath << "'" << std::endl;
          *cacheState = AMMONITE_CACHE_MISS;
          return nullptr;
        }

        //Validate the loaded cache
        if (!validator(cacheData, *size, userPtr)) {
          ammonite::utils::warning << "Failed to validate '" << cacheFilePath << "'" << std::endl;
          ammonite::utils::status << "Clearing '" << cacheFilePath << "'" << std::endl;
          deleteFile(cacheFilePath);

          delete [] cacheData;
          *cacheState = AMMONITE_CACHE_INVALID;
          return nullptr;
        }

        *cacheState = AMMONITE_CACHE_HIT;
        return cacheData;
      }
    }
  }
}

#include <filesystem>
#include <string>
#include <chrono>

#include "internalDebug.hpp"

namespace ammonite {
  namespace utils {
    namespace files {
      void deleteFile(std::string filePath) {
        if (std::filesystem::exists(filePath)) {
          std::remove(filePath.c_str());
        }
      }

      bool getFileMetadata(const char* filePath, long long int* filesize, long long int* timestamp) {
        //Give up if the file doesn't exist
        std::filesystem::path filesystemPath = std::string(filePath);
        if (!std::filesystem::exists(filesystemPath)) {
          return false;
        }

        //Get a time point for last write time of the file
        std::filesystem::file_time_type fileTimestamp = std::filesystem::last_write_time(filesystemPath);

        //Convert time point to unix time
        *timestamp = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(fileTimestamp));

        //Get filesize of the file
        *filesize = std::filesystem::file_size(filesystemPath);
        return true;
      }
    }
  }
}

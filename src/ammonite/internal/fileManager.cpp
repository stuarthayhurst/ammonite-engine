#include <sys/stat.h>
#include <filesystem>
#include <string>

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace utils {
    namespace files {
      void deleteFile(std::string filePath) {
        if (std::filesystem::exists(filePath)) {
          std::remove(filePath.c_str());
        }
      }

      //In C++ 20, the std::filesystem can do this
      bool getFileMetadata(const char* filePath, long long int* filesize, long long int* timestamp) {
        struct stat fileInfo;
        if (stat(filePath, &fileInfo) != 0) {
          //Failed to open file, fail the cache
          return false;
        }

        *timestamp = fileInfo.st_mtime;
        *filesize = fileInfo.st_size;
       return true;
      }
    }
  }
}

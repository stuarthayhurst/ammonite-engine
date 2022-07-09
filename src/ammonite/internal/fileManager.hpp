#ifndef FILES
#define FILES

namespace ammonite {
  namespace utils {
    namespace files {
      void deleteFile(std::string filePath);
      bool getFileMetadata(const char* filePath, long long int* filesize, long long int* timestamp);
    }
  }
}

#endif

#ifndef INTERNALFILES
#define INTERNALFILES

/* Internally exposed header:
 - Expose some file handling methods internally
*/

namespace ammonite {
  namespace utils {
    namespace internal {
      void deleteFile(std::string filePath);
      bool getFileMetadata(const char* filePath, long long int* filesize, long long int* timestamp);
    }
  }
}

#endif

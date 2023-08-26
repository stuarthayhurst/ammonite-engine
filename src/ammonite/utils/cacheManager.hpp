#ifndef CACHE
#define CACHE

namespace ammonite {
  namespace utils {
    namespace cache {
      bool useDataCache(const char* dataCachePath);
      bool getCacheEnabled();
    }
  }
}

#endif

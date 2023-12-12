#ifndef INTERNALEXTENSION
#define INTERNALEXTENSION

/* Internally exposed header:
 - Expose extension checking functions
*/

namespace ammonite {
  namespace graphics {
    namespace internal {
      bool checkExtension(const char* extension, const char* version);
      bool checkExtension(const char* extension);
    }
  }
}

#endif

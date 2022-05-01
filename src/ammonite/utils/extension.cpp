#include <iostream>
#include <GL/glew.h>

namespace ammonite {
  namespace utils {
    bool checkExtension(const char extension[], const char version[]) {
      if (glewIsSupported(extension) or glewIsSupported(version)) {
        //Extension supported, either explicitly or by version
        #ifdef DEBUG
          std::cout << "DEBUG: " << extension << " supported (" << version << ")" << std::endl;
        #endif
        return true;
      }

      //Extension unsupported
      #ifdef DEBUG
        std::cout << "DEBUG: " << extension << " unsupported (" << version << ")" << std::endl;
      #endif
      return false;
    }

    //Allow checking for extensions without a fallback version
    bool checkExtension(const char extension[]) {
      if (glewIsSupported(extension)) {
        //Extension supported
        #ifdef DEBUG
          std::cout << "DEBUG: " << extension << " supported" << std::endl;
        #endif
        return true;
      }

      //Extension unsupported
      #ifdef DEBUG
        std::cout << "DEBUG: " << extension << " unsupported" << std::endl;
      #endif
      return false;
    }
  }
}

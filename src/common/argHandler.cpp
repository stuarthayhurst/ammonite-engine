#include <string>

#ifdef DEBUG
  #include <iostream>
#endif

namespace arguments {
  int searchArgument(int argc, char* argv[], const char identifier[], bool isToggle, std::string* argValue) {
    for (int i = 0; i < argc; i++) {
      if (argv[i] == std::string(identifier)) {
        //If argument is found and it's a toggle, return true
        if (isToggle == true) {
          return 1;
        }

        //If we're not at the end of argv, return the next value
        if (i + 1 < argc) {
          *argValue = std::string(argv[i + 1]);
          return 1;
        //If we're at the end exit with -1
        } else {
          return -1;
        }
      }
    }

    return 0;
  }
}

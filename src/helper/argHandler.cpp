#include <string>

namespace arguments {
  /*
   - Look for identifier in argv
   - If it's not found, return 0
   - If it's found and its value (if any) should be ignored, return 1
   - If it's found and its value shouldn't be ignored, return 1
     - Additionally, write its value to argValuePtr
     - If there isn't a value to save, return -1
  */
  int searchArgument(int argc, char** argv, std::string identifier, std::string* argValuePtr) {
    for (int i = 0; i < argc; i++) {
      if (argv[i] == std::string(identifier)) {
        //If argument is found and it's a toggle, return true
        if (argValuePtr == nullptr) {
          return 1;
        }

        //If we're not at the end of argv, return the next value
        if (i + 1 < argc) {
          *argValuePtr = std::string(argv[i + 1]);
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

#ifndef ARGUMENTS
#define ARGUMENTS

#include <string>

namespace arguments {
  int searchArgument(int argc, char** argv, const std::string& identifier,
                     std::string* argValuePtr);
}

#endif

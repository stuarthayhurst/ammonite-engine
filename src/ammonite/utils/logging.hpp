#ifndef LOGGING
#define LOGGING

#include <iostream>
#include <string>

namespace ammonite {
  namespace utils {
    class OutputHelper {
    private:
      std::ostream& outputStream;
      std::string prefix;
      bool hasFlushed = true;

    public:
      OutputHelper(std::ostream& output, std::string pre):outputStream(output) {
        prefix = pre;
      }

      template<typename T> OutputHelper& operator << (T&& x) {
        if (hasFlushed) {
          outputStream << prefix << x;
          hasFlushed = false;
        } else {
          outputStream << x;
        }
        return *this;
      }

      //Handle std::endl
      OutputHelper& operator << (std::ostream& (*)(std::ostream&)) {
        outputStream << std::endl;
        hasFlushed = true;
        return *this;
      }
    };

    extern OutputHelper error;
    extern OutputHelper warning;
    extern OutputHelper status;
  }
}

#endif

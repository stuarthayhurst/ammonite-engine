#ifndef LOGGING
#define LOGGING

#include <iostream>
#include <string>

namespace ammonite {
  namespace utils {
    class Output {
    private:
      std::ostream& outputStream;
      std::string prefix;
      bool hasFlushed = true;

    public:
      Output(std::ostream& output, std::string pre):outputStream(output) {
        prefix = pre;
      }
      template<typename T> Output& operator << (T&& x) {
        if (hasFlushed) {
          outputStream << prefix << x;
          hasFlushed = false;
        } else {
          outputStream << x;
        }
        return *this;
      }

      //Handle std::endl
      Output& operator << (std::ostream& (*)(std::ostream&)) {
        outputStream << std::endl;
        hasFlushed = true;
        return *this;
      }
    };

    extern Output error;
    extern Output warning;
    extern Output status;
  }
}

#endif

#ifndef LOGGING
#define LOGGING

#include <iostream>
#include <string>
#include <utility>

namespace ammonite {
  namespace utils {
    class OutputHelper {
    private:
      //NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
      std::ostream& outputStream;
      std::string prefix;
      bool hasFlushed = true;

    public:
      OutputHelper(std::ostream& output, std::string pre);
      template<typename T> OutputHelper& operator << (T&& input);
      OutputHelper& operator << (std::ostream& (*)(std::ostream&));
    };

    template<typename T> OutputHelper& OutputHelper::operator << (T&& input) {
      if (hasFlushed) {
        outputStream << prefix << std::forward<T>(input);
        hasFlushed = false;
      } else {
        outputStream << std::forward<T>(input);
      }

      return *this;
    }

    //NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
    extern OutputHelper error;
    extern OutputHelper warning;
    extern OutputHelper status;
    //NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
  }
}

#endif

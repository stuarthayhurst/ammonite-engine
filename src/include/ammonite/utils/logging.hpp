#ifndef LOGGING
#define LOGGING

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

namespace ammonite {
  namespace utils {
    namespace colour {
      const std::string black = "\033[30m";
      const std::string red = "\033[31m";
      const std::string green = "\033[32m";
      const std::string yellow = "\033[33m";
      const std::string blue = "\033[34m";
      const std::string magenta = "\033[35m";
      const std::string cyan = "\033[36m";
      const std::string white = "\033[37m";
    }

    /*
     - Output helper to buffer output data and synchronise with other helpers to display
     - Prefixes the output with the prefix of the instant used to flush
     - Buffer is shared between all instances on the same thread
     - Colour is only used for the prefix
    */
    class OutputHelper {
    private:
      //NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
      std::ostream& outputStream;
      std::string prefix;

      static thread_local std::stringstream storageStream;

    public:
      OutputHelper(std::ostream& output, const std::string& pre);
      OutputHelper(std::ostream& output, const std::string& pre, const std::string& colour);
      template<typename T> OutputHelper& operator << (T&& input);
      OutputHelper& operator << (std::ostream& (*)(std::ostream&));
    };

    template<typename T> OutputHelper& OutputHelper::operator << (T&& input) {
      storageStream << std::forward<T>(input);
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

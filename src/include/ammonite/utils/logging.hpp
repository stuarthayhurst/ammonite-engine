#ifndef LOGGING
#define LOGGING

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

namespace ammonite {
  namespace utils {
    /*
     - Output helper to buffer output data and synchronise with other helpers to display
     - Prefixes the output with the prefix of the instant used to flush
     - Buffer is shared between all instances on the same thread
    */
    class OutputHelper {
    private:
      //NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
      std::ostream& outputStream;
      std::string prefix;

    public:
      OutputHelper(std::ostream& output, const std::string& pre);
      template<typename T> OutputHelper& operator << (T&& input);
      OutputHelper& operator << (std::ostream& (*)(std::ostream&));
      static void writeStorage(const std::string& data);
    };

    template<typename T> OutputHelper& OutputHelper::operator << (T&& input) {
      std::stringstream temp;
      temp << std::forward<T>(input);
      writeStorage(temp.str());
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

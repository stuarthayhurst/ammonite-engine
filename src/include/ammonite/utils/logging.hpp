#ifndef AMMONITELOGGING
#define AMMONITELOGGING

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include "../maths/vec.hpp"

namespace ammonite {
  namespace utils {
    namespace colour {
      constexpr const char* black = "\033[30m";
      constexpr const char* red = "\033[31m";
      constexpr const char* green = "\033[32m";
      constexpr const char* yellow = "\033[33m";
      constexpr const char* blue = "\033[34m";
      constexpr const char* magenta = "\033[35m";
      constexpr const char* cyan = "\033[36m";
      constexpr const char* white = "\033[37m";

      constexpr const char* none = "";
      constexpr const char* reset = "\033[0m";
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
      OutputHelper(std::ostream& output, const std::string& pre, const char* colour);
      template<typename T> OutputHelper& operator << (T&& input);
      OutputHelper& operator << (std::ostream& (*)(std::ostream&));

      template<typename T, std::size_t size> requires ammonite::validVector<T, size>
      OutputHelper& operator << (ammonite::Vec<T, size>&);

      void printEmptyLine();
    };

    template<typename T> OutputHelper& OutputHelper::operator << (T&& input) {
      storageStream << std::forward<T>(input);
      return *this;
    }

    template<typename T, std::size_t size> requires ammonite::validVector<T, size>
    OutputHelper& OutputHelper::operator << (ammonite::Vec<T, size>& vector) {
      for (std::size_t i = 0; i < size; i++) {
        if (i != 0) {
          storageStream << ", ";
        }

        storageStream << vector[i];
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

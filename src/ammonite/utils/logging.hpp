#ifndef LOGGING
#define LOGGING

#include <iostream>

namespace ammonite {
  namespace utils {
    class Error {
    private:
      bool hasFlushed = true;

    public:
      template<typename T> Error& operator << (T&& x) {
        if (hasFlushed) {
          std::cerr << "ERROR: " << x;
          hasFlushed = false;
        } else {
          std::cerr << x;
        }
        return *this;
      }

      //Handle std::endl
      Error& operator << (std::ostream& (*)(std::ostream&)) {
        std::cerr << std::endl;
        hasFlushed = true;
        return *this;
      }
    };

    class Warning {
    private:
      bool hasFlushed = true;

    public:
      template<typename T> Warning& operator << (T&& x) {
        if (hasFlushed) {
          std::cerr << "WARNING: " << x;
          hasFlushed = false;
        } else {
          std::cerr << x;
        }
        return *this;
      }

      //Handle std::endl
      Warning& operator << (std::ostream& (*)(std::ostream&)) {
        std::cerr << std::endl;
        hasFlushed = true;
        return *this;
      }
    };

    class Status {
    private:
      bool hasFlushed = true;

    public:
      template<typename T> Status& operator << (T&& x) {
        if (hasFlushed) {
          std::cout << "STATUS: " << x;
          hasFlushed = false;
        } else {
          std::cout << x;
        }
        return *this;
      }

      //Handle std::endl
      Status& operator << (std::ostream& (*)(std::ostream&)) {
        std::cout << std::endl;
        hasFlushed = true;
        return *this;
      }
    };

    extern Error error;
    extern Warning warning;
    extern Status status;
  }
}

#endif

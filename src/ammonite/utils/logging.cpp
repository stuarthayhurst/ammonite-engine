#include <iostream>
#include <mutex>
#include <string>
#include <sstream>

#include "logging.hpp"

namespace ammonite {
  namespace utils {
    namespace {
      std::mutex outputLock;

      constexpr const char* reset = "\033[0m";
    }

    OutputHelper::OutputHelper(std::ostream& output, const std::string& pre):
      outputStream(output), prefix(pre) {}

    OutputHelper::OutputHelper(std::ostream& output, const std::string& pre, const std::string& colour):
      outputStream(output), prefix(colour + pre + reset) {}

    //Output the stored string
    OutputHelper& OutputHelper::operator << (std::ostream& (*)(std::ostream&)) {
      //Safely output the prefix, buffered string and new line
      outputLock.lock();
      outputStream << prefix << storageStream.str() << std::endl;
      outputLock.unlock();

      //Clear the storage
      storageStream.clear();
      storageStream.str(std::string());

      return *this;
    }

    void OutputHelper::printEmptyLine() {
      outputLock.lock();
      outputStream << std::endl;
      outputLock.unlock();
    }

    /*NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,
                  cppcoreguidelines-interfaces-global-init)*/
    thread_local std::stringstream OutputHelper::storageStream = std::stringstream("");

    OutputHelper error(std::cerr, "ERROR: ", colour::red);
    OutputHelper warning(std::cerr, "WARNING: ", colour::yellow);
    OutputHelper status(std::cout, "STATUS: ", colour::green);
    /*NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,
                cppcoreguidelines-interfaces-global-init)*/
  }
}

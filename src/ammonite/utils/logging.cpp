#include <iostream>
#include <mutex>
#include <string>
#include <sstream>

#include "logging.hpp"

namespace ammonite {
  namespace utils {
    namespace {
      std::mutex outputLock;
    }

    OutputHelper::OutputHelper(std::ostream& output, const std::string& pre):
      outputStream(output), prefix(pre) {}

    OutputHelper::OutputHelper(std::ostream& output, const std::string& pre, const char* colour):
      outputStream(output), prefix(colour + pre + colour::reset) {}

    //Output the stored string
    OutputHelper& OutputHelper::operator << (std::ostream& (*newStream)(std::ostream&)) {
      //Safely output the prefix, buffered string and new line
      outputLock.lock();
      outputStream << prefix << storageStream.str() << newStream;
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
    OutputHelper normal(std::cout, "", colour::none);
    /*NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,
                cppcoreguidelines-interfaces-global-init)*/
  }
}

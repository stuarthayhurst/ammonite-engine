#include <iostream>
#include <mutex>
#include <string>
#include <sstream>

#include "logging.hpp"

namespace ammonite {
  namespace utils {
    namespace {
      thread_local std::stringstream storageStream;
      std::mutex outputLock;
    }

    OutputHelper::OutputHelper(std::ostream& output, const std::string& pre):
      outputStream(output), prefix(pre) {}

    void OutputHelper::writeStorage(const std::string& data) {
      storageStream << data;
    }

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

    /*NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,
                  cppcoreguidelines-interfaces-global-init)*/
    OutputHelper error(std::cerr, "ERROR: ");
    OutputHelper warning(std::cerr, "WARNING: ");
    OutputHelper status(std::cout, "STATUS: ");
    /*NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,
                cppcoreguidelines-interfaces-global-init)*/
  }
}

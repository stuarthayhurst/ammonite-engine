#include "logging.hpp"

namespace ammonite {
  namespace utils {
    OutputHelper::OutputHelper(std::ostream& output, std::string pre):outputStream(output),
      prefix(pre) {}

    //Handle std::endl
    OutputHelper& OutputHelper::operator << (std::ostream& (*)(std::ostream&)) {
      outputStream << std::endl;
      hasFlushed = true;
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

#include "logging.hpp"

namespace ammonite {
  namespace utils {
    //NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
    OutputHelper error(std::cerr, "ERROR: ");
    OutputHelper warning(std::cerr, "WARNING: ");
    OutputHelper status(std::cout, "STATUS: ");
    //NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
  }
}

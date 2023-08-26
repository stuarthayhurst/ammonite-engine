#include "logging.hpp"

namespace ammonite {
  namespace utils {
    OutputHelper error(std::cerr, "ERROR: ");
    OutputHelper warning(std::cerr, "WARNING: ");
    OutputHelper status(std::cout, "STATUS: ");
  }
}

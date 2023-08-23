#include "logging.hpp"

namespace ammonite {
  namespace utils {
    Output error(std::cerr, "ERROR: ");
    Output warning(std::cerr, "WARNING: ");
    Output status(std::cout, "STATUS: ");
  }
}

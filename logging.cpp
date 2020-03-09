#include <iostream>
#include "logging.h"

namespace logging {
  Trace::Trace (const char *f, std::stringstream &ostr) : fun(f)
  {
    std::cerr << "T B(" << ++lev << ") " << fun
      << " with " << ostr.str() << std::endl;
  }

  Trace::~Trace ()
  {
    std::cerr << "T E(" << lev-- << ") " << fun << std::endl;
  }
  int Trace::lev = 0;
}

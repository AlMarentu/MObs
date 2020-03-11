#include <iostream>
#include "logging.h"

namespace logging {
  Trace::Trace (const char *f, const std::string &str) : fun(f)
  {
    if (traceOn)
      std::cerr << "T B(" << ++lev << ") " << fun
          << " with " << str << std::endl;
  }

  Trace::~Trace ()
  {
    if (traceOn)
      std::cerr << "T E(" << lev-- << ") " << fun << std::endl;
  }
  int Trace::lev = 0;
  bool Trace::traceOn = false;
}

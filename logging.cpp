#include <iostream>
#include "logging.h"

namespace logging {
void logMessage(loglevel l, const std::string &message)
  {
    char c = ' ';
    switch(l)
    {
      case lm_debug: c = 'D'; break;
      case lm_trace: c = 'T'; break;
      case lm_info: c = 'I'; break;
      case lm_error: c = 'E'; break;
      case lm_warn: c = 'W'; break;
    }
    std::cerr << c << " " << message << std::endl;
  }


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

#ifndef LOGGING_H
#define LOGGING_H

#include <sstream>


namespace logging {
  class Trace {
    public:
      Trace (const char *f, std::stringstream &ostr);
      ~Trace ();
    private:
      static int lev;
      const char *fun;
  };
}

#define PARAM(x) " " #x "=\"" << x << "\""
#ifndef NDEBUG
#define TRACE(x) std::stringstream ___s___; ___s___ << std::boolalpha << x; logging::Trace ___t___(__PRETTY_FUNCTION__, ___s___);
#else
#define TRACE(x)
#endif

#endif

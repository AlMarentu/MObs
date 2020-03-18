#ifndef LOGGING_H
#define LOGGING_H

#include <sstream>


namespace logging {

typedef enum {
  lm_trace = 1,
  lm_debug = 2,
  lm_info  = 3,
  lm_warn  = 4,
  lm_error = 5
} loglevel;

class Trace {
public:
  Trace (const char *f, const std::string &str);
  ~Trace ();
  static bool traceOn;
private:
  static int lev;
  const char *fun;
};

void logMessage(loglevel l, const std::string &message);

}

#define LM_DEBUG logging::lm_debug
#define LM_INFO logging::lm_info
#define LM_WARNING logging::lm_warn
#define LM_ERROR logging::lm_error


#define PARAM(x) " " #x "=\"" << x << "\""
#ifndef NDEBUG
#define TRACE(x) std::stringstream ___s___; ___s___ << std::boolalpha << x; logging::Trace ___t___(__PRETTY_FUNCTION__, ___s___.str());
#else
#define TRACE(x)
#endif

#define LOG(l,x) { std::stringstream ___s___; ___s___ << __FILE_NAME__ << ':' << __LINE__ << " " << std::boolalpha << x; logging::logMessage(l, ___s___.str()); }



#endif

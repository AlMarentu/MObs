// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs https://github.com/AlMarentu/MObs.git
//
// MObs is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#ifndef MOBS_LOGGING_H
#define MOBS_LOGGING_H

#include <sstream>

/** \file logging.h
\brief Hilfsfunktionen und Makros für logging und tracing
 
 Kann leicht dur andere Tools esetzt werden. Es mussen die Makros
 
 LOG(level, streamop)
 TRACE(streamop)
 PARAM(par)
 
 sowie dei Log_level-Makros existieren.
 
 
 */

namespace logging {

/// Log-Lebel, bitte Makros verwenden
typedef enum {
  lm_trace = 1,
  lm_debug = 2,
  lm_info  = 3,
  lm_warn  = 4,
  lm_error = 5
} loglevel;

/// Interne Klasse für Tracing, bitte Makro TRACE verwenden
class Trace {
public:
  /// \private
  Trace (const char *f, const std::string &str);
  ~Trace ();
  static bool traceOn; ///< schaltet Tracing zur Laufzeit ein und aus
private:
  static int lev;
  const char *fun;
};

void logMessage(loglevel l, const std::string &message);

}

#define LM_DEBUG logging::lm_debug  ///< Log-Level für Debug
#define LM_INFO logging::lm_info    ///< Log-Level für Info
#define LM_WARNING logging::lm_warn ///< Log-Level für Warning
#define LM_ERROR logging::lm_error  ///< Log-Level für Error

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif


/// \brief Erzeugt eine Logmeldung aud stderr.
#define LOG(l,x) { std::stringstream ___s___; ___s___ << __FILE_NAME__ << ':' << __LINE__ << " " << std::boolalpha << x; logging::logMessage(l, ___s___.str()); }


/// \brief Hilfs-Makro für TRACE, stellt Parameternamen vor Inhalt.
#define PARAM(x) " " #x "=\"" << x << "\""
#ifndef NDEBUG
/// \brief Makro für Tracing
#define TRACE(x) std::stringstream ___s___; ___s___ << std::boolalpha << x; logging::Trace ___t___(__PRETTY_FUNCTION__, ___s___.str())
#else
#define TRACE(x)
#endif



#endif

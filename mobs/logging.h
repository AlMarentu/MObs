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
#include <string>
#include <functional>

/** \file logging.h
\brief Hilfsfunktionen und Makros für logging und tracing
 
 Kann leicht durch andere Tools ersetzt werden. Es müssen die Makros
 
 LOG(level, streamOp)
 TRACE(streamOp)
 PARAM(par)
 THROW(streamOp)
 
 sowie die Log_level-Makros existieren.
 
 
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

extern loglevel currentLevel;

/// Interne Klasse für Tracing, bitte Makro TRACE verwenden
class Trace {
public:
  /// \private
  Trace (const char *f, std::function<std::string()> str);
  ~Trace ();
  static bool traceOn; ///< schaltet Tracing zur Laufzeit ein und aus
private:
  static int lev;
  const char *fun;
};

void logMessage(loglevel l, std::function<std::string()> message);

}

#define LM_DEBUG logging::lm_debug  ///< Log-Level für Debug
#define LM_INFO logging::lm_info    ///< Log-Level für Info
#define LM_WARNING logging::lm_warn ///< Log-Level für Warning
#define LM_ERROR logging::lm_error  ///< Log-Level für Error

#ifndef __FILE_NAME__
/// \private
#define __FILE_NAME__ __FILE__
#endif

/// \brief Erzeugt eine Log-Meldung als stream und wirft sie als runtime-error.
#define THROW(x) do { std::stringstream ___s___; ___s___ << __FILE_NAME__ << ':' << __LINE__ << " " << std::boolalpha << x; throw std::runtime_error(___s___.str()); } while(false)

/// \brief Erzeugt eine Log-Meldung auf stderr.
#define LOG(l,x) logging::logMessage(l, [&]()->std::string { std::stringstream ___s___; ___s___ << __FILE_NAME__ << ':' << __LINE__ << " " << std::boolalpha << x; return ___s___.str(); })

/// \brief Hilfs-Makro das einen Stream als std::string ausgibt
#define STRSTR(x) ([&]()->std::string { std::stringstream ___s___; ___s___ << x; return ___s___.str(); })()
/// \brief Makro zum Erzeugen eines Log std::string für exceptions o.ä.
#define LOGSTR(x) STRSTR(__FILE_NAME__ << ':' << __LINE__ << ' ' << std::boolalpha << x)


/// \brief Hilfs-Makro für TRACE, stellt Parameternamen vor Inhalt.
#define PARAM(x) " " #x "=\"" << x << "\""
#ifndef NDEBUG
/// \brief Makro für Tracing
#define TRACE(x) logging::Trace ___t___(__PRETTY_FUNCTION__, [&]()->std::string { std::stringstream ___s___; ___s___ << x; return ___s___.str(); })
#else
#define TRACE(x)
#endif



#endif

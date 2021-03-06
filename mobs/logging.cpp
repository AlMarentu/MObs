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


#include <iostream>
#include <iomanip>
#include "logging.h"

#ifdef __MINGW32__
typedef unsigned char u_char;
#endif

/*! \def PARAM(x)
 \brief Hilfs-Makro für TRACE, stellt Parameternamen vor Inhalt.
 */

/*! \def TRACE(x)
\brief Makro für Tracing
 */

/*! \def LOG(level, stream Inhalt
\brief Erzeugt eine Logmeldung aud stderr
 */

namespace logging {

loglevel currentLevel = lm_trace;


/// \brief Logmeldung ausgeben, interne Funktion, bitte Makro LOG() verwenden
/// \see LOG(l, x)
/// @param l Log-Level
/// @param message Inhalt der Log-Meldung als lambda
void logMessage(loglevel l, std::function<std::string()> message)
{
  if (l < currentLevel)
    return;

  char c = ' ';
  switch(l)
  {
    case lm_debug: c = 'D'; break;
    case lm_trace: c = 'T'; break;
    case lm_info: c = 'I'; break;
    case lm_error: c = 'E'; break;
    case lm_warn: c = 'W'; break;
  }
  std::string tmp;
  tmp += c;
  tmp += ' ';
  tmp += message();
  for (auto c:tmp) {
    switch(c) {
      case '\n': std::cerr << "<NL>"; break;
      case '\r': std::cerr << "<CR>"; break;
      case '\0' ... 0x09:
      case 0x0b:
      case 0x0e ... 0x1f: std::cerr << '<' << std::hex << std::setfill('0') << std::setw(2) << int(u_char(c)) << '>'; break;
      default: std::cerr << c;
    }
  }
  std::cerr  << std::endl;
}



Trace::Trace (const char *f, std::function<std::string()> str) : fun(f)
{
  if (traceOn)
    std::cerr << "T B(" << ++lev << ") " << fun
              << " with " << str() << std::endl;
}

/// Destruktor
Trace::~Trace ()
{
  if (traceOn)
    std::cerr << "T E(" << lev-- << ") " << fun << std::endl;
}

int Trace::lev = 0;
bool Trace::traceOn = false;

}

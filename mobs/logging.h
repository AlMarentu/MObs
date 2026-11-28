// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2026 Matthias Lautner
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
#include <array>
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

/// Log-Level, bitte im LOG-Makro die defines LM_DEBUG, LM_INFO, LM_WARNING und LM_ERROR verwenden
typedef enum {
  lm_trace = 1,
  lm_debug = 2,
  lm_info  = 3,
  lm_warn  = 4,
  lm_error = 5,
  lm_none = 6 ///< ausschließlich zum Abschalten des Logs
} loglevel;

extern loglevel currentLevel; ///< Level ab dem geloggt werden soll; bei lm_none erfolgt kein logging

extern std::function<void(char logType, const std::string &)> logFileCallback; ///< Callback für alternative LOG-Ausgabe; default = nullptr


/** \brief Alternitive Log-Funktion mit Ausgabe der Zeit auf stderr
 *
 * @param logType Typ der Meldunf 'T'race, 'D'ebug, 'I'nfo, 'W'arning, 'E'rror
 * @param text Log-Text aus dem LOG-Makro
 */
void logWithTime(char logType, const std::string &text);


/// Interne Klasse für Tracing; bitte Makro TRACE verwenden
class Trace {
public:
  /// \private
  Trace (const char *f, const std::function<std::string()> &str);
  ~Trace ();
  static bool traceOn; ///< schaltet Tracing zur Laufzeit ein und aus
private:
  static int lev;
  const char *fun;
};

/** \brief Klasse zur Behandlung von Logdateien über parallele Programme
 *
 * Die Klasse erzeugt eine Log-Datei. Diese wird immer mit .0 erweitert.
 * Ist diese Datei bereits von einem anderen Programm geöffnet, wird eine neue Datei mit .1 erzeugt, usw...
 *
 * Die wird erst erzeugt, wenn die erste Log-Meldung geschrieben wird.
 */

class FileMultiLog {
  class FileMultiLogData;
public:
  explicit FileMultiLog(const std::string &filenamePart);
  ~FileMultiLog();
  void logString(const std::string &str);
  int getVersion() const;

protected:
  FileMultiLogData *data;
};

/// streambuf zu FileMultiLog für LogMultiStream
class LogMultiBuf : public std::basic_streambuf<char> {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = Base::char_type; ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = Base::int_type; ///< zugehöriger int-Typ

  explicit LogMultiBuf(const std::string &filenamePart);
  /// \private
  LogMultiBuf(const LogMultiBuf &) = delete;
  LogMultiBuf &operator=(const LogMultiBuf &) = delete;
  ~LogMultiBuf() override;
  protected:
  /// \private
  int_type overflow(int_type ch) override;

  /// \private
  int sync() override;

private:
  FileMultiLog log;
  std::array<char_type, 1024> buffer {};
};

/// stream für FileMultiLog
class LogMultiStream : public std::ostream {
public:
  explicit LogMultiStream(const std::string & filenamePart);
  ~LogMultiStream() override;
};



void logMessage(loglevel l, const std::function<std::string()>& message);

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
/// \brief Hilfs-Makro das einen Stream als std::wstring ausgibt
#define WSTRSTR(x) ([&]()->std::wstring { std::wstringstream ___s___; ___s___ << x; return ___s___.str(); })()
/// \brief Makro zum Erzeugen eines Log std::string für exceptions o.ä.
#define LOGSTR(x) STRSTR(__FILE_NAME__ << ':' << __LINE__ << ' ' << std::boolalpha << x)


/// \brief Hilfs-Makro für TRACE, stellt Parameternamen vor Inhalt.
#define PARAM(x) " " #x "=\"" << x << "\""
#ifndef NTRACE
/// \brief Makro für Tracing
#define TRACE(x) logging::Trace ___t___(__PRETTY_FUNCTION__, [&]()->std::string { std::stringstream ___s___; ___s___ << x; return ___s___.str(); })
#else
#define TRACE(x)
#endif



#endif

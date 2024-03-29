// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2024 Matthias Lautner
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
#else
#include <fcntl.h>
#include <unistd.h>
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


FileMultiLog::~FileMultiLog() {
#ifdef _WIN32
  CloseHandle(handle);
#else
  ::close(handle);
#endif
}


void FileMultiLog::logString(const std::string &str) {
  if (version < 0) {
    for (int v = 0;;v++) {
      if (v > 100)
        throw std::runtime_error("Cannot open log file");
      std::string fn = fileName;
      fn += '.';
      fn += std::to_string(v);
      // create file or append existing file
#ifdef _WIN32
      handle = CreateFile(fn.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                          OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, nullptr);
      if (handle == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
          continue;
        throw std::runtime_error(STRSTR("Cannot open log file " << GetLastError()));
      }
      // write and end of file
      SetFilePointer(handle, 0, nullptr, FILE_END);
#else
      handle = open(fn.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0666);
      if (handle < 0) {
        throw std::runtime_error(STRSTR("Cannot open log file " << "errno=" << errno << " " << strerror(errno)));
      }
      /* l_type   l_whence  l_start  l_len  l_pid   */
      struct flock fl = {};
      fl.l_type = F_WRLCK;
      fl.l_whence = SEEK_SET;
      if (fcntl(handle, F_SETLK, &fl) < 0) {
        if (errno == EAGAIN || errno == EACCES) {
          close(handle);
          continue;
        }
        throw std::runtime_error(STRSTR("Cannot lock log file " << "errno=" << errno << " " << strerror(errno)));
      }
#endif
      version = v;
      break;
    }
  }
#ifdef _WIN32
  DWORD written;
  WriteFile(handle, str.c_str(), str.size(), &written, nullptr);
  if (written != str.size())
    throw std::runtime_error("Cannot write log file");
#else
  if (write(handle, str.c_str(), str.size()) != str.size())
    throw std::runtime_error("Cannot write log file");
#endif
}

LogMultiBuf::LogMultiBuf(const std::string &filenamePart) : log(filenamePart) {
  Base::setp(buffer.begin(), buffer.end()); // buffer zurücksetzen
}

LogMultiBuf::~LogMultiBuf() {
  sync();
}

LogMultiBuf::int_type LogMultiBuf::overflow(int_type ch) {
  if (Base::pbase() != Base::pptr()) {
    log.logString(std::string(Base::pbase(), std::distance(Base::pbase(), Base::pptr())));
    Base::setp(buffer.begin(), buffer.end()); // buffer zurücksetzen
  }

  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(ch);
  return ch;
}

int LogMultiBuf::sync() {
  overflow(Traits::eof());
  return 0;
}


LogMultiStream::LogMultiStream(const std::string &filenamePart) : std::ostream(nullptr) {
  rdbuf(new LogMultiBuf(filenamePart));
}

LogMultiStream::~LogMultiStream() {
  delete rdbuf();
}


int Trace::lev = 0;
bool Trace::traceOn = false;

}

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


#include "unixtime.h"
#include "converter.h"

#include <iostream>
#include <iomanip>


using namespace std;

namespace mobs {

UxTime::UxTime(int year, int month, int day, int hour, int minute, int second) {
  // TODO checks
//  if (year < 1900)
//    throw std::runtime_error(u8"UxTime: year below 1900");
  struct tm ts{};
  ts.tm_isdst = -1;
  ts.tm_year = year - 1900;
  ts.tm_mon = month-1;
  ts.tm_mday = day;
  ts.tm_hour = hour;
  ts.tm_min = minute;
  ts.tm_sec = second;
#ifdef __MINGW32__
  m_time = ::mktime(&ts);
#else
   ts.tm_gmtoff = 0;
   m_time = ::timelocal(&ts);
#endif
}

UxTime::UxTime(const std::string& s) {
  struct tm ts{};
  ts.tm_isdst = -1;
  const char *cp = s.c_str();
  parseYear(ts.tm_year, cp);
  parseChar('-', cp);
  parseInt2(ts.tm_mon, cp);
  ts.tm_mon -= 1;
  parseChar('-', cp);
  parseInt2(ts.tm_mday, cp);
  if (*cp == ' ')
    parseChar(' ', cp);
  else
    parseChar('T', cp);
  parseInt2(ts.tm_hour, cp);
  parseChar(':', cp);
  parseInt2(ts.tm_min, cp);
  parseChar(':', cp);
  parseInt2(ts.tm_sec, cp);
#ifndef __MINGW32__
    ts.tm_gmtoff = 0;
#endif

  if (*cp)
  {
    long off;
    parseOff(off, cp);
    if (*cp)
      throw std::runtime_error(u8"extra characters at end");
#ifdef __MINGW32__
      m_time = _mkgmtime(&ts) - off;
#else
      m_time = ::timegm(&ts) - off;
#endif
  }
  else
  {
    cerr << "---" << endl;
#ifdef __MINGW32__
      m_time = mktime(&ts);
#else
    m_time = ::timelocal(&ts);
#endif
  }
}


std::string UxTime::toISO8601() const {
  struct ::tm ts{};
  std::stringstream s;
  long gmtoff = 0;
#ifdef __MINGW32__
  localtime_s(&ts, &m_time);
  s << put_time(&ts, "%Y-%m-%dT%H:%M:%S");
  struct tm ptmgm;
  gmtime_s(&ptmgm, &m_time);
  time_t gm = mktime(&ptmgm);
  gmtoff = m_time -gm;
  if (ptmgm.tm_isdst > 0)
    gmtoff += 60 * 60;
#else
  ::localtime_r(&m_time, &ts);
  s << put_time(&ts, "%FT%T");
  gmtoff = ts.tm_gmtoff;
#endif

  // 2007-04-05T12:30:00+02:00

  if (ptmgm.tm_isdst >= 0)
    s << mobs::timeOffsetToStr(gmtoff);
  return s.str();
}



void UxTime::parseChar(char c, const char *&cp) {
  if (*cp != c)
    throw std::runtime_error(u8"missing " + std::to_string(c));
  cp++;
}
int UxTime::parseDigit(const char *&cp) {
  if (*cp > '9' or *cp < '0')
    throw std::runtime_error("no digit");
  return *cp++ - '0';
}
void UxTime::parseInt2(int &i, const char *&cp) {
  i = parseDigit(cp) * 10 + parseDigit(cp);
}
void UxTime::parseYear(int &y, const char *&cp) {
  int i = parseDigit(cp) * 1000 + parseDigit(cp) * 100 +
          parseDigit(cp) * 10 + parseDigit(cp);
  if (i < 1900)
    throw std::runtime_error(u8"year below 1900");
  y = i - 1900;
}
void UxTime::parseOff(long &i, const char *&cp) {
  i = 0;
  if (not *cp)
    return;
  int sign = 1;
  if (*cp == 'Z') {
    if (*++cp == 0)
      return;
    throw std::runtime_error("extra char after Z");
  }
  else if (*cp == '-')
    sign = -1;
  else if (*cp != '+')
    throw std::runtime_error("+/- expected");
  cp++;
  i = sign * (parseDigit(cp) * 10 + parseDigit(cp)) * 3600;
  if (not *cp)
    return;
  parseChar(':', cp);
  i += sign * (parseDigit(cp) * 10 + parseDigit(cp)) * 60;
  if (not *cp)
    return;
  parseChar(':', cp);
  i += sign * (parseDigit(cp) * 10 + parseDigit(cp));
}


/// Konvertierung UxTime nach std::string
template <> bool string2x(const std::string &str, UxTime &t)
{
  try {
    t = UxTime(str);
  }
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
  catch (std::runtime_error &x)
  {
    std::cerr << x.what() << std::endl;
    return false;
  }
#pragma clang diagnostic pop
  return true;
}


bool StrConv<UxTime>::c_string2x(const std::string &str, UxTime &t, const ConvFromStrHint &cfh) {
  if (cfh.acceptExtended()) {
    if (mobs::string2x(str, t))
      return true;
  }
  if (not cfh.acceptCompact())
    return false;
  int64_t i;
  if (not mobs::string2x(str, i))
    return false;
  return c_from_int(i, t);
}

bool StrConv<UxTime>::c_from_int(int64_t i, UxTime &t) {
  if (i < std::numeric_limits<time_t>::min() or i > std::numeric_limits<time_t>::max())
    return false;
  t = UxTime(time_t(i));
  return true;
}

}

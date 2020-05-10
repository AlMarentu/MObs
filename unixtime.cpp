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
#include "logging.h"


#include <sstream>
#include <iostream>
#include <iomanip>


using namespace std;

namespace mobs {

UxTime::UxTime(int year, int month, int day, int hour, int minute, int second) {
  // TODO checks
//  if (year < 1900)
//    throw std::runtime_error(u8"UxTime: year below 1900");
  struct tm ts;
  ts.tm_isdst = -1;
  ts.tm_year = year - 1900;
  ts.tm_mon = month-1;
  ts.tm_mday = day;
  ts.tm_hour = hour;
  ts.tm_min = minute;
  ts.tm_sec = second;
  ts.tm_gmtoff = 0;
  m_time = ::timelocal(&ts);
}

UxTime::UxTime(std::string s) {
    struct tm ts;
//    memset(&ts, 0, sizeof(struct tm));
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
    ts.tm_gmtoff = 0;

    if (*cp)
    {
      long off;
      parseOff(off, cp);
      if (*cp)
        throw std::runtime_error(u8"extra characters at end");
      m_time = ::timegm(&ts) + off;
    }
    else
    {
      cerr << "---" << endl;
      m_time = ::timelocal(&ts);
    }
  }


std::string UxTime::toISO8601() const {
  struct tm ts;
  ::localtime_r(&m_time, &ts);

  // 2007-04-05T12:30:00+02:00
  std::stringstream s;
  s << put_time(&ts, "%FT%T%z");
#if 0
  s << std::setfill('0') << std::setw(4) << ts.tm_year + 1900 << '-'
    << std::setw(2) << ts.tm_mon +1 << '-' << std::setw(2) << ts.tm_mday << 'T'
    << std::setw(2) << ts.tm_hour << ':' << std::setw(2) << ts.tm_min << ':' << std::setw(2) << ts.tm_sec
  << (ts.tm_gmtoff >= 0 ? '+': '-');
  long int off = abs(ts.tm_gmtoff);
  s << std::setw(2) << off / 3600;
  off = off % 3600;
  s << ':' << std::setw(2) << off % 60;
#endif
  string res = s.str();
  if (res.length() == 24)
    res.insert(22, u8":");
  return res;
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
  if (*cp == '-')
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
  catch (std::exception &x)
  {
    std::cerr << x.what() << std::endl;
    return false;
  }
  return true;
}

/// \private
template<> bool to_int64(UxTime t, int64_t &i, int64_t &min, uint64_t &max)
{
  i = t.toUxTime() * 1000;
  min = 0;
  max = std::numeric_limits<time_t>::max();
  return true;
}

/// \private
template<> bool from_number(int64_t i, UxTime &t)
{
  i /= 1000;
  if (i >= 0 or i < std::numeric_limits<time_t>::max())
  {
    t = UxTime(i);
    return true;
  }
  return false;
}



}

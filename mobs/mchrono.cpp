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


#include "mchrono.h"
#include "objtypes.h"
#include "logging.h"

#include <sstream>
#include <iomanip>
#include <ctime>

namespace {
class TimeHelper {
public:
  struct tm ts{};
  int micros = 0;

  // split Mtime into seconds and microseconds
  static void split(mobs::MTime t, time_t &s, int &us);

  size_t read(const std::string &s, mobs::MTime &t);


    private:
  static void parseChar(char c, const char *&cp);

  static int parseDigit(const char *&cp);

  static void parseInt2(int &i, const char *&cp);

  static void parseYear(int &y, const char *&cp);

  static int parseOff(const char *&cp);

  static int parseMicro(const char *&cp);
};

size_t TimeHelper::read(const std::string &s, mobs::MTime &t) {
  const char *cp = s.c_str();
  bool neg = false;
  while (*cp == ' ') cp++;
  if (*cp == '-') {
    neg = true;
    cp++;
  }
  parseYear(ts.tm_year, cp);
  if (neg)
    ts.tm_year *= -1;
  ts.tm_year -= 1900;
  parseChar('-', cp);
  parseInt2(ts.tm_mon, cp);
  ts.tm_mon -= 1;
  parseChar('-', cp);
  parseInt2(ts.tm_mday, cp);
  if (*cp) {
    if (*cp == ' ')
      parseChar(' ', cp);
    else
      parseChar('T', cp);
    parseInt2(ts.tm_hour, cp);
    parseChar(':', cp);
    parseInt2(ts.tm_min, cp);
    if (*cp == ':') {
      parseChar(':', cp);
      parseInt2(ts.tm_sec, cp);
      if (*cp == '.' or *cp == ',')
        micros = parseMicro(++cp);
    }
  }
  ts.tm_gmtoff = 0;
  ts.tm_isdst = -1;
  std::time_t ti;
  if (*cp == '-' or *cp == '+' or *cp == 'Z') {
    long off = parseOff(cp);
    ti = ::timegm(&ts);
    if (ti == -1)
      return 0;
    ti -= off;
  } else {
    ti = ::timelocal(&ts);
    if (ti == -1)
      return 0;
  }

  std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(ti);
  t = std::chrono::time_point_cast<std::chrono::microseconds>(tp);
  t += std::chrono::microseconds(micros);
  return cp - s.c_str();

}



void TimeHelper::parseChar(char c, const char *&cp) {
  if (*cp != c)
    throw std::runtime_error(u8"missing " + std::to_string(c));
  cp++;
}

int TimeHelper::parseDigit(const char *&cp) {
  if (*cp > '9' or *cp < '0')
    throw std::runtime_error("no digit");
  return *cp++ - '0';
}

int TimeHelper::parseMicro(const char *&cp) {
  int i = 0;
  for (int frac = 100000; frac > 0; frac /= 10) {
    i += frac * parseDigit(cp);
    if (not isdigit(*cp))
      break;
  }
  return i;
}

void TimeHelper::parseInt2(int &i, const char *&cp) {
  i = parseDigit(cp);
  if (isdigit(*cp))
    i = i * 10 + parseDigit(cp);
}

void TimeHelper::parseYear(int &y, const char *&cp) {
  int i = 0;
  parseInt2(i, cp);
  if (isdigit(*cp))
    i = i * 10 + parseDigit(cp);
  if (isdigit(*cp))
    i = i * 10 + parseDigit(cp);
  y = i;
}

int TimeHelper::parseOff(const char *&cp) {
  if (not *cp)
    return 0;
  int sign = 1;
  if (*cp == 'Z') {
    cp++;
    return 0;
  }
  else if (*cp == '-')
    sign = -1;
  else if (*cp != '+')
    throw std::runtime_error("+/- expected");
  cp++;
  int i = sign * (parseDigit(cp) * 10 + parseDigit(cp)) * 3600;
  if (*cp ==':')
    ++cp;
  else if (not isdigit(*cp))
    return i;
  i += sign * (parseDigit(cp) * 10 + parseDigit(cp)) * 60;
  if (*cp ==':')
    ++cp;
  else if (not isdigit(*cp))
    return i;
  i += sign * (parseDigit(cp) * 10 + parseDigit(cp));
  return i;
}

void TimeHelper::split(mobs::MTime t, time_t &s, int &us) {
  us = int(t.time_since_epoch().count() % mobs::MTime::period::den);
  s = std::chrono::system_clock::to_time_t(t);
  if (us < 0) {
    us += mobs::MTime::period::den;
    --s;
  }
}

}

namespace mobs {

// .time_since_epoch().count() liefert anzahl Ticks (MDays)
template<>
bool string2x(const std::string &str, MDate &t) {
  std::istringstream s(str);
  std::tm ts = {};
  s >> std::get_time(&ts, "%Y-%m-%d");
  if (s.fail())
    return false;
  ts.tm_isdst = -1;
  std::time_t ti = ::timelocal(&ts);
  std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(ti);
  t = std::chrono::time_point_cast<MDays>(tp);
//  i64 = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
  return true;
}

std::string to_string(MDate t) {
  std::stringstream s;
  struct tm ts{};
  time_t time = std::chrono::system_clock::to_time_t(t);
  ::localtime_r(&time, &ts);
  s << std::put_time(&ts, "%F");
  return s.str();
}

std::wstring StrConv<MDate>::c_to_wstring(const MDate &t, const ConvToStrHint &cth) {
  if (cth.compact())
    return std::to_wstring(t.time_since_epoch().count());
  return to_wstring(t);
}

std::string StrConv<MDate>::c_to_string(const MDate &t, const ConvToStrHint &cth) {
  if (cth.compact())
    return std::to_string(t.time_since_epoch().count());
  return to_string(t);
}

bool StrConv<MDate>::c_string2x(const std::string &str, MDate &t, const ConvFromStrHint &cfh) {
  if (cfh.acceptExtended()) {
    if (mobs::string2x(str, t))
      return true;
  }
  if (not cfh.acceptCompact())
    return false;
  int i;
  if (not mobs::string2x(str, i))
    return false;
  return c_from_int(i, t);
}

bool StrConv<MDate>::c_from_int(int64_t i, MDate &t) {
  std::chrono::system_clock::time_point tp{}; // sollte genau auf "epoch" stehen
  tp += MDays(i);
  t = std::chrono::time_point_cast<MDays>(tp);
  return true;
}

bool StrConv<MDate>::c_to_int64(MDate t, int64_t &i) {
  i = t.time_since_epoch().count();
  return true;
}

bool StrConv<MDate>::c_from_mtime(int64_t i, MDate &t) {
  std::chrono::system_clock::time_point tp{}; // sollte genau auf "epoch" stehen
  tp += std::chrono::microseconds(i);
  t = std::chrono::time_point_cast<MDays>(tp);
  return true;
}

bool StrConv<MDate>::c_to_mtime(MDate t, int64_t &i) {
  i = std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count();
  return true;
}


template<>
bool string2x(const std::string &str, MTime &t) {
  if (str.empty())
    return false;
  TimeHelper th;
  try {
    return th.read(str, t) == str.length();
  } catch (std::exception &e) {
    LOG(LM_DEBUG, "string2x " << e.what());
  }
  return false;
}




std::string to_string_iso8601(MTime t, MTimeFract f) {
  static const char *fmt[] = {"%Y", "%Y-%m", "%F", "%FT%H", "%FT%H:%M", "%FT%T", "%FT%T", "%FT%T", "%FT%T", "%FT%T", "%FT%T", "%FT%T"};
  std::stringstream s;
  struct tm ts{};
  time_t time;
  int us;
  TimeHelper::split(t, time, us);
  ::localtime_r(&time, &ts);
  s << std::put_time(&ts, fmt[f]);
  if (f >= MF1) {
    for (MTimeFract i = MF6; i > f; i = MTimeFract((int)i -1))
      us = us / 10;
    s << '.' << std::setfill('0') << std::setw(f - MSecond) << us;
  }
  if (f < MHour)
    return s.str();
  s << std::put_time(&ts, "%z");
  std::string res = s.str();
  res.insert(res.length() -2, u8":");
  return res;
}

std::string to_string_ansi(MTime t, MTimeFract f) {
  static const char *fmt[] = {"%Y", "%Y-%m", "%F", "%F %H", "%F %H:%M", "%F %T", "%F %T", "%F %T", "%F %T", "%F %T", "%F %T", "%F %T"};
  std::stringstream s;
  struct tm ts{};
  time_t time;
  int us;
  TimeHelper::split(t, time, us);
  ::localtime_r(&time, &ts);
  s << std::put_time(&ts, fmt[f]);
  if (f >= MF1) {
    for (MTimeFract i = MF6; i > f; i = MTimeFract((int)i -1))
      us = us / 10;
    s << '.' << std::setfill('0') << std::setw(f - MSecond) << us;
  }
  return s.str();
}

std::string to_string_gmt(MTime t, MTimeFract f) {
  static const char *fmt[] = {"%Y", "%Y-%m", "%F", "%FT%H", "%FT%H:%M", "%FT%T", "%FT%T", "%FT%T", "%FT%T", "%FT%T", "%FT%T", "%FT%T"};
  std::stringstream s;
  struct tm ts{};
  time_t time;
  int us;
  TimeHelper::split(t, time, us);
  ::gmtime_r(&time, &ts);
  s << std::put_time(&ts, fmt[f]);
  if (f >= MF1) {
    for (MTimeFract i = MF6; i > f; i = MTimeFract((int)i -1))
      us = us / 10;
    s << '.' << std::setfill('0') << std::setw(f - MSecond) << us;
  }
  if (f >= MHour)
    s << 'Z';
  return s.str();
}



std::string to_string(MTime t) {
  int i = int(t.time_since_epoch().count() % 1000000);
  MTimeFract f = MF6;
  while (f != MSecond and i % 10 == 0) {
    f = MTimeFract((int)f -1);
    i /= 10;
  }
  return to_string_iso8601(t, f);
}

std::wstring to_wstring(MTime t) {
  return to_wstring(to_string(t));
}

template<>
bool from_number(int64_t i, MTime &t) {
  return StrConv<MTime>::c_from_mtime(i, t);
}

template<>
bool to_int64(MTime t, int64_t &i) {
  i = t.time_since_epoch().count();
  return true;
}

std::wstring StrConv<MTime>::c_to_wstring(const MTime &t, const ConvToStrHint &cth) {
  if (cth.compact())
    return std::to_wstring(t.time_since_epoch().count());
  return to_wstring(t);
}

std::string StrConv<MTime>::c_to_string(const MTime &t, const ConvToStrHint &cth) {
  if (cth.compact())
    return std::to_string(t.time_since_epoch().count());
  return to_string(t);
}

bool StrConv<MTime>::c_string2x(const std::string &str, MTime &t, const ConvFromStrHint &cfh) {
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


bool StrConv<MTime>::c_from_mtime(int64_t i, MTime &t) {
  std::chrono::system_clock::time_point tp{}; // sollte genau auf "epoch" stehen
  tp += std::chrono::microseconds (i);
  t = std::chrono::time_point_cast<std::chrono::microseconds>(tp);
  return true;
}

bool StrConv<MTime>::c_to_mtime(MTime t, int64_t &i) {
  i = std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count();
  return true;
}



}
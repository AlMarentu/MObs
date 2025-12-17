// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2025 Matthias Lautner
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

#include "objgen.h"
#include "objtypes.h"
#include "mchrono.h"

#include <codecvt>
#include <locale>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <iomanip>
//#include <iostream>

using namespace std;


namespace mobs {

std::wstring to_wstring(const std::string &val) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> c;
  return c.from_bytes(val);
}

std::u32string to_u32string(std::string val) {
  std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> c;
  return c.from_bytes(val);
}

// alle möglichen Escapes in JSON (nach RFC 8259)
std::string to_quoteJson(const std::string &s) {
  string result;
  result.reserve(s.size() + 2);
  result = "\"";
  for (const auto c: s) {
    switch (c) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\b': // 010
        result += "\\b";
        break;
      case '\t': // 011
        result += "\\t";
        break;
      case '\n': // 012
        result += "\\n";
        break;
      case '\f': // 014
        result += "\\f";
        break;
      case '\r': // 015
        result += "\\r";
        break;
      case '\0': case '\1': case '\2': case '\3': case '\4': case '\5': case '\6': case '\7':
      case '\13': case '\16': case '\17':
      case '\20': case '\21': case '\22': case '\23': case '\24': case '\25': case '\26': case '\27':
      case '\30': case '\31': case '\32': case '\33': case '\34': case '\35': case '\36': case '\37':
        result += "\\u00";
        result += STRSTR(std::hex << std::setw(2) << std::setfill('0') << (int)c);
        break;
      default:
        result += c;
    }
  }
  result += '"';
  return result;
}


std::string to_quote(const std::string &s) {
  string result = "\"";
  if (not s.empty())
  {
    size_t pos = 0;
    size_t pos2 = 0;
    while ((pos = s.find_first_of(string("\"\\\0", 3), pos2)) != string::npos)
    {
      result += s.substr(pos2, pos - pos2) + "\\" + (s[pos] ? s[pos] : '0');
      pos2 = pos+1;
    }
    result += s.substr(pos2);
  }
  result += '"';
  return result;
}

std::string to_squote(const std::string &s) {
  string result = "'";
  if (not s.empty())
  {
    size_t pos = 0;
    size_t pos2 = 0;
    while ((pos = s.find_first_of(string("\'\\\0", 3), pos2)) != string::npos)
    {
      result += s.substr(pos2, pos - pos2) + "\\" + (s[pos] ? s[pos] : '0');
      pos2 = pos+1;
    }
    result += s.substr(pos2);
  }
  result += '\'';
  return result;
}

template<>
/// \private
bool string2x(const std::string &str, char32_t &t) {
  std::u32string s = mobs::to_u32string(str);
  if (s.length() > 1)
    return false;
  t = s.empty() ? U'\U00000000' : s[0];
  return true;
};

template<>
/// \private
bool string2x(const std::string &str, char &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if ((c & 0xffffff00))
    return false;
  t = static_cast<char>(c);
  return true;
}

template<>
/// \private
bool string2x(const std::string &str, signed char &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if ((c & 0xffffff00))
    return false;
  t = static_cast<signed char>(c);
  return true;
}

template<>
/// \private
bool string2x(const std::string &str, unsigned char &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if ((c & 0xffffff00))
    return false;
  t = static_cast<unsigned char>(c);
  return true;
}

template<>
/// \private
bool string2x(const std::string &str, char16_t &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if ((c & 0xffff0000))
    return false;
  t = static_cast<char16_t>(c);
  return true;
}

template<>
/// \private
bool string2x(const std::string &str, wchar_t &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if (numeric_limits<wchar_t>::digits <= 16 and (c & 0xff0000))
    return false;
  t = c;
  return true;
}

template<>
/// \private
bool string2x(const std::string &str, bool &t) {
  if (str == "true")
    t = true;
  else if (str == "false")
    t = false;
  else
    return false;
  return true;
}

template<>
bool string2x(const std::string &str, u32string &t) {
  std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> c;
  t = c.from_bytes(str);
  return true;
}

template<>
bool string2x(const std::string &str, u16string &t) {
  std::wstring_convert<std::codecvt_utf8<char16_t>,char16_t> c;
  t = c.from_bytes(str);
  return true;
}

template<>
bool string2x(const std::string &str, wstring &t) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> c;
  t = c.from_bytes(str);
  return true;
}


template <>
/// \private
bool wstring2x(const std::wstring &wstr, std::u32string &t) {
  std::copy(wstr.cbegin(), wstr.cend(), back_inserter(t));
  return true;
}

template <>
/// \private
bool wstring2x(const std::wstring &wstr, std::u16string &t) {
  std::copy(wstr.cbegin(), wstr.cend(), back_inserter(t));
  return true;
}


std::string to_string(std::u32string t) {
  std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> c;
  return c.to_bytes(t);
}

std::string to_string(std::u16string t) {
  std::wstring_convert<std::codecvt_utf8<char16_t>,char16_t> c;
  return c.to_bytes(t);
}

std::string to_string(std::wstring t) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> c;
  return c.to_bytes(t);
}

std::string to_string(float t) {
  std::stringstream str;
  str << t;
  return str.str();
}

std::string to_string(double t ){
  std::stringstream str;
  str << t;
  return str.str();
}

std::string to_string(long double t) {
  std::stringstream str;
  str << t;
  return str.str();
}

std::wstring to_wstring(float t) {
  std::wstringstream str;
  str << t;
  return str.str();
}

std::wstring to_wstring(double t ){
  std::wstringstream str;
  str << t;
  return str.str();
}

std::wstring to_wstring(long double t) {
  std::wstringstream str;
  str << t;
  return str.str();
}

std::wstring to_wstring(const std::u32string &t) {
  wstring result;
  std::transform(t.cbegin(), t.cend(), std::back_inserter(result),
  [](const wchar_t c) { return (c < 0 or c > 0x10FFFF) ? L'\uFFFD' : c; });
//  copy(t.cbegin(), t.cend(), back_inserter(result));
  return result;
}

std::wstring to_wstring(const std::u16string &t) {
  wstring result;
  std::transform(t.cbegin(), t.cend(), std::back_inserter(result),
  [](const char16_t c) -> wchar_t{ return (c < 0) ? L'\uFFFD' : c; });
//  copy(t.cbegin(), t.cend(), back_inserter(result));
  return result;
}

std::string to_string(const std::vector<u_char> &buf) {
  std::stringstream x;
  for (auto c:buf)
    x << std::hex << std::setfill('0') << std::setw(2) << u_int(c);
  return x.str();
}



template<>
/// \private
bool to_int64(int t, int64_t &i)
{ i = t; return true; }
template<>
/// \private
bool to_int64(short int t, int64_t &i)
{ i = t; return true; }
template<>
/// \private
bool to_int64(long int t, int64_t &i)
{ i = t; return true; }
template<>
/// \private
bool to_int64(long long int t, int64_t &i)
{ i = t; return true; }
template<>
/// \private
bool to_uint64(unsigned int t, uint64_t &u)
{ u = t; return true; }
template<>
/// \private
bool to_uint64(unsigned short int t, uint64_t &u)
{ u = t; return true; }
template<>
/// \private
bool to_uint64(unsigned long int t, uint64_t &u)
{ u = t;  return true; }
template<>
/// \private
bool to_uint64(unsigned long long int t, uint64_t &u)
{ u = t; return true; }

template<>
/// \private
bool from_number(int64_t i, int &t) {
  if (i > std::numeric_limits<int>::max() or i < std::numeric_limits<int>::min()) return false;
  t = ( int)i;
  return true;
}
template<>
/// \private
bool from_number(int64_t i, short int &t) {
  if (i > std::numeric_limits<short int>::max() or i < std::numeric_limits<short int>::min()) return false;
  t = (short int)i;
  return true;
}
template<>
/// \private
bool from_number(int64_t i, long int &t) {
  if (i > std::numeric_limits<long int>::max() or i < std::numeric_limits<long int>::min()) return false;
  t = (long int)i;
  return true;
}
template<>
/// \private
bool from_number(int64_t i, long long int &t) {
  if (i > std::numeric_limits<long long int>::max() or i < std::numeric_limits<long long int>::min()) return false;
  t = (long long int)i;
  return true;
}
template<>
/// \private
bool from_number(uint64_t u, unsigned int &t) {
  if (u > std::numeric_limits<unsigned int>::max()) return false;
  t = (unsigned int)u;
  return true;
}
template<>
/// \private
bool from_number(uint64_t u, unsigned short int &t) {
  if (u > std::numeric_limits<unsigned short int>::max()) return false;
  t = (unsigned short int)u;
  return true;
}
template<>
/// \private
bool from_number(uint64_t u, unsigned long int &t) {
  if (u > std::numeric_limits<unsigned long int>::max()) return false;
  t = (unsigned long int)u;
  return true;
}
template<>
/// \private
bool from_number(uint64_t u, unsigned long long int &t) {
  if (u > std::numeric_limits<unsigned long long int>::max()) return false;
  t = (unsigned long long int)u;
  return true;
}
template<>
/// \private
bool from_number(uint64_t u, bool &t) {
  if (u > 1) return false;
  t = u != 0;
  return true;
}

std::string MobsMemberInfoDb::toString(bool *needQuotes) const {
  if (needQuotes)
    *needQuotes = false;
  if (isFloat)
    return std::to_string(d);
  if (isTime) {
    if (needQuotes)
      *needQuotes = true;
    MTime t;
    if (not from_number(t64, t))
      throw std::runtime_error("Time Conversion");
    MTimeFract f = mobs::MSecond;
    if (granularity < 10)
      f = mobs::MF6;
    else if (granularity < 100)
      f = mobs::MF5;
    else if (granularity < 1000)
      f = mobs::MF4;
    else if (granularity < 10000)
      f = mobs::MF3;
    else if (granularity < 100000)
      f = mobs::MF2;
    else if (granularity < 1000000)
      f = mobs::MF1;
    else if (granularity < 3600000000)
      f = mobs::MHour;
    else if (granularity < 86400000000)
      f = mobs::MDay;
    return to_string_ansi(t, f);
  }
  if (isUnsigned and max == 1) // bool
    return (u64 ? "true" : "false");
  if (isSigned)
    return std::to_string(i64);
  if (isUnsigned)
    return std::to_string(u64);
  if (needQuotes)
    *needQuotes = true;
  return text;
}


class ConvFromStrHintDefault : virtual public ConvFromStrHint {
public:
  ConvFromStrHintDefault() = default;
  ~ConvFromStrHintDefault() override = default;
  bool acceptCompact() const override { return true; }
  bool acceptExtended() const override { return true; }
};

class ConvFromStrHintExplizit : virtual public ConvFromStrHint {
public:
  ConvFromStrHintExplizit() = default;
  ~ConvFromStrHintExplizit() override = default;
  bool acceptCompact() const override { return false; }
  bool acceptExtended() const override { return true; }
};

const ConvFromStrHint &ConvFromStrHint::convFromStrHintDflt = ConvFromStrHintDefault();
const ConvFromStrHint &ConvFromStrHint::convFromStrHintExplizit = ConvFromStrHintExplizit();

void MobsMemberInfo::toLocalTime(struct ::tm &ts) const {
  if (granularity >= 86400000000 ) {
    toGMTime(ts);
    return;
  }
  std::chrono::system_clock::time_point tp{};
  tp += std::chrono::microseconds(t64);
  time_t time = std::chrono::system_clock::to_time_t(tp);
#ifdef __MINGW32__
  localtime_s(&ts, &time);
#else
  localtime_r(&time, &ts);
#endif
  if (granularity >= 3600000000)
    ts.tm_min = 0;
  if (granularity >= 60000000)
    ts.tm_sec = 0;
}

void MobsMemberInfo::toGMTime(struct ::tm &ts) const {
  std::chrono::system_clock::time_point tp{};
  tp += std::chrono::microseconds(t64);
  time_t time = std::chrono::system_clock::to_time_t(tp);
  // Bei Windows und Apple ist gmtime nicht für Zeiten vor 1970 geeignet
#if defined(__MINGW32__) || defined(__APPLE__)
  int yOfs = 0;
  if (time < 0 and time >= -8488800000) { // ab 1.1.1701
    const int64_t f = 883612800; // 365,25*24*3600*28   28 Jahre wegen Wochentag und Schaltjahr
    //Problem bei 1700 1800 1900
    int x = (f-1-time) / f;
    yOfs = x * 28;
    if (time < -5359564800) // 1.3.1800
      time -= 86400;
    if (time < -2203891200) // 1.3.1900
        time -= 86400;
    time += x * f;
    int y = 1970 - yOfs;
  }
#ifdef __MINGW32__
  gmtime_s(&ts, &time);
#else
  gmtime_r(&time, &ts);
#endif
  ts.tm_year -= yOfs;
  if (ts.tm_year == 0 and ts.tm_mon > 1) // 1900 ist kein Schaltjahr
    ts.tm_yday--;
  else if (ts.tm_year <= 0)
    ts.tm_wday = (ts.tm_wday +1) % 7;
  if (ts.tm_year == -100 and ts.tm_mon > 1) // 1800 ist kein Schaltjahr
    ts.tm_yday--;
  else if (ts.tm_year <= -100)
    ts.tm_wday = (ts.tm_wday +1) % 7;
#else
  gmtime_r(&time, &ts);
#endif
  if (granularity >= 86400000000) {
    ts.tm_hour = 0;
    ts.tm_isdst = -1;
  }
  if (granularity >= 3600000000)
    ts.tm_min = 0;
  if (granularity >= 60000000)
    ts.tm_sec = 0;
}

void MobsMemberInfo::fromLocalTime(tm &ts) {
  if (granularity >= 86400000000) {
    fromGMTime(ts);
    return;
  }
  ts.tm_isdst = -1;
  if (granularity >= 3600000000)
    ts.tm_min = 0;
  if (granularity >= 60000000)
    ts.tm_sec = 0;
#ifdef __MINGW32__
  std::time_t t = mktime(&ts); // TODO Timezone korrekt?
#else
  std::time_t t = timelocal(&ts);
#endif
  std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(t);
  setTime(std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count());
}

void MobsMemberInfo::fromGMTime(tm &ts) {
  if (granularity >= 86400000000)
    ts.tm_hour = 0;
  if (granularity >= 3600000000)
    ts.tm_min = 0;
  if (granularity >= 60000000)
    ts.tm_sec = 0;
  // Bei Windows und Apple ist mktime nicht für Zeiten vor 1970 geeignet
#if defined(__MINGW32__) || defined(__APPLE__)
  int64_t mOfs = 0;
  if (ts.tm_year < 70 and ts.tm_year > -200) { // ab 1701
    //kein Schaltjahr bei 1700 1800 1900
    const int64_t f = 883612800; // 365,25*24*3600*28   28 Jahre wegen Wochentag und Schaltjahr
    int x = (27+70-ts.tm_year) / 28;
    ts.tm_year += x * 28;
    mOfs = x * f;
  }
#ifdef __MINGW32__
  time_t t = _mkgmtime(&ts) - mOfs;
#else
    time_t t = timegm(&ts) - mOfs;
#endif
  if (t < -2203891200) // 1.3.1900
    t += 86400;
  if (t < -5359564800) // 1.3.1800
    t += 86400;
#else
  std::time_t t = timegm(&ts);
#endif
  std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(t);
  setTime(std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count());
}

void MobsMemberInfo::setTime(int64_t t) {
  t64 = t;
  isSigned = false;
  isUnsigned = false;
  isFloat = false;
  // isTime muss explizit gesetzt werden;
}

void MobsMemberInfo::setInt(int64_t t) {
  if (isFloat) {
    d = double(t);
    return;
  }
  if (isSigned) {
    if ((t >= 0 and uint64_t(t) > max) or t < min)
      throw runtime_error(u8"MobsMemberInfo int out of range");
    i64 = t;
  } else if (isUnsigned) {
    if (t < 0 or uint64_t(t) > max or t < min)
      throw runtime_error(u8"MobsMemberInfo int out of range");
    u64 = uint64_t(t);
  }
  else
     throw runtime_error(u8"MobsMemberInfo no int");
}

void MobsMemberInfo::setUInt(uint64_t t) {
  if (isFloat) {
    d = double(t);
    return;
  }
  if (t > max)
    throw runtime_error(u8"MobsMemberInfo uint out of range");
  if (isSigned)
    i64 = int64_t(t);
  else if (isUnsigned)
    u64 = t;
  else
    throw runtime_error(u8"MobsMemberInfo no int");
}

void MobsMemberInfo::setBool(bool t) {
  if (isSigned)
    i64 = t ? 1 : 0;
  else if (isUnsigned)
    u64 = t ? 1 : 0;
  else
    throw runtime_error(u8"MobsMemberInfo no bool");
}

void MobsMemberInfo::changeCompact(bool compact) {
  if (not hasCompact)
    return;
  if (compact) {
    isTime = false;
    is_specialized = true;
  } else {
    is_specialized = false;
    isUnsigned = false;
    isSigned = false;
  }
}

bool MobsMemberInfo::isNumber() const {
  return isSigned or isUnsigned or isTime or isFloat;
}


bool StrConv<float>::c_string2x(const std::string &str, float &t, const ConvFromStrHint &) {
  return mobs::string2x(str, t);
}

bool StrConv<float>::c_wstring2x(const std::wstring &wstr, float &t, const ConvFromStrHint &cth) {
  return c_string2x(mobs::to_string(wstr), t, cth);
}

bool StrConv<double>::c_wstring2x(const std::wstring &wstr, double &t, const ConvFromStrHint &cth) {
  return c_string2x(mobs::to_string(wstr), t, cth);
}

bool StrConv<double>::c_string2x(const std::string &str, double &t, const ConvFromStrHint &) {
  return mobs::string2x(str, t);
}

}

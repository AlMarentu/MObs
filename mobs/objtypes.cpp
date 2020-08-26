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

#include "objgen.h"
#include "objtypes.h"

#include <codecvt>
#include <locale>
#include <algorithm>
#include <ctime>
#include <chrono>

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

std::string to_quote(const std::string &s) {
  string result = "\"";
  if (s.length() > 1 or s[0] != 0)
  {
    size_t pos = 0;
    size_t pos2 = 0;
    while ((pos = s.find_first_of("\"\\", pos2)) != string::npos)
    {
      result += s.substr(pos2, pos - pos2) + "\\" + s[pos];
      pos2 = pos+1;
    }
    result += s.substr(pos2);
  }
  result += '"';
  return result;
}

std::string to_squote(const std::string &s) {
  string result = "'";
  if (s.length() > 1 or s[0] != 0)
  {
    size_t pos = 0;
    size_t pos2 = 0;
    while ((pos = s.find_first_of("\'\\", pos2)) != string::npos)
    {
      result += s.substr(pos2, pos - pos2) + "\\" + s[pos];
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
  t = c;
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
  t = c;
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
  t = c;
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
  t = c;
  return true;
}

template<>
/// \private
bool string2x(const std::string &str, wchar_t &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if (numeric_limits<wchar_t>::digits <= 16 and (c & 0xffff00))
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


class ConvFromStrHintDefault : virtual public ConvFromStrHint {
public:
  ConvFromStrHintDefault() {}
  virtual ~ConvFromStrHintDefault() {}
  virtual bool acceptCompact() const { return true; }
  virtual bool acceptExtended() const { return true; }
};

class ConvFromStrHintExplizit : virtual public ConvFromStrHint {
public:
  ConvFromStrHintExplizit() {}
  virtual ~ConvFromStrHintExplizit() {}
  virtual bool acceptCompact() const { return false; }
  virtual bool acceptExtended() const { return true; }
};

const ConvFromStrHint &ConvFromStrHint::convFromStrHintDflt = ConvFromStrHintDefault();
const ConvFromStrHint &ConvFromStrHint::convFromStrHintExplizit = ConvFromStrHintExplizit();

void MobsMemberInfo::toLocalTime(struct ::tm &ts) const {
  std::chrono::system_clock::time_point tp{};
  tp += std::chrono::microseconds(t64);
  time_t time = std::chrono::system_clock::to_time_t(tp);
  ::localtime_r(&time, &ts);
}

void MobsMemberInfo::toGMTime(struct ::tm &ts) const {
  std::chrono::system_clock::time_point tp{};
  tp += std::chrono::microseconds(t64);
  time_t time = std::chrono::system_clock::to_time_t(tp);
  ::gmtime_r(&time, &ts);
}

void MobsMemberInfo::fromLocalTime(tm &ts) {
  ts.tm_isdst = -1;
  std::time_t t = ::timelocal(&ts);
  std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(t);
  setTime(std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count());
}

void MobsMemberInfo::fromGMTime(tm &ts) {
  std::time_t t = ::timegm(&ts);
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


}

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
//#include <iostream>

using namespace std;


namespace mobs {


std::string to_quote(const std::string &s) {
  string result = "\"";
  if (s.length() > 1 or s[0] != 0)
  {
    size_t pos = 0;
    size_t pos2 = 0;
    while ((pos = s.find('"', pos2)) != string::npos)
    {
      result += s.substr(pos2, pos - pos2) + "\\\"";
      pos2 = pos+1;
    }
    result += s.substr(pos2);
  }
  result += '"';
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
};

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
};

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
};

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
};

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
};

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
bool wstring2x(const std::wstring &wstr, std::u32string &t) {
  std::copy(wstr.cbegin(), wstr.cend(), back_inserter(t));
  return true;
}

template <>
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



template<>
bool to_int64(int t, int64_t &i, int64_t &min, uint64_t &max)
{ i = t; min = std::numeric_limits<int>::min(); max = std::numeric_limits<int>::max(); return true; }
template<>
bool to_int64(short int t, int64_t &i, int64_t &min, uint64_t &max)
{ i = t; min = std::numeric_limits<short int>::min(); max = std::numeric_limits<short int>::max(); return true; }
template<>
bool to_int64(long int t, int64_t &i, int64_t &min, uint64_t &max)
{ i = t; min = std::numeric_limits<long int>::min(); max = std::numeric_limits<long int>::max(); return true; }
template<>
bool to_int64(long long int t, int64_t &i, int64_t &min, uint64_t &max)
{ i = t; min = std::numeric_limits<long long int>::min(); max = std::numeric_limits<long long int>::max(); return true; }
template<>
bool to_uint64(unsigned int t, uint64_t &u, uint64_t &max)
{ u = t; max = std::numeric_limits<unsigned int>::max(); return true; }
template<>
bool to_uint64(unsigned short int t, uint64_t &u, uint64_t &max)
{ u = t; max = std::numeric_limits<unsigned short int>::max(); return true; }
template<>
bool to_uint64(unsigned long int t, uint64_t &u, uint64_t &max)
{ u = t; max = std::numeric_limits<unsigned long int>::max(); return true; }
template<>
bool to_uint64(unsigned long long int t, uint64_t &u, uint64_t &max)
{ u = t; max = std::numeric_limits<unsigned long long int>::max(); return true; }
template<>
bool to_uint64(bool t, uint64_t &u, uint64_t &max)
{ u = t ? 1:0; max = 1; return true; }
template<>
bool to_double(double t, double &d) { d = t; return true; }
template<>
bool to_double(float t, double &d) { d = t; return true; }
//template<>
//bool to_double(long double t, double &d) { d = t; return true; }

template<>
bool from_number(int64_t i, int &t) {
  if (i > std::numeric_limits<int>::max() or i < std::numeric_limits<int>::min()) return false;
  t = ( int)i;
  return true;
}
template<>
bool from_number(int64_t i, short int &t) {
  if (i > std::numeric_limits<short int>::max() or i < std::numeric_limits<short int>::min()) return false;
  t = (short int)i;
  return true;
}
template<>
bool from_number(int64_t i, long int &t) {
  if (i > std::numeric_limits<long int>::max() or i < std::numeric_limits<long int>::min()) return false;
  t = (long int)i;
  return true;
}
template<>
bool from_number(int64_t i, long long int &t) {
  if (i > std::numeric_limits<long long int>::max() or i < std::numeric_limits<long long int>::min()) return false;
  t = (long long int)i;
  return true;
}
template<>
bool from_number(uint64_t u, unsigned int &t) {
  if (u > std::numeric_limits<unsigned int>::max()) return false;
  t = (unsigned int)u;
  return true;
}
template<>
bool from_number(uint64_t u, unsigned short int &t) {
  if (u > std::numeric_limits<unsigned short int>::max()) return false;
  t = (unsigned short int)u;
  return true;
}
template<>
bool from_number(uint64_t u, unsigned long int &t) {
  if (u > std::numeric_limits<unsigned long int>::max()) return false;
  t = (unsigned long int)u;
  return true;
}
template<>
bool from_number(uint64_t u, unsigned long long int &t) {
  if (u > std::numeric_limits<unsigned long long int>::max()) return false;
  t = (unsigned long long int)u;
  return true;
}
template<>
bool from_number(uint64_t u, bool &t) {
  if (u > 1) return false;
  t = u != 0;
  return true;
}
template<>
bool from_number(double d, float &t) { t = d; return true; }
template<>
bool from_number(double d, double &t) { t = d; return true; }



class ConvFromStrHintDefault : virtual public ConvFromStrHint {
public:
  ConvFromStrHintDefault() {}
  virtual ~ConvFromStrHintDefault() {}
  virtual bool acceptCompact() const { return true; }
  virtual bool acceptExtented() const { return true; }
};

class ConvFromStrHintExplizit : virtual public ConvFromStrHint {
public:
  ConvFromStrHintExplizit() {}
  virtual ~ConvFromStrHintExplizit() {}
  virtual bool acceptCompact() const { return false; }
  virtual bool acceptExtented() const { return true; }
};

const ConvFromStrHint &ConvFromStrHint::convFromStrHintDflt = ConvFromStrHintDefault();
const ConvFromStrHint &ConvFromStrHint::convFromStrHintExplizit = ConvFromStrHintExplizit();


}

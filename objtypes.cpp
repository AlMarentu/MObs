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

#if 0
string to_string(const ObjectBase &obj) {
  
  class ObjDump : virtual public ObjTravConst {
  public:
    virtual void doObjBeg(ObjTravConst &ot, const ObjectBase &obj)
    {
      if (not fst)
        res << ",";
      fst = true;
      if (not obj.name().empty())
        res << obj.name() << ":";
      if (obj.isNull())
        res << "null";
      else
        res << "{";
    };
    virtual void doObjEnd(ObjTravConst &ot, const ObjectBase &obj)
    {
      if (not obj.isNull())
        res << "}";
      fst = false;
    };
    virtual void doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec)
    {
      if (not fst)
        res << ",";
      fst = false;
      res << vec.name() << ":[";
    };
    virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec)
    {
      res << "]";
      fst = false;
    };
    virtual void doMem(ObjTravConst &ot, const MemberBase &mem)
    {
      if (not fst)
        res << ",";
      fst = false;
      res << boolalpha << mem.name() << ":";
      if (mem.isNull())
        res << "null";
      else if (mem.is_chartype())
      {
        res << '"';
        string s = mem.toStr();
        if (s.length() > 1 and s[0] != 0)
        {
          size_t pos = 0;
          size_t pos2 = 0;
          while ((pos = s.find('"', pos2)) != string::npos)
          {
            res << s.substr(pos2, pos - pos2) << "\\\"";
            pos2 = pos+1;
          }
          res << s.substr(pos2);
        }
        res << '"';

      }
      else
        mem.strOut(res);
      
    };
    std::string result() const { return res.str(); };
  private:
    bool fst = true;
    stringstream res;
  };
  
  ObjDump od;
  obj.traverse(od);
  return od.result();
}
#endif

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
};

template<>
bool string2x(const std::string &str, u32string &t) {
  std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> c;
  t = c.from_bytes(str);
  return true;
};

template<>
bool string2x(const std::string &str, u16string &t) {
  std::wstring_convert<std::codecvt_utf8<char16_t>,char16_t> c;
  t = c.from_bytes(str);
  return true;
};

template<>
bool string2x(const std::string &str, wstring &t) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> c;
  t = c.from_bytes(str);
  return true;
};


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



}

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

#if 1 // USERAPIJSON

#include "readjson.h"
#include "logging.h"

#include<stack>
#include<limits.h>
#include<iostream>

#define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseValidateEncodingFlag | kParseNanAndInfFlag | kParseEscapedApostropheFlag
#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/filereadstream.h"
#include "rapidjson/reader.h"
#include "rapidjson/error/en.h"


using namespace std;
using namespace rapidjson;

namespace mobs {

class JsonReadData : public ObjectNavigator, public BaseReaderHandler<UTF8<>, JsonReadData>  {
public:
  JsonReadData(const string &input) : json(input), my_count(0), my_errors(0) { };
  ~JsonReadData() { };
  
  bool Null() {
    TRACE("");
    if (enter(lastKey) and member())
    {
      if (member()->nullAllowed())
        member()->setNull(true);
    }
    leave();
    return true;
  }
  bool Bool(bool b) {
    TRACE(PARAM(b));
    if (enter(lastKey) and member())
    {
      auto mbool = dynamic_cast<MemVarType(bool) *>(member());
      if (mbool)
        mbool->operator()(b);
    }
    leave();
    return true;
  }
  bool Int(int i) {
    TRACE(PARAM(i));
    if (enter(lastKey) and member())
    {
      auto mint = dynamic_cast<MemVarType(int) *>(member());
      auto mdouble = dynamic_cast<MemVarType(double) *>(member());
      auto mbool = dynamic_cast<MemVarType(bool) *>(member());
      auto mstring = dynamic_cast<MemVarType(string) *>(member());
      if (mint)
      {
        cerr << "MINT" << endl;
        mint->operator()(int(i));
        leave();
        return true;
      }
      if (mstring)
      {
        cerr << "MSTRING" << endl;
        mstring->operator()(to_string(i));
      }
    }
    leave();
    return true;
  }
  bool Uint(unsigned u) {
    TRACE(PARAM(u));
    if (enter(lastKey) and member())
    {
      auto mint = dynamic_cast<MemVarType(int) *>(member());
      auto mdouble = dynamic_cast<MemVarType(double) *>(member());
      auto mbool = dynamic_cast<MemVarType(bool) *>(member());
      auto mstring = dynamic_cast<MemVarType(string) *>(member());
      if (mint)
      {
        cerr << "MINT" << endl;
        if (u > INT_MAX)
          throw runtime_error("INT_Overflow");
        mint->operator()(int(u));
      }
      if (mstring)
      {
        cerr << "MSTRING" << endl;
        mstring->operator()(to_string(u));
      }
    }
    leave();
    return true;
  }
  bool Int64(int64_t i) {
    TRACE(PARAM(i));
    if (enter(lastKey) and member())
    {
      cout << "Int64(" << i << ")" << endl;
    }
    leave();
    return true;
  }
  bool Uint64(uint64_t u) {
    TRACE(PARAM(u));
    if (enter(lastKey) and member())
    {
      cout << "Uint64(" << u << ")" << endl;
    }
    leave();
    return true;
  }
  bool Double(double d) {
    TRACE(PARAM(d));
    if (enter(lastKey) and member())
    {
      cout << "Double(" << d << ")" << endl;
    }
    leave();
    return true;
  }
  bool RawNumber(const char* str, SizeType length, bool copy) {
    TRACE(PARAM(str) << PARAM(length) << PARAM(copy));
    throw runtime_error(u8"RapidJson keine String-Zahlen");
    return true;
  }
  bool String(const char* str, SizeType length, bool copy) {
    TRACE(PARAM(str) << PARAM(length) << PARAM(copy));
    if (enter(lastKey) and member())
    {
//      auto mint = dynamic_cast<MemVarType(int) *>(member());
//      auto mdouble = dynamic_cast<MemVarType(double) *>(member());
//      auto mbool = dynamic_cast<MemVarType(bool) *>(member());
      auto mstring = dynamic_cast<MemVarType(string) *>(member());
      
      if (mstring)
        mstring->operator()(string(str, length));
      else
        throw runtime_error(u8"JSON: string statt Zahl in Variable " + member()->name());
    }
    leave();
    return true;
  }
  
  
  bool StartObject() {
    TRACE(PARAM(lastKey));
    LOG(LM_INFO, "Start " << lastKey << " " << level);
    if (++level > 1)
      enter(lastKey);
    return true;
  }
  bool Key(const char* str, SizeType length, bool copy) {
    lastKey = string(str, length);
    return true;
  }
  bool EndObject(SizeType memberCount) {
    TRACE(PARAM(memberCount));
    lastKey = current();
    LOG(LM_INFO, "Ende " << lastKey);
    if (level-- > 1)
      leave();
    return true;
  }
  bool StartArray() {
    TRACE("");
    return true;
  }
  bool EndArray(SizeType elementCount) {
    TRACE(PARAM(elementCount));
    return true;
  }
  
  const string &json;
  int my_count;
  int my_errors;
private:
  int level = 0;
  string lastKey;
  string prefix;
};



void JsonRead::parse()
{
  TRACE("");
  StringStream s(data->json.c_str());
  Reader reader;
  ParseResult ok = reader.Parse(s, *data);
  if (not ok)
    throw runtime_error(string("Parsing failed ") + GetParseError_En(ok.Code()) + " at pos. " + to_string(ok.Offset()));
}

JsonRead::JsonRead(const string &input)
{
  TRACE("");
  data = new JsonReadData(input);
}

JsonRead::~JsonRead()
{
  TRACE("");
  delete data;
}

void JsonRead::fill(ObjectBase &obj)
{
  TRACE("");
  data->pushObject(obj);
  parse();
}

}

#endif

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
#include "jsonparser.h"
#include "xmlparser.h"
#include <map>

using namespace std;

namespace mobs {


void string2Obj(const std::string &str, ObjectBase &obj, ConvObjFromStr cfh)
{
  class JsonReadData : public ObjectNavigator, public JsonParser  {
  public:
    JsonReadData(const string &input, const ConvObjFromStr &c) : JsonParser(input) { cfs = c; }
    
    void Value(const string &val, bool charType) {
      TRACE(PARAM(val));
      if (enter(lastKey, currentIdx))
      {
        if (not charType and val == "null")
          setNull();
        else if (not member())
          throw runtime_error(u8"string2Obj: " + showName() + " is no variable, can't assign");
        else if (not member()->fromStr(val, cfs))
          throw runtime_error(u8"string2Obj: invalid type in variable " + showName() + " can't assign");
      }
      if (currentIdx != SIZE_T_MAX)
        currentIdx++;
      leave();
    }
    void StartObject() {
      TRACE(PARAM(lastKey));
      index.push(currentIdx);
      currentIdx = SIZE_T_MAX;
      if (++level > 1)
        enter(lastKey);
    }
    void Key(const string &key) {
      lastKey = key;
    }
    void EndObject() {
      TRACE("");
      lastKey = current();
      if (level-- > 1)
        leave();
      if (index.empty())
        throw runtime_error(u8"string2Obj: Structure invalid");
      currentIdx = index.top();
      index.pop();
    }
    void StartArray() {
      TRACE("");
      currentIdx = 0;
    }
    void EndArray() {
      TRACE("");
      currentIdx = SIZE_T_MAX;
    }
    
  private:
    int level = 0;
    size_t currentIdx = SIZE_T_MAX;
    string lastKey;
    stack<size_t> index;
  };
  
  
#if 0
  class XmlReadData : public ObjectNavigator, public XmlParser  {
  public:
    XmlReadData(const std::string &input, const ConvObjFromStr &c) : XmlParser(input) { cfs = c; };
      
    void NullTag(const std::string &element) {
      TRACE(PARAM(element));
      setNull();
      if (tagPath().size() > 1)
        leave();
    };
    void Attribut(const std::string &element, const std::string &attribut, const std::string &value) {
      TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
      LOG(LM_INFO, "string2Obj: ignoring attribute " << element << ":" << attribut << " = " << value);
    };
    void Value(const std::string &val) {
      TRACE(PARAM(val));
      if (not member())
        throw runtime_error(u8"string2Obj: " + showName() + " is no variable, can't assign");
      else if (not member()->fromStr(val, cfs))
        throw runtime_error(u8"string2Obj: invalid type in variable " + showName() + " can't assign");
    };
    void Cdata(const char *value, size_t len) { Value(std::string(value, len)); }

    void StartTag(const std::string &element) {
      TRACE(PARAM(element));
      if (tagPath().size() <= 1)
        return;
      if (not enter(element))
        LOG(LM_INFO, element << " wurde nicht gefunden");
    }
    void EndTag(const std::string &element) {
      TRACE(PARAM(element));
      if (tagPath().size() > 1)
        leave(element);
    }
    void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::string &value) {
      TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
      if (element == "xml" and attribut == "encoding")
        encoding = value;
    }
        
    std::string encoding;
    std::string prefix;
  };
    
#else
  class XmlReadData : public ObjectNavigator, public XmlParserW  {
  public:
    XmlReadData(const std::string &input, const ConvObjFromStr &c) : XmlParserW(str), str(to_wstring(input)) { cfs = c; };
      
    void NullTag(const std::string &element) {
      TRACE(PARAM(element));
      setNull();
      if (tagPath().size() > 1)
        leave();
    };
    void Attribut(const std::string &element, const std::string &attribut, const std::wstring &value) {
//      TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
      LOG(LM_INFO, "string2Obj: ignoring attribute " << element << ":" << attribut << " = " << to_string(value));
    };
    void Value(const std::wstring &val) {
//      TRACE(PARAM(val));
      if (not member())
        throw runtime_error(u8"string2Obj: " + showName() + " is no variable, can't assign");
      else if (not member()->fromStr(val, cfs))
        throw runtime_error(u8"string2Obj: invalid type in variable " + showName() + " can't assign");
    };
    void Cdata(const wchar_t *value, size_t len) { Value(std::wstring(value, len)); }

    void StartTag(const std::string &element) {
      TRACE(PARAM(element));
      if (tagPath().size() <= 1)
        return;
      if (not enter(element))
        LOG(LM_INFO, element << " wurde nicht gefunden");
    }
    void EndTag(const std::string &element) {
      TRACE(PARAM(element));
      if (tagPath().size() > 1)
        leave(element);
    }
    void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::wstring &value) {
//      TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
      if (element == "xml" and attribut == "encoding")
        encoding = mobs::to_string(value);
    }
    
    std::wistringstream str;
    std::string encoding;
    std::string prefix;
  };
#endif


  if (cfh.acceptXml()) {
    XmlReadData xd(str, cfh);
    xd.pushObject(obj);
      xd.parse();
//    catch (std::exception &e) {
//      LOG(LM_INFO, "Exception " << e.what());
//      size_t pos;
//      std::string xml = data->info(pos);
//      LOG(LM_INFO, xml.substr(pos));
//      throw std::runtime_error(std::string("Parsing failed ") + " at pos. " + std::to_string(pos));
//    }
  } else {
    JsonReadData jd(str, cfh);
    jd.pushObject(obj);
    jd.parse();
  }
}


/////////////////////////////////////////////////


}

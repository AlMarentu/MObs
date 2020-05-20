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
#include "xmlread.h"
#include <map>

using namespace std;

namespace mobs {


void string2Obj(const std::string &str, ObjectBase &obj, const ConvObjFromStr& cfh)
{
  class JsonReadData : public ObjectNavigator, public JsonParser  {
  public:
    JsonReadData(const string &input, const ConvObjFromStr &c) : JsonParser(input) { cfs = c; }
    
    void Value(const string &val, bool charType) override {
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
    void StartObject() override {
      TRACE(PARAM(lastKey));
      if (++level > 1)
        enter(lastKey, currentIdx);
      index.push(currentIdx);
      currentIdx = SIZE_T_MAX;
    }
    void Key(const string &key) override {
      lastKey = key;
    }
    void EndObject() override {
      TRACE("");
      lastKey = current();
      if (index.empty())
        throw runtime_error(u8"string2Obj: Structure invalid");
      currentIdx = index.top();
      index.pop();
      if (level-- > 1)
        leave();
      if (currentIdx != SIZE_T_MAX)
        currentIdx++;
    }
    void StartArray() override {
      TRACE("");
      currentIdx = 0;
    }
    void EndArray() override {
      TRACE("");
      currentIdx = SIZE_T_MAX;
    }
    
  private:
    int level = 0;
    size_t currentIdx = SIZE_T_MAX;
    string lastKey;
    stack<size_t> index;
  };
  
  

  if (cfh.acceptXml()) {
    XmlRead xd(str, obj, cfh);
    xd.parse();
    if (not xd.found())
      throw runtime_error("string2Obj: no Object found");
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

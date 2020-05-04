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


#include "xmlread.h"
#include "xmlparser.h"
#include "logging.h"


#include<stack>

using namespace std;

namespace mobs {


  class XmlReadData : public ObjectNavigator, public XmlParserW  {
  public:
//    XmlReadData(const std::string &input, const ConvObjFromStr &c) : XmlParserW(str), str(to_wstring(input)) { cfs = c; };
    XmlReadData(XmlReader *p, wistream &s) : XmlParserW(s), parent(p) {}
    XmlReadData(XmlReader *p, const wstring &s) : XmlParserW(str), parent(p), str(s) {}

    void NullTag(const std::string &element) {
      TRACE(PARAM(element));
      if (obj) {
        setNull();
        EndTag(element);
      }
      else
        parent->NullTag(element);
    };
    void Attribute(const std::string &element, const std::string &attribute, const std::wstring &value) {
      if (obj and not member()) {
        enter(attribute);
        if (member() and member()->xmlAsAttr())
        {
          if (not member()->fromStr(value, cfs))
            error += string(error.empty() ? "":"\n") + u8"invalid type in variable " + showName() + u8" can't assign";
        }
        leave();
      }
      else
        parent->Attribute(element, attribute, value);
    };
    void Value(const std::wstring &val) {
      if (obj) {
        if (not member())
          error += string(error.empty() ? "":"\n") + showName() + u8" is no variable, can't assign";
        else if (not member()->fromStr(val, cfs))
          error += string(error.empty() ? "":"\n") + u8"invalid type in variable " + showName() + u8" can't assign";
      }
      else
        parent->Value(val);
    };
    void Cdata(const std::wstring &value) {
      if (obj)
        Value(value);
      else
        parent->Cdata(value);
    }
    void Base64(const std::vector<u_char> &base64) {
      if (obj) {
        if (not member())
          error += string(error.empty() ? "":"\n") + showName() + u8" is no variable, can't assign";
        else //if (not member()->fromStr(val, cfs))
          error += string(error.empty() ? "":"\n") + u8"invalid type in variable " + showName() + u8" can't assign";
      }
      else
        parent->Base64(base64);
    }
    void StartTag(const std::string &element) {
      if (obj) {
        if (not enter(element) and cfs.exceptionIfUnknown())
          error += string(error.empty() ? "":"\n") + element + u8"not found";
      }
      else
        parent->StartTag(element);
    }
    void EndTag(const std::string &element) {
      if (obj) {
        if (tagPath().size() == levelStart)
        {
          parent->filled(obj, error);
          obj = nullptr;
          error = "";
          parent->EndTag(element);
        }
        else
          leave(element);
      }
      else
        parent->EndTag(element);
    }
    void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::wstring &value) {
      if (element == "xml" and attribut == "encoding")
        encoding = mobs::to_string(value);
    }
    
    void setObj(ObjectBase *o) {
      obj = o;
      reset();  // ObjectNavigator zurücksetzen
      if (obj)
        pushObject(*obj);
      levelStart = tagPath().size();
    }
    
    XmlReader *parent;
    std::wistringstream str;
    ObjectBase *obj = 0;
    size_t levelStart;
    std::string error;
    std::string encoding;
    std::string prefix;
  };


XmlReader::XmlReader(const std::string &input, const ConvObjFromStr &c) {
  data = new XmlReadData(this, to_wstring(input));
  data->cfs = c;
}

XmlReader::XmlReader(const std::wstring &input, const ConvObjFromStr &c) {
  data = new XmlReadData(this, input);
  data->cfs = c;
}

XmlReader::XmlReader(std::wistream &str, const ConvObjFromStr &c) {
  data = new XmlReadData(this, str);
  data->cfs = c;
}

XmlReader::~XmlReader() {
  delete data;
}
void XmlReader::fill(ObjectBase *obj) {
  data->setObj(obj);
}

void XmlReader::setBase64(bool b) { data->setBase64(b); }
void XmlReader::parse() { data->parse(); }
bool XmlReader::eof() const { return data->eof(); }
void XmlReader::stop() { data->stop(); }
//  XmlReadData(const std::string &input, const ConvObjFromStr &c) : XmlParserW(str), str(to_wstring(input)) { cfs = c; };




}

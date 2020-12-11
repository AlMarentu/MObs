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

static std::wstring stow(const string &s, bool dontConvert) {
  wstring res;
  if (dontConvert)
    transform(s.cbegin(), s.cend(), back_inserter(res), [](char c) -> wchar_t{ return u_char(c); });
  else
    res = to_wstring(s);
  return res;
}
                                                     
  class XmlReadData : public ObjectNavigator, public XmlParserW  {
  public:
    XmlReadData(XmlReader *p, wistream &s, const ConvObjFromStr &c) : ObjectNavigator(c), XmlParserW(s), parent(p) {}
    XmlReadData(XmlReader *p, const wstring &s, const ConvObjFromStr &c) : ObjectNavigator(c), XmlParserW(str), parent(p), str(s) {}
    XmlReadData(XmlReader *p, const string &s, const ConvObjFromStr &c, bool charsetUnknown = false) : ObjectNavigator(c), XmlParserW(str), parent(p),
                str(stow(s, charsetUnknown)), doConversion(charsetUnknown) { }

    std::string elementRemovePrefix(const std::string &element) const {
      if (prefix.empty())
        return element;
      if (element.length() > prefix.length() and element.substr(0, prefix.length()) == prefix)
        return element.substr(prefix.length());
      throw runtime_error("Prefix mismatch");
    }
    void NullTag(const std::string &element) override {
      TRACE(PARAM(element));
      if (obj) {
        setNull();
        EndTag(element);
      }
      else
        parent->NullTag(elementRemovePrefix(element));
    };
    void Attribute(const std::string &element, const std::string &attribute, const std::wstring &value) override {
      if (obj and not member()) {
        enter(attribute);
        if (member() and member()->hasFeature(XmlAsAttr))
        {
          if (not member()->fromStr(value, cfs))
            error += string(error.empty() ? "":"\n") + u8"invalid type in variable " + showName() + u8" can't assign";
        }
        leave();
      }
      else
        parent->Attribute(elementRemovePrefix(element), attribute, value);
    };
    void Value(const std::wstring &val) override {
      if (obj) {
        if (not member())
          error += string(error.empty() ? "":"\n") + showName() + u8" is no variable, can't assign";
        else if (not member()->fromStr(val, cfs))
          error += string(error.empty() ? "":"\n") + u8"invalid type in variable " + showName() + u8" can't assign";
      }
      else
        parent->Value(val);
    };
    void Cdata(const std::wstring &value) override {
      if (obj)
        Value(value);
      else
        parent->Cdata(value);
    }
    void Base64(const std::vector<u_char> &base64) override {
      if (obj) {
        if (not member())
          error += string(error.empty() ? "":"\n") + showName() + u8" is no variable, can't assign";
        else //if (not member()->fromStr(val, cfs))
          error += string(error.empty() ? "":"\n") + u8"invalid type in variable " + showName() + u8" can't assign";
      }
      else
        parent->Base64(base64);
    }
    void StartTag(const std::string &element) override {
      if (obj) {
        if (not enter(elementRemovePrefix(element)) and cfs.exceptionIfUnknown())
          error += string(error.empty() ? "":"\n") + elementRemovePrefix(element) + u8"not found";
      }
      else
        parent->StartTag(element);
    }
    void EndTag(const std::string &element) override {
      if (obj) {
        if (currentLevel() == levelStart)
        {
          parent->filled(obj, error);
          obj = nullptr;
          error = "";
          parent->EndTag(element);
        }
        else
          leave(elementRemovePrefix(element));
      }
      else
        parent->EndTag(element);
    }
    void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::wstring &value) override {
      if (element == "xml" and attribut == "encoding") {
        encoding = mobs::to_string(value);
        
        // da wstringstream das encoding der local ignoriert, hier explizit umsetzen wenn Input ein std::string mit undefiniertem Zeichensatz war
        if (doConversion and encoding != "ISO-8859-1") {
          size_t pos = str.tellg();
          std::wistringstream str2;
          str2.swap(str);
          const std::wstring &s = str2.str();
          if (encoding == "UTF-8") {
            std::string res;
            transform(s.cbegin(), s.cend(), back_inserter(res), [](char c) { return char(c); });
            str.str(to_wstring(res));
          }
          else if (encoding == "ISO-8859-15") {
            std::wstring res;
            transform(s.cbegin(), s.cend(), back_inserter(res), [](wchar_t c) { return from_iso_8859_15(c); });
            str.str(res);
          }
          else if (encoding == "ISO-8859-9") {
            std::wstring res;
            transform(s.cbegin(), s.cend(), back_inserter(res), [](wchar_t c) { return from_iso_8859_9(c); });
            str.str(res);
          }
          str.seekg(pos);
        }
      }
    }
    void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, mobs::CryptBufBase *&cryptBufp) override {
      parent->Encrypt(algorithm, keyName, cipher, cryptBufp);
    }


    void setObj(ObjectBase *o) {
      obj = o;
      reset();  // ObjectNavigator zurücksetzen
      if (obj)
        pushObject(*obj);
      levelStart = currentLevel();
    }
    
    XmlReader *parent;
    std::wistringstream str;
    ObjectBase *obj = nullptr;
    size_t levelStart{};
    std::string error;
    std::string encoding;
    std::string prefix;
    bool doConversion = false;
  };


XmlReader::XmlReader(const std::string &input, const ConvObjFromStr &c, bool charsetUnknown) {
  data = new XmlReadData(this, input, c, charsetUnknown);
}

XmlReader::XmlReader(const std::wstring &input, const ConvObjFromStr &c) {
  data = new XmlReadData(this, input, c);
}

XmlReader::XmlReader(std::wistream &str, const ConvObjFromStr &c) {
  data = new XmlReadData(this, str, c);
}

XmlReader::~XmlReader() {
  delete data;
}
void XmlReader::fill(ObjectBase *obj) {
  data->setObj(obj);
}
std::string XmlReader::elementRemovePrefix(const std::string &element) const {
  return data->elementRemovePrefix(element);
}


void XmlReader::setPrefix(const std::string &pf) { data->prefix = pf; }
void XmlReader::setBase64(bool b) { data->setBase64(b); }
void XmlReader::parse() { data->parse(); }
bool XmlReader::eof() const { return data->eof(); }
void XmlReader::stop() { data->stop(); }
//  XmlReadData(const std::string &input, const ConvObjFromStr &c) : XmlParserW(str), str(to_wstring(input)) { cfs = c; };




}

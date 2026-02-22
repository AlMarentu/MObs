// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2026 Matthias Lautner
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
    void NullTag(const string &ns, const std::string &element) override {
      TRACE(PARAM(element));
      if (levelStart and (encLevel != 1 or levelStartTmp)) {
        setNull();
        EndTag(currentXmlns(), element);
      }
      else
        parent->NullTag(elementRemovePrefix(element));
    };
    void Attribute(const string &ns, const std::string &element, const std::string &attribute, const std::wstring &value) override {
      if (element == "EncryptedData" and attribute == "Type" and value == L"http://www.w3.org/2001/04/xmlenc#Element") {
        //LOG(LM_INFO, "ENCRYPTION BEGIN");
        encLevel = 1;
        encCbb = nullptr;
      }
      else if (encLevel == 1 and element == "EncryptionMethod" and attribute == "Algorithm" and ns == "http://www.w3.org/2001/04/xmlenc#") {
        encAlgo =  mobs::to_string(value);
        if (encAlgo.length() > ns.length() and encAlgo.substr(0, ns.length()) == ns)
          encAlgo.erase(0, ns.length());
      }
      else if (levelStart and not member()) {
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
      //LOG(LM_INFO, "XMLREAD VALUE  " << mobs::to_string(val) << " " << levelStart);
      if (levelStart) {
        if (not member())
          error += string(error.empty() ? "":"\n") + showName() + u8" is no variable, can't assign";
        else if (not member()->fromStr(val, cfs))
          error += string(error.empty() ? "":"\n") + u8"invalid type in variable " + showName() + u8" can't assign";
      }
      else
        parent->Value(val);
    };
    void Base64(const std::vector<u_char> &base64) override {
      if (levelStart) {
        if (not member())
          error += string(error.empty() ? "":"\n") + showName() + u8" is no variable, can't assign";
        else //if (not member()->fromStr(val, cfs))
          error += string(error.empty() ? "":"\n") + u8"invalid type in variable " + showName() + u8" can't assign";
      }
      else
        parent->Base64(base64);
    }
    void StartTag(const string &ns, const std::string &element) override {
      //LOG(LM_INFO, "XMLREAD START TAG " << element);
      if (encLevel == 2 and levelStartTmp == 0 and element == "CipherValue" and ns == "http://www.w3.org/2001/04/xmlenc#") {
        encLevel = 3;
        startEncryption(encCbb);
        encCbb = nullptr;
      }
      else if (encLevel == 1 and levelStartTmp == 0 and element == "CipherData" and ns == "http://www.w3.org/2001/04/xmlenc#") {
        encLevel = 2; // danach muss CipherValue kommen
      }
      else if (encLevel == 1 and (element == "EncryptedData" or element == "EncryptionMethod")) {
        // Encryption-Start-Tags nicht weiterreichen
      }
      else if (encLevel == 1 and element == "KeyInfo" and ns == "http://www.w3.org/2000/09/xmldsig#") {
        // KeyInfo-Objekt parsen
        ki = new mobs::KeyInfo;
        pushObject(*ki, "KeyInfo");
        levelStartTmp = currentLevel();
        if (not levelStart)
          levelStart = levelStartTmp;
      }
      else if (levelStart) {
        if (not enter(elementRemovePrefix(element)) and cfs.exceptionIfUnknown())
          error += string(error.empty() ? "":"\n") + elementRemovePrefix(element) + u8"not found";
      }
      else
        parent->StartTag(element);
    }
    void EndTag(const string &ns, const std::string &element) override {
      //LOG(LM_INFO, "XMLREAD END TAG " << element);
      if (levelStartTmp > 0) {
        if (currentLevel() == levelStartTmp) {
          if (levelStartTmp == levelStart) // bei primären Objekt KeyInfo waren beide identisch
            levelStart = 0;
          levelStartTmp = 0;
          popTemp();
          if (ki) {
            LOG(LM_DEBUG, "Filled Encryption KeyInfo: " << ki->to_string());
            if (not encCbb) // mit allen KeyInfos versuch bis eine gefunden
            {
              parent->Encrypt(encAlgo, ki, encCbb);
              //if (encCbb) LOG(LM_DEBUG, "ENCRYPT OK");
            }
            ki = nullptr;
          }
        }
        else
          leave(elementRemovePrefix(element));
      }
      else if (encLevel == 3 and levelStartTmp == 0 and element == "CipherValue") {
        encLevel = 2;
      }
       else if (encLevel == 2 and levelStartTmp == 0 and element == "CipherData") {
        encLevel = 1;
      }
      else if (encLevel == 1 and levelStartTmp == 0 and element == "EncryptedData") {
        encLevel = 0;
        parent->EncryptionFinished();
      }
      else if (encLevel == 1 and element == "EncryptionMethod") {
        // Encryption-End-Tags nicht weiterreichen
      }
      else if (levelStart) {
        if (currentLevel() == levelStart)
        {
          parent->filled(obj, error);
          obj = nullptr;
          levelStart = 0;
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
      TRACE(PARAM(element) << PARAM(attribut));
      if (element == "xml" and attribut == "encoding") {
        encoding = mobs::to_string(value);
        
        // da wstringstream das encoding der local ignoriert, hier explizit umsetzen, wenn Input ein string mit undefiniertem Zeichensatz war
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
      parent->ProcessingInstruction(element, attribut, value);
    }

    void setObj(ObjectBase *o) {
      obj = o;
      reset();  // ObjectNavigator zurücksetzen
      if (obj)
        pushObject(*obj);
      levelStart = currentLevel();
      if (levelStart == 0)
        THROW("NOT SUPPORTED TODO");
    }
    void setMaxElementSize(size_t s) {
      XmlParserW::maxElementSize = s;
    }

    XmlReader *parent;
    std::wistringstream str;
    ObjectBase *obj = nullptr;
    ObjectBase *ki = nullptr;
    size_t levelStart = 0;
    size_t levelStartTmp = 0;
    std::string error;
    std::string encoding;
    std::string encAlgo; // EncryptionMethod
    CryptBufBase *encCbb{};
    std::string prefix;
    int encLevel = 0; // off/EncryptedData/CipherData/CipherValue
    bool doConversion = false;
  };


XmlReader::XmlReader(const std::string &input, const ConvObjFromStr &c, bool charsetUnknown) {
  data = std::unique_ptr<XmlReadData>(new XmlReadData(this, input, c, charsetUnknown));
}

XmlReader::XmlReader(const std::wstring &input, const ConvObjFromStr &c) {
  data = std::unique_ptr<XmlReadData>(new XmlReadData(this, input, c));
}

XmlReader::XmlReader(std::wistream &str, const ConvObjFromStr &c) {
  data = std::unique_ptr<XmlReadData>(new XmlReadData(this, str, c));
}

XmlReader::~XmlReader() = default;

void XmlReader::fill(ObjectBase *obj) {
  data->setObj(obj);
}

std::string XmlReader::elementRemovePrefix(const std::string &element) const {
  return data->elementRemovePrefix(element);
}

std::istream &XmlReader::byteStream(size_t len, CryptBufBase *cbbp) {
  return data->byteStream(len, cbbp);
}


void XmlReader::setPrefix(const std::string &pf) { data->prefix = pf; }
void XmlReader::setBase64(bool b) { data->setBase64(b); }
bool XmlReader::parse() { return data->parse(); }
bool XmlReader::eof() const { return data->eof(); }
bool XmlReader::eot() const { return data->eot(); }
void XmlReader::stop() { data->stop(); }
void XmlReader::readTillEof(bool s) { data->readTillEof(s); }
void XmlReader::readNonBlocking(bool s) { data->readNonBlocking(s); };
size_t XmlReader::level() const { return data->currentLevel(); }
std::string XmlReader::currentXmlns() const { return data->currentXmlns(); };
void XmlReader::setMaxElementSize(size_t s) { data->setMaxElementSize(s); }
std::wistream &XmlReader::getIstr() { return data->getIstr(); }
bool XmlReader::encrypted() const { return data->encrypted(); }

//  XmlReadData(const std::string &input, const ConvObjFromStr &c) : XmlParserW(str), str(to_wstring(input)) { cfs = c; };




}

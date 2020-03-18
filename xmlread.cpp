// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs
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



class XmlReadData : public ObjectInserter, public XmlParser  {
public:
  XmlReadData(const std::string &input) : XmlParser(input) {  };
  ~XmlReadData() { };
    
  std::string encoding;
    
  void NullTag(const std::string &element) {
    TRACE(PARAM(element));
    LOG(LM_INFO, "NULL " << element);

    if (member() and member()->nullAllowed())
    {
      member()->setNull(true);
    }
    EndTag(element);
  };
  void Attribut(const std::string &element, const std::string &attribut, const std::string &value) {
    TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
    LOG(LM_INFO, "Attribut " << element << " " << attribut << " " << value);

  };
  void Value(const std::string &value) {
    TRACE(PARAM(value));
    LOG(LM_INFO, "Value " << showName() << " = " << value);
    if (not member())
      LOG(LM_INFO, "Variable fehlt " << showName())
    else
      member()->fromStr(value);
  };
  void Cdata(const char *val, size_t len) {
    std::string value(val, len);
    TRACE(PARAM(value));
    LOG(LM_INFO, "Cdata " << value << " atadC");
    if (not member())
      LOG(LM_WARNING, "Variable fehlt " << showName())
    else
      member()->fromStr(value);
  };
  void StartTag(const std::string &element) {
    TRACE(PARAM(element));
    LOG(LM_INFO, "Start " << element << " " << tagPath().size());

    if (tagPath().size() <= 1)
      return;
    if (not enter(element))
      LOG(LM_INFO, element << " wurde nicht gefunden");


  }
  void EndTag(const std::string &element) {
    TRACE(PARAM(element));
    LOG(LM_INFO, "Ende " << element);
    if (tagPath().size() > 1)
      leave(element);
  }
  void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::string &value) {
    TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
    LOG(LM_INFO, "ProcInst " << element << " " << attribut << " " << value);
    if (element == "xml" and attribut == "encoding")
      encoding = value;
  }

  private:
    std::string prefix;;
  };
  
  
  
  void XmlRead::parse()
  {
    TRACE("");
    data->parse();
  }
  
  XmlRead::XmlRead(const std::string &input)
  {
    TRACE("");
    data = new XmlReadData(input);
  }
  
  XmlRead::~XmlRead()
  {
    TRACE("");
    delete data;
  }
  
  void XmlRead::fill(ObjectBase &obj)
  {
    TRACE("");
    data->pushObject(obj);
    try {
      parse();
    } catch (std::exception &e) {
      LOG(LM_INFO, "Exception " << e.what());
      size_t pos;
      const std::string &xml = data->info(pos);
          LOG(LM_INFO, xml.substr(pos));
      throw std::runtime_error(std::string("Parsing failed ") + " at pos. " + std::to_string(pos));
    }
  }
  
  

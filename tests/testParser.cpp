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


#include "jsonparser.h"
#include "jsonparser.h"
#include "xmlparser.h"
#include "xmlparser.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;


namespace {

class JParser: public mobs::JsonParser {
public:
  explicit JParser(const string &i) : mobs::JsonParser(i) {};
  void Key(const std::string &value) override { LOG(LM_INFO, "KEY " << value); };
  void Value(const std::string &value, bool charType) override { LOG(LM_INFO, "VALUE " << value); }
  void StartArray() override { LOG(LM_INFO, "START ARRAY"); }
  void EndArray() override { LOG(LM_INFO, "END ARRAY"); }
  void StartObject() override { LOG(LM_INFO, "START OBJECT"); }
  void EndObject() override { LOG(LM_INFO, "END OBJECT"); }
};

class XParser: public mobs::XmlParser {
public:
  explicit XParser(const string &i) : mobs::XmlParser(i) {}
  void NullTag(const std::string &element) override { LOG(LM_INFO, "NULL"); }
  void Attribute(const std::string &element, const std::string &attribut, const std::string &value) override { LOG(LM_INFO, "ATTRIBUT " << element); }
  void Value(const std::string &value) override { LOG(LM_INFO, "VALUE"); }
  void Cdata(const char *value, size_t len) override { LOG(LM_INFO, "CDATA >" << string(value, len) << "<"); }
  void StartTag(const std::string &element) override { LOG(LM_INFO, "START " << element); }
  void EndTag(const std::string &element) override { LOG(LM_INFO, "END " << element); }
  void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::string &value) override { LOG(LM_INFO, "PI" << element); }
};

class XParserW: public mobs::XmlParserW {
public:
  explicit XParserW(const wstring &i) : mobs::XmlParserW(str), str(i) { }
  void NullTag(const std::string &element) override { LOG(LM_INFO, "NULL"); }
  void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value) override { LOG(LM_INFO, "ATTRIBUT " << element); }
  void Value(const std::wstring &value) override { LOG(LM_INFO, "VALUE >" << mobs::to_string(value) << "<"); }
  void Cdata(const std::wstring &value) override { LOG(LM_INFO, "CDATA >" << mobs::to_string(value) << "<"); lastCdata = mobs::to_string(value); }
  void Base64(const std::vector<u_char> &base64) override {
    std::string s;
    std::copy(base64.cbegin(), base64.cend(), back_inserter(s));
    lastCdata = s;
    LOG(LM_INFO, "BASE64 >" << s << "< " << base64.size());
  }
  void StartTag(const std::string &element) override { LOG(LM_INFO, "START " << element); }
  void EndTag(const std::string &element) override { LOG(LM_INFO, "END " << element); }
  void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::wstring &value) override { LOG(LM_INFO, "PI" << element); }
  void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, mobs::CryptBufBase *&cryptBufp) override { };

  std::wistringstream str;
  std::string lastCdata;
};



TEST(parserTest, jsonTypes) {
  string inhalt = R"({Bool:true,Char:"a",Char16_t:"b",Char32_t:"c",Wchar_t:"d",SignedChar:"e",ShortInt:42,Int:-9876543,LongInt:-45454545,LongLongInt:-34343434343434,UnsignedChar:"f",UnsignedShortInt:999,UnsignedInt:88888,UnsignedLongLong:109876543,UnsignedLongLongInt:1234567890,Float:-21.3,Double:1e-05,LongDouble:123.456,String:"Anton",Wstring:"Berti",U16string:"Conni",U32string:"Det"})";

  JParser p(inhalt);
  ASSERT_NO_THROW(p.parse());
}

TEST(parserTest, jsonParser) {

  string jjj =
  // aus https://de.wikipedia.org/wiki/JavaScript_Object_Notation
  R"(
  {
  "Herausgeber": "Xema",
  "Nummer": "1234-5678-9012-3456",
  "Deckung": 2e+6,
  "Waehrung": "EURO",
  "Inhaber":
  {
  "Name": "Mustermann",
  "Vorname": "Max",
  "maennlich": true,
  "Hobbys": ["Reiten", "Golfen", "Lesen"],
  "Alter": 42,
  "Kinder": [],
  "Partner": null
  }
  })";
  
 
  JParser p(jjj);
  ASSERT_NO_THROW(p.parse());   
}

void parse(string s) { JParser p(s); p.parse(); };

TEST(parserTest, jsonStruct1) {
  
  EXPECT_NO_THROW(parse(u8"{}"));
  EXPECT_NO_THROW(parse(u8"[{}]"));
  EXPECT_NO_THROW(parse(u8"{ \"a\" : 1 }"));
  EXPECT_NO_THROW(parse(u8"{a:[]}"));
  EXPECT_NO_THROW(parse(u8"[]"));

  EXPECT_ANY_THROW(parse(u8""));
  EXPECT_ANY_THROW(parse(u8"\"a\":1"));
  EXPECT_ANY_THROW(parse(u8"{[]}"));
  EXPECT_ANY_THROW(parse(u8"{a:b,[]}"));
  EXPECT_ANY_THROW(parse(u8"{a:b:c}"));
  EXPECT_ANY_THROW(parse(u8"{a,b}"));
  EXPECT_ANY_THROW(parse(u8"{a:[a:b]}"));
  EXPECT_ANY_THROW(parse(u8"{a:[a,b,]}"));

}

void xparse(string s) { XParser p(s); p.parse(); };
void xparse(wstring s, bool b64 = false) { XParserW p(s); p.setBase64(b64); p.parse(); };
std::string xparseCdata(wstring s, bool b64 = false) { XParserW p(s); p.setBase64(b64); p.parse(); return p.lastCdata; };

// aus https://de.wikipedia.org/wiki/Extensible_Markup_Language
const string x1 = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<verzeichnis>
     <titel>Wikipedia Städteverzeichnis</titel>
     <eintrag>
          <stichwort>Genf</stichwort>
          <eintragstext>Genf ist der Sitz von ...</eintragstext>
     </eintrag>
     <eintrag>
          <stichwort>Köln</stichwort>
          <eintragstext>Köln ist eine Stadt, die ...</eintragstext>
     </eintrag>
</verzeichnis>
                    )";
                    
TEST(parserTest, xmlStruct1) {
  EXPECT_NO_THROW(xparse(x1));
  EXPECT_NO_THROW(xparse(u8"<abc/>"));
  EXPECT_NO_THROW(xparse(u8"<abc a=\"xx\" bcd=\"9999\"/>"));
  EXPECT_NO_THROW(xparse(u8"<abc f=\"\">xx</abc>"));
  EXPECT_NO_THROW(xparse(u8"<abc>   <cde/> </abc>"));
  EXPECT_NO_THROW(xparse(u8"<abc>   <!-- sdfsdf -->  <cde/>  </abc>"));
  EXPECT_NO_THROW(xparse(u8"<abc>  <cde/> </abc> <!-- sdfs<< df --> "));
  
  EXPECT_ANY_THROW(xparse(u8"<abc  />"));
  EXPECT_ANY_THROW(xparse(u8"<abc>   <cde/> </abce>"));
  EXPECT_ANY_THROW(xparse(u8"<abc a =\"xx\" bcd=\"9999\"/>"));
  EXPECT_ANY_THROW(xparse(u8"<abc a=xx bcd=\"9999\"/>"));

}

TEST(parserTest, xmlStructW1) {
  EXPECT_NO_THROW(xparse(mobs::to_wstring(x1)));
  EXPECT_NO_THROW(xparse(L"<abc/>"));
  EXPECT_NO_THROW(xparse(L"<abc a=\"xx\" bcd=\"9999\"/>"));
  EXPECT_NO_THROW(xparse(L"<abc f=\"\">xx&#188;&#x20;ö&lt;ä&gt;ü</abc>"));
  EXPECT_NO_THROW(xparse(L"<abc>   <cde/> </abc>"));
  EXPECT_NO_THROW(xparse(L"<ggg>   <!-- sdfsdf --><cde/>  </ggg>"));
  EXPECT_NO_THROW(xparse(L"<abc>   <![CDATA[J]]></abc>"));
  EXPECT_EQ("J", xparseCdata(L"<abc>   <![CDATA[J]]></abc>"));
  EXPECT_EQ("AB", xparseCdata(L"<abc>   <![CDATA[A]]><![CDATA[B]]></abc>"));
  EXPECT_EQ("...]]>...", xparseCdata(L"<abc>   <![CDATA[...]]]>  <![CDATA[]>...]]> </abc>"));
  EXPECT_NO_THROW(xparse(L"<abc>  <cde/> </abc> <!-- sdfs<< df --> "));
  
  EXPECT_ANY_THROW(xparse(L"<abc>  dd <![CDATA[J]]></abc>"));

  EXPECT_ANY_THROW(xparse(L"<abc  />"));
  EXPECT_ANY_THROW(xparse(L"<abc>   <cde/> </abce>"));
  EXPECT_ANY_THROW(xparse(L"<abc a =\"xx\" bcd=\"9999\"/>"));
  EXPECT_ANY_THROW(xparse(L"<abc a=xx bcd=\"9999\"/>"));
  EXPECT_ANY_THROW(xparse(L"<abc> a  <![CDATA[A]]><![CDATA[B]]></abc>"));
  EXPECT_ANY_THROW(xparse(L"<abc>   <![CDATA[A]]> x <![CDATA[B]]></abc>"));
  EXPECT_ANY_THROW(xparse(L"<abc>  <![CDATA[A]]><![CDATA[B]]> kk </abc>"));

}

TEST(parserTest, base64) {
  EXPECT_NO_THROW(xparse(L"<abc>   <![CDATA[UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJl\n  biwgSm9naHVydCB1bmQgUXVhcms= ]]>  </abc>"));
  EXPECT_EQ(u8"Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark",
            xparseCdata(L"<abc>   <![CDATA[UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJl\n  biwgSm9naHVydCB1bmQgUXVhcms= ]]>  </abc>", true));
  // missing padding
  EXPECT_EQ(u8"Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark",
            xparseCdata(L"<abc>   <![CDATA[UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJl\n  biwgSm9naHVydCB1bmQgUXVhcms ]]>  </abc>", true));
  EXPECT_ANY_THROW(xparse(L"<abc>   <![CDATA[UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJl\n =  biwgSm9naHVydCB1bmQgUXVhcms= ]]>  </abc>", true));
  EXPECT_EQ("A\n", xparseCdata(L"<abc>   <![CDATA[QQo= ]]>  </abc>", true));
  EXPECT_NO_THROW(xparseCdata(L"<abc>   <![CDATA[Q+/= ]]>  </abc>", true));
  EXPECT_ANY_THROW(xparseCdata(L"<abc>   <![CDATA[Q-o= ]]>  </abc>", true));
}

}


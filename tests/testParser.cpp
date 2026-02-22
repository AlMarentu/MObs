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


#include "jsonparser.h"
#include "jsonparser.h"
#include "xmlparser.h"
#include "xmlparser.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

#include "nbuf.h"
#include "xmlread.h"

using namespace std;


namespace {

class JParser : public mobs::JsonParser {
public:
  explicit JParser(const string &i) : mobs::JsonParser(i) {};

  void Key(const std::string &value) override { LOG(LM_INFO, "KEY " << value); };

  void Value(const std::string &value, bool charType) override {
    LOG(LM_INFO, "VALUE " << value);
    lastValue = value;
  };

  void StartArray() override { LOG(LM_INFO, "START ARRAY"); }

  void EndArray() override { LOG(LM_INFO, "END ARRAY"); }

  void StartObject() override { LOG(LM_INFO, "START OBJECT"); }

  void EndObject() override { LOG(LM_INFO, "END OBJECT"); }

  string lastValue;
};

class XParser : public mobs::XmlParser {
public:
  explicit XParser(const string &i) : mobs::XmlParser(i) {}

  void NullTag(const std::string &element) override { LOG(LM_INFO, "NULL"); }

  void Attribute(const std::string &element, const std::string &attribut, const std::string &value) override {
    LOG(LM_INFO, "ATTRIBUT " << element);
  }

  void Value(const std::string &value) override { LOG(LM_INFO, "VALUE"); }

  void Cdata(const char *value, size_t len) override { LOG(LM_INFO, "CDATA >" << string(value, len) << "<"); }

  void StartTag(const std::string &element) override { LOG(LM_INFO, "START " << element); }

  void EndTag(const std::string &element) override { LOG(LM_INFO, "END " << element); }

  void ProcessingInstruction(const std::string &element, const std::string &attribut,
                             const std::string &value) override { LOG(LM_INFO, "PI" << element); }
};

class XParserW : public mobs::XmlParserW {
public:
  explicit XParserW(const wstring &i) : mobs::XmlParserW(str), str(i) {}

  void NullTag(const string &ns, const std::string &element) override { LOG(LM_INFO, "NULL " << ns << element); }

  void Attribute(const string &ns, const std::string &element, const std::string &attribut, const std::wstring &value) override {
    LOG(LM_INFO, "ATTRIBUT " << attribut << " (" << ns << element <<
                             ") VALUE >" << mobs::to_string(value) << "<");
    lastValue = mobs::to_string(value);
  }

  void Value(const std::wstring &value) override {
    LOG(LM_INFO, "VALUE >" << mobs::to_string(value) << "<");
    lastValue = mobs::to_string(value);
    if (wait)
      stop();
  }

  void Base64(const std::vector<u_char> &base64) override {
    std::string s;
    std::copy(base64.cbegin(), base64.cend(), back_inserter(s));
    lastCdata = s;
    LOG(LM_INFO, "BASE64 >" << s << "< " << base64.size());
  }

  void StartTag(const string &ns, const std::string &element) override { LOG(LM_INFO, "START " << ns << element);
    lastKey += ns  + element + "|";
  }

  void EndTag(const string &ns, const std::string &element) override { LOG(LM_INFO, "END " << ns << element); }

  void ProcessingInstruction(const std::string &element, const std::string &attribut,
                             const std::wstring &value) override { LOG(LM_INFO, "PI " << element << "." << attribut << "="
                               << mobs::to_string(value)); }


  std::wistringstream str;
  std::string lastCdata;
  std::string lastValue;
  std::string lastKey;
  bool wait = false;
};

class XParserReader : public mobs::XmlReader {
public:
  explicit XParserReader(const wstring &i) : XmlReader(str), str(i) {}

  void NullTag(const std::string &element) override { LOG(LM_INFO, "NULL " << element); }

  void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value) override {
    LOG(LM_INFO, "ATTRIBUT " << attribut <<
                             " VALUE >" << mobs::to_string(value) << "<");
    lastValue = mobs::to_string(value);
  }

  void Value(const std::wstring &value) override {
    LOG(LM_INFO, "VALUE >" << mobs::to_string(value) << "<");
    lastValue = mobs::to_string(value);
    if (wait)
      stop();
  }

  void Base64(const std::vector<u_char> &base64) override {
    std::string s;
    std::copy(base64.cbegin(), base64.cend(), back_inserter(s));
    lastCdata = s;
    LOG(LM_INFO, "BASE64 >" << s << "< " << base64.size());
  }

  void StartTag(const std::string &element) override { LOG(LM_INFO, "START " << element);
    lastKey += currentXmlns() + element + "|";
  }

  void EndTag(const std::string &element) override { LOG(LM_INFO, "END " << element); }

  void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher,
               mobs::CryptBufBase *&cryptBufp) override {
    LOG(LM_INFO, "Encrypt algorithm=" << algorithm << " keyName=" << keyName << " cipher=" << cipher);
    cryptBufp = new mobs::CryptBufNone();
  }

  void filled(mobs::ObjectBase *obj, const std::string &error) override {
     LOG(LM_INFO, "Filled: " << obj->to_string());
  }

  void EncryptionFinished() override { }


  std::wistringstream str;
  std::string lastCdata;
  std::string lastValue;
  std::string lastKey;
  bool wait = false;
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

void parse(string s) {
  JParser p(s);
  p.parse();
};

std::string parseValue(string s) {
  JParser p(s);
  p.parse();
  return p.lastValue;
};

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
  EXPECT_EQ(u8"1", parseValue(u8"{\"a\":1}"));
  EXPECT_EQ(u8"Ot\tt€o", parseValue(u8"{\"a\":\"Ot\\tt\\u20aco\"}")); // u+20ac = €
  EXPECT_EQ(u8"1\n2\t3", parseValue(u8"{\"a\":\"1\\n2\\t3\"}"));

}

void xparse(string s) {
  XParser p(s);
  p.parse();
};

std::string xparse(wstring s) {
  XParserW p(s);
  p.parse();
  return p.lastValue;
};

std::string xparseCdata(wstring s) {
  XParserW p(s);
  p.setBase64(true);
  p.parse();
  return p.lastCdata;
};

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
  //EXPECT_NO_THROW(xparse(mobs::to_wstring(x1)));
  EXPECT_NO_THROW(xparse(L"<abc/>"));
  EXPECT_NO_THROW(xparse(L"<abc \n/>"));
  EXPECT_NO_THROW(xparse(L"<abc a=\"xx\"  bcd=\"9999\"/>"));
  EXPECT_EQ(u8"xx¼ ö<ä>ü", xparse(L"<abc f=\"ä&amp;\n\">xx&#188;&#x20;ö&lt;ä&gt;ü</abc>"));
  ASSERT_ANY_THROW(xparse(L"<!ENTITY greet \"Hallo !!\" x ><abc>xx&#188;&#x20;ö&lt;ä&gt;ü</abc>"));
  ASSERT_NO_THROW(
      xparse(L"<!ENTITY greet \"Hallo !!\" ><!ENTITY   xxx   \"XXX&#10;\"><abc>xx&#188;&#x20;ö&lt;ä&gt;ü&xxx;</abc>"));
  EXPECT_EQ(u8"xx¼ ö<Hallo !!>üXXX\n&lt;", xparse(
      L"<!ENTITY greet \"Hallo !!\" ><!ENTITY   xxx   \"XXX&#10;&lt;\"><abc>xx&#188;&#x20;ö&lt;&greet;&gt;ü&xxx;</abc>"));
  EXPECT_NO_THROW(xparse(L"<abc>   <cde/> </abc>"));
  EXPECT_NO_THROW(xparse(L"<abc \t>   <cde \n /> </abc \t >"));
  EXPECT_NO_THROW(xparse(L"<ggg>   <!-- sdfsdf --><cde/>  </ggg>"));
  EXPECT_EQ(u8"9999", xparse(L"<abc a=\"xx\" \t bcd=\"9999\" \t/> \n"));
  EXPECT_EQ(u8"9<999", xparse(L"<abc a=\"xx\" \t bcd=\"9&lt;999\" \t/> \n"));
  EXPECT_EQ(u8" A J", xparse(L"<abc> A <![CDATA[J]]></abc>"));
  EXPECT_EQ("\nJ \t ", xparse(L"<abc>\n<![CDATA[J]]> \t </abc>"));
  EXPECT_EQ("   ...]]>... ", xparse(L"<abc>   <![CDATA[...]]]><![CDATA[]>...]]> </abc>"));
  EXPECT_EQ(u8"   A x B \n", xparse(L"<abc>   <![CDATA[A]]> x <![CDATA[B]]> \n</abc>"));
  EXPECT_EQ(u8"<A&lt;<<!<", xparse(L"<abc>&lt;<![CDATA[A&lt;]]>&lt;<![CDATA[<!]]>&lt;</abc>"));
  EXPECT_EQ(u8"J", xparse(L"<abc>   <![CDATA[A]]> x <![CDATA[B]]> \n</abc><xyz>J</xyz>"));
  EXPECT_NO_THROW(xparse(L"<abc>  <cde/> </abc> <!-- sdfs<< df --> "));

  EXPECT_NO_THROW(xparse(L"<abc>  dd <![CDATA[J]]></abc>"));
  EXPECT_NO_THROW(xparse(L"<abc> a  <![CDATA[A]]><![CDATA[B]]></abc>"));
  EXPECT_NO_THROW(xparse(L"<abc>  <![CDATA[A]]><![CDATA[B]]> kk </abc>"));
  EXPECT_NO_THROW(xparse(L"<abc  />"));
  EXPECT_ANY_THROW(xparse(L"<abc>   <cde/> </abce>"));
  EXPECT_NO_THROW(xparse(L"<abc a =\t \"xx\" \t bcd='9999'/>"));
  EXPECT_NO_THROW(xparse(L" \t <abc a ='xx'/>"));
  EXPECT_NO_THROW(xparse(L"<?xml version=\"1.0\"?> \n <abc a='xx' bcd=\"9999\"/>"));
  EXPECT_ANY_THROW(xparse(L" <?xml version=\"1.0\"?> \n <abc a='xx' bcd=\"9999\"/>"));
  EXPECT_ANY_THROW(xparse(L"<abc a=\"xx' bcd=\"9999\"/>"));
  EXPECT_ANY_THROW(xparse(L"<abc a a='xx' bcd=\"9999\"/>"));

}

TEST(parserTest, base64) {
  EXPECT_NO_THROW(xparse(
      L"<abc>   <![CDATA[UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJl\n  biwgSm9naHVydCB1bmQgUXVhcms= ]]>  </abc>"));
  EXPECT_EQ(u8"Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark",
            xparseCdata(
                L"<abc>   <![CDATA[UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJl\n  biwgSm9naHVydCB1bmQgUXVhcms= ]]>  </abc>"));
  // missing padding
  EXPECT_EQ(u8"Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark",
            xparseCdata(
                L"<abc>   <![CDATA[UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJl\n  biwgSm9naHVydCB1bmQgUXVhcms ]]>  </abc>"));
  EXPECT_ANY_THROW(xparseCdata(
      L"<abc>   <![CDATA[UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJl\n =  biwgSm9naHVydCB1bmQgUXVhcms= ]]>  </abc>"));
  EXPECT_EQ("A\n", xparseCdata(L"<abc>   <![CDATA[QQo= ]]>  </abc>"));
  EXPECT_NO_THROW(xparseCdata(L"<abc>   <![CDATA[Q+/= ]]> \t </abc>"));
  EXPECT_ANY_THROW(xparseCdata(L"<abc>   <![CDATA[Q+/= ]]> x </abc>"));
  EXPECT_ANY_THROW(xparseCdata(L"<abc>  a <![CDATA[Q+/= ]]>  </abc>"));
  EXPECT_ANY_THROW(xparseCdata(L"<abc>   <![CDATA[Q-o= ]]>  </abc>"));
}


TEST(parserTest, xmlns1) {
  // KeyInfo xmlns ohne Name (globaler ns)
  std::wstring x = LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Root>
  <XXXData Type="http://www.w3.org/2001/04/xmlenc#" xmlns:xenc="http://www.w3.org/2001/04/xmlenc#" xmlns:ds="http://www.w3.org/2000/09/xmldsig#">
    <ds:EncryptionMethod Algorithm="http://www.w3.org/2001/04/xmlenc#aes-256-cbc"/>
    <ds:KeyInfo xmlns="https://www.w3.org/2000/09/xmldsig#">
      <KeyName>Client</KeyName>
      <CipherData>
        <CipherValue>ABC</CipherValue>
      </CipherData>
   </ds:KeyInfo>
  </XXXData>
  <Wert>55</Wert>
</Root>
)";
  XParserW p(x);
  p.wait = true;
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "Client");
  EXPECT_EQ(p.lastKey, "Root|XXXData|http://www.w3.org/2000/09/xmldsig#EncryptionMethod|http://www.w3.org/2000/09/xmldsig#KeyInfo|https://www.w3.org/2000/09/xmldsig#KeyName|");
  p.lastKey.clear();
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "ABC");
  EXPECT_EQ(p.lastKey, "https://www.w3.org/2000/09/xmldsig#CipherData|https://www.w3.org/2000/09/xmldsig#CipherValue|");
  p.lastKey.clear();
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "55");
  EXPECT_EQ(p.lastKey, "Wert|");
}

TEST(parserTest, xmlns2) {
  // KeyInfo xmlns mit Name

  std::wstring x = LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Root>
  <XXXData Type="http://www.w3.org/2001/04/xmlenc#" xmlns:xenc="http://www.w3.org/2001/04/xmlenc#" xmlns:ds="http://www.w3.org/2000/09/xmldsig#">
    <ds:EncryptionMethod Algorithm="http://www.w3.org/2001/04/xmlenc#aes-256-cbc"/>
    <ds:KeyInfo xmlns:xx="https://www.w3.org/2000/09/xmlxxx#">
      <KeyName>Client</KeyName>
      <xx:CipherData>
        <CipherValue>ABC</CipherValue>
      </xx:CipherData>
   </ds:KeyInfo>
  </XXXData>
  <Wert>55</Wert>
</Root>
)";
  XParserW p(x);
  p.wait = true;
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "Client");
  EXPECT_EQ(p.lastKey, "Root|XXXData|http://www.w3.org/2000/09/xmldsig#EncryptionMethod|http://www.w3.org/2000/09/xmldsig#KeyInfo|KeyName|");
  p.lastKey.clear();
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "ABC");
  EXPECT_EQ(p.lastKey, "https://www.w3.org/2000/09/xmlxxx#CipherData|CipherValue|");
  p.lastKey.clear();
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "55");
  EXPECT_EQ(p.lastKey, "Wert|");
}

TEST(parserTest, xmlns4) {
  // KeyInfo xmlns mit Name

  std::wstring x = LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Root>
  <XXXData xmlns:ds="http://www.w3.org/2001/04/xmlenc#" >
    <xx:KeyInfo>
      <KeyName>Client</KeyName>
   </xx:KeyInfo>
</Root>
)";
  XParserW p(x);
  p.wait = true;
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "Client");
  EXPECT_EQ(p.lastKey, "Root|XXXData|xx:KeyInfo|KeyName|");
  p.lastKey.clear();
  ASSERT_ANY_THROW(p.parse());
}

TEST(parserTest, xmlns5) {
  std::wstring x = LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Root xmlns="http://www.mobs.org/test#" xmlns:ds="http://www.w3.org/2001/04/xmlenc#">
    <ds:KeyInfo>
      <KeyName>Client</KeyName>
   </ds:KeyInfo>
</Root>
)";
  XParserW p(x);
  p.wait = true;
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "Client");
  EXPECT_EQ(p.lastKey, "http://www.mobs.org/test#Root|http://www.w3.org/2001/04/xmlenc#KeyInfo|http://www.mobs.org/test#KeyName|");
  p.lastKey.clear();
  ASSERT_NO_THROW(p.parse());
}

TEST(parserTest, xmlns6) {
  std::wstring x = LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<?aaa?>
<Root>
    <ds:KeyInfo  xmlns:ds="http://www.w3.org/2001/04/xmlenc#" name="hallo">
      <KeyName>Client</KeyName>
      <leer/>
      <d:dings>Bums</d:dings>
   </ds:KeyInfo>
</Root>
)";
  XParserW p(x);
  p.setGlobalNs("d", "Hallo#");
  p.setGlobalNs("", "http://www.mobs.org/test#");
  p.wait = true;
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "Client");
  EXPECT_EQ(p.lastKey, "http://www.mobs.org/test#Root|http://www.w3.org/2001/04/xmlenc#KeyInfo|http://www.mobs.org/test#KeyName|");
  p.lastKey.clear();
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "Bums");
  EXPECT_EQ(p.lastKey, "http://www.mobs.org/test#leer|Hallo#dings|");
  p.wait = false;
  ASSERT_NO_THROW(p.parse());
}


TEST(parserTest, xmlEnc1) {

  std::wstring x = LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Root>
  <EncryptedData Type="http://www.w3.org/2001/04/xmlenc#Element" xmlns:xenc="http://www.w3.org/2001/04/xmlenc#"
                   xmlns:ds="http://www.w3.org/2000/09/xmldsig#">
        <xenc:EncryptionMethod Algorithm="http://www.w3.org/2001/04/xmlenc#aes-256-cbc"/>
        <ds:KeyInfo>
            <ds:KeyName>testkey</ds:KeyName>
            <xenc:CipherData>
                <xenc:CipherValue>
                    BBWjL8H2dnkjw14oxEvxT80mqdhdf9plarUSfCTJ/tZT4lBuIuR0vyE5IjnSpkvj8o0DuEKk4EKYlMcihTr13Gw=
                </xenc:CipherValue>
            </xenc:CipherData>
        </ds:KeyInfo>
       <ds:KeyInfo>
            <ds:KeyName>testkey2</ds:KeyName>
            <xenc:CipherData>
                <xenc:CipherValue>
                    BBWjL8H2dnkjw14oxEvxT80mqdhdf9plarUSfCTJ/tZT4lBuIuR0vyE5IjnSpkvj8o0DuEKk4EKYlMcihTr13Gw=
                </xenc:CipherValue>
            </xenc:CipherData>
        </ds:KeyInfo>
        <xenc:CipherData>
            <xenc:CipherValue>
                <Feld>blah</Feld>
            </xenc:CipherValue>
        </xenc:CipherData>
    </EncryptedData>
  <Wert>55</Wert>
</Root>
)";
  XParserReader p(x);
  p.wait = true;
  ASSERT_NO_THROW(p.parse());
  EXPECT_FALSE(p.encrypted());
  EXPECT_EQ(p.lastValue, "blah");
  EXPECT_EQ(p.lastKey, "Root|Feld|");
  p.lastKey.clear();
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "55");
  EXPECT_EQ(p.lastKey, "Wert|");

#if 0
  p.lastKey.clear();
  ASSERT_NO_THROW(p.parse());
  EXPECT_EQ(p.lastValue, "ABC");
  EXPECT_EQ(p.lastKey, "https://www.w3.org/2000/09/xmlxxx#CipherData|CipherValue|");
#endif

}


}

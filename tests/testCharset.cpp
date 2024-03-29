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
#include "converter.h"
#include "xmlout.h"
#include "xmlwriter.h"
#include "xmlread.h"

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;
using namespace mobs;

namespace {


class Person : virtual public mobs::ObjectBase {
public:
  ObjInit(Person);
  MemVar(std::string, name);
};

static Person person;

class XmlInput : public XmlReader {
public:
  XmlInput(wistream &str) : XmlReader(str) { }
  XmlInput(const std::string &str, bool charsetUnknown = false) : XmlReader(str, ConvObjFromStr(), charsetUnknown) { }

//  virtual void NullTag(const std::string &element) { EndTag(element); }
  virtual void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value) {
    LOG(LM_INFO, "attribute " << element << ":" << attribut << " = " << to_string(value));
  }
  virtual void Value(const std::wstring &value) {
    LOG(LM_INFO, "value " << to_string(value));
  }

  virtual void StartTag(const std::string &element) {
    LOG(LM_INFO, "start " << element);
    person.name("XXX");
    fill(&person);
  }
  virtual void EndTag(const std::string &element) {
    LOG(LM_INFO, "end " << element);
  }
  virtual void filled(ObjectBase *obj, const string &error) {
    LOG(LM_INFO, "filled " << obj->to_string() << (error.empty() ? " OK":" ERROR = ") << error);
  }

};


TEST(parserTest, charsetIso1) {
  Person p;
  p.name("Mähr");
  wofstream xout("pt.xml", ios::trunc);
  ASSERT_TRUE(xout.is_open());
  XmlWriter xf(xout, mobs::XmlWriter::CS_iso8859_1, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  xout.close();
  char buf[1000];
  FILE *f = ::fopen("pt.xml", "r");
  ASSERT_TRUE(f);
  size_t s = fread(buf, 1, 999, f);
  buf[s] = '\0';
  fclose(f);
  EXPECT_STREQ("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?><root><name>M\xE4hr</name></root>", buf);

  wifstream xin("pt.xml");
  ASSERT_TRUE(xin.is_open());
  XmlInput xr(xin);
  xr.setBase64(true);
  EXPECT_NO_THROW(xr.parse());
  xin.close();
  EXPECT_EQ("Mähr", person.name());
}

TEST(parserTest, charsetIso9) {
  Person p;
  p.name("Mähr\u015F");
  wofstream xout("pt.xml", ios::trunc);
  ASSERT_TRUE(xout.is_open());
  XmlWriter xf(xout, mobs::XmlWriter::CS_iso8859_9, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  xout.close();
  char buf[1000];
  FILE *f = ::fopen("pt.xml", "r");
  ASSERT_TRUE(f);
  size_t s = fread(buf, 1, 999, f);
  buf[s] = '\0';
  fclose(f);
  EXPECT_STREQ("<?xml version=\"1.0\" encoding=\"ISO-8859-9\" standalone=\"yes\"?><root><name>M\xE4hr\xfe</name></root>", buf);

  wifstream xin("pt.xml");
  ASSERT_TRUE(xin.is_open());
  XmlInput xr(xin);
  xr.setBase64(true);
  EXPECT_NO_THROW(xr.parse());
  xin.close();
  EXPECT_EQ("Mähr\u015f", person.name());
}

TEST(parserTest, charsetIso15) {
  Person p;
  p.name(u8"€Mähr");
  wofstream xout("pt.xml", ios::trunc);
  ASSERT_TRUE(xout.is_open());
  XmlWriter xf(xout, mobs::XmlWriter::CS_iso8859_15, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  xout.close();
  char buf[1000];
  FILE *f = ::fopen("pt.xml", "r");
  ASSERT_TRUE(f);
  size_t s = fread(buf, 1, 999, f);
  buf[s] = '\0';
  fclose(f);
  EXPECT_STREQ("<?xml version=\"1.0\" encoding=\"ISO-8859-15\" standalone=\"yes\"?><root><name>\xA4M\xE4hr</name></root>", buf);

  wifstream xin("pt.xml");
  ASSERT_TRUE(xin.is_open());
  XmlInput xr(xin);
  xr.setBase64(true);
  EXPECT_NO_THROW(xr.parse());
  xin.close();
  EXPECT_EQ(u8"€Mähr", person.name());
}

TEST(parserTest, charsetUtf8) {
  Person p;
  p.name("€Mähr");
  wofstream xout("pt.xml", ios::trunc);
  ASSERT_TRUE(xout.is_open());
  XmlWriter xf(xout, mobs::XmlWriter::CS_utf8, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  xout.close();
  char buf[1000];
  FILE *f = ::fopen("pt.xml", "r");
  ASSERT_TRUE(f);
  size_t s = fread(buf, 1, 999, f);
  buf[s] = '\0';
  fclose(f);
  EXPECT_STREQ(u8"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><root><name>€Mähr</name></root>", buf);

  wifstream xin("pt.xml");
  ASSERT_TRUE(xin.is_open());
  XmlInput xr(xin);
  xr.setBase64(true);
  EXPECT_NO_THROW(xr.parse());
  xin.close();
  EXPECT_EQ("€Mähr", person.name());
}

TEST(charsetTest, charsetUtf8bom) {
  Person p;
  p.name("€Mähr");
  wofstream xout("pt.xml", ios::trunc);
  ASSERT_TRUE(xout.is_open());
   //      XmlWriter xf(xout, XmlWriter::CS_iso8859_15, true); // CS_utf16_le CS_utf8
  XmlWriter xf(xout, mobs::XmlWriter::CS_utf8_bom, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  xout.close();
  char buf[1000];
  FILE *f = ::fopen("pt.xml", "r");
  ASSERT_TRUE(f);
  size_t s = fread(buf, 1, 999, f);
  buf[s] = '\0';
  fclose(f);
  EXPECT_STREQ(u8"\xef\xbb\xbf<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><root><name>€Mähr</name></root>", buf);

  wifstream xin("pt.xml");
  ASSERT_TRUE(xin.is_open());
  XmlInput xr(xin);
  xr.setBase64(true);
  EXPECT_NO_THROW(xr.parse());
  xin.close();
  EXPECT_EQ("€Mähr", person.name());

}

TEST(charsetTest, charsetUtf16le) {
  Person p;
  p.name("€Mähr");
  wofstream xout("pt.xml", ios::trunc);
  ASSERT_TRUE(xout.is_open());
   //      XmlWriter xf(xout, XmlWriter::CS_iso8859_15, true); // CS_utf16_le CS_utf8
  XmlWriter xf(xout, mobs::XmlWriter::CS_utf16_le, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  xout.close();
  char buf[1000];
  FILE *f = ::fopen("pt.xml", "r");
  ASSERT_TRUE(f);
  size_t s = fread(buf, 1, 999, f);
  buf[s] = '\0';
  fclose(f);
  EXPECT_STREQ("\xFF\xFE<", buf);
//  EXPECT_STREQ("\xFF\xFE<\0?\0x\0m\0l\0 \0v\0e\0r\0s\0i\0o\0n\0=\0\"\01\0.\00\0\"\0 \0e\0n\0c\0o\0d\0i\0n\0g\0=\0\"\0U\0T\0F\0-\01\06\0\"\0 \0s\0t\0a\0n\0d\0a\0l\0o\0n\0e\0=\0\"\0y\0e\0s\0\"\0?\0>\0<\0r\0o\0o\0t\0>\0<\0n\0a\0m\0e\0>\0\xAC M\0\xE4\0h\0r\0<\0/\0n\0a\0m\0e\0>\0<\0/\0r\0o\0o\0t\0>\0", buf);
  
  wifstream xin("pt.xml");
  ASSERT_TRUE(xin.is_open());
  XmlInput xr(xin);
  xr.setBase64(true);
  EXPECT_NO_THROW(xr.parse());
  xin.close();
  EXPECT_EQ("€Mähr", person.name());
  
}

TEST(charsetTest, charsetUtf16be) {
  Person p;
  p.name("€Mähr");
  wofstream xout("pt.xml", ios::trunc);
  ASSERT_TRUE(xout.is_open());
  XmlWriter xf(xout, mobs::XmlWriter::CS_utf16_be, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  xout.close();
  char buf[1000];
  FILE *f = ::fopen("pt.xml", "r");
  ASSERT_TRUE(f);
  size_t s = fread(buf, 1, 999, f);
  buf[s] = '\0';
  fclose(f);
  EXPECT_STREQ("\xFE\xFF", buf);
  EXPECT_EQ('<', buf[3]);
//  EXPECT_EQ("\xFE\xFF\0<\0?\0x\0m\0l\0 \0v\0e\0r\0s\0i\0o\0n\0=\0\"\01\0.\00\0\"\0 \0e\0n\0c\0o\0d\0i\0n\0g\0=\0\"\0U\0T\0F\0-\01\06\0\"\0 \0s\0t\0a\0n\0d\0a\0l\0o\0n\0e\0=\0\"\0y\0e\0s\0\"\0?\0>\0<\0r\0o\0o\0t\0>\0<\0n\0a\0m\0e\0> \xAC\0M\0\xE4\0h\0r\0<\0/\0n\0a\0m\0e\0>\0<\0/\0r\0o\0o\0t\0>", str.str());
  
  wifstream xin("pt.xml");
  ASSERT_TRUE(xin.is_open());
  XmlInput xr(xin);
  xr.setBase64(true);
  EXPECT_NO_THROW(xr.parse());
  xin.close();
  EXPECT_EQ("€Mähr", person.name());
  
}

TEST(charsetTest, charsetStrIso1) {
  Person p;
  p.name("Mähr");
  XmlWriter xf(mobs::XmlWriter::CS_iso8859_1, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  EXPECT_EQ("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?><root><name>M\xE4hr</name></root>", xf.getString());

  XmlInput xr(xf.getString(), true);
  EXPECT_NO_THROW(xr.parse());
  EXPECT_EQ("Mähr", person.name());

}

TEST(charsetTest, charsetStrIso9) {
  Person p;
  p.name("Mähr\u015F");
  XmlWriter xf(mobs::XmlWriter::CS_iso8859_9, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  EXPECT_EQ("<?xml version=\"1.0\" encoding=\"ISO-8859-9\" standalone=\"yes\"?><root><name>M\xE4hr\xfe</name></root>", xf.getString());
  
  XmlInput xr(xf.getString(), true);
  EXPECT_NO_THROW(xr.parse());
  EXPECT_EQ("Mähr\u015F", person.name());

}

TEST(charsetTest, charsetStrIso15) {
  Person p;
  p.name("€Mähr");
  XmlWriter xf(mobs::XmlWriter::CS_iso8859_15, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  EXPECT_EQ("<?xml version=\"1.0\" encoding=\"ISO-8859-15\" standalone=\"yes\"?><root><name>\xa4M\xE4hr</name></root>", xf.getString());
  
  XmlInput xr(xf.getString(), true);
  EXPECT_NO_THROW(xr.parse());
  EXPECT_EQ("€Mähr", person.name());

}

TEST(charsetTest, charsetStrUtf8) {
  Person p;
  p.name("€Mähr");
  XmlWriter xf(mobs::XmlWriter::CS_utf8, false);
  XmlOut xo(&xf, ConvObjToString().exportXml());
  xf.writeHead();
  p.traverse(xo);
  EXPECT_EQ("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><root><name>€Mähr</name></root>", xf.getString());

  XmlInput xr(xf.getString(), true);
  EXPECT_NO_THROW(xr.parse());
  EXPECT_EQ("€Mähr", person.name());

  XmlInput xr2(xf.getString());
  EXPECT_NO_THROW(xr.parse());
  EXPECT_EQ("€Mähr", person.name());
}

TEST(charsetTest, upperLower) {
  EXPECT_EQ(wstring(L"möèt"), mobs::toLower(L"MÖÈT"));
  EXPECT_EQ(wstring(L"MÖÈT"), mobs::toUpper(L"möèt"));
  EXPECT_EQ(string(u8"möètßa"), mobs::toLower(u8"MÖÈTßa"));
  EXPECT_EQ(string(u8"MÖÈTAß"), mobs::toUpper(u8"möètAß"));

}

TEST(charsetTest, uuid) {
  std::string uuid = gen_uuid_v4_p();
  EXPECT_EQ(36, uuid.length());
  EXPECT_NE(uuid, gen_uuid_v4_p());
  EXPECT_EQ('-', uuid[8]);
  EXPECT_EQ('-', uuid[13]);
  EXPECT_EQ('4', uuid[14]);
  EXPECT_EQ('-', uuid[18]);
  EXPECT_EQ('-', uuid[23]);
}

TEST(charsetTest, timeoff) {
  EXPECT_EQ("+02:00", timeOffsetToStr(120*60));
  EXPECT_EQ("-01:30", timeOffsetToStr(-90*60));
  EXPECT_EQ("+01:06", timeOffsetToStr(66*60));
  EXPECT_EQ("Z", timeOffsetToStr(0));

}

TEST(charsetTest, stringFormatter) {
  StringFormatter sf;

  ASSERT_EQ(1, sf.insertPattern(L"(\\d{1,5})-(\\d{1,3})", L"%1%05d.%2%03d"));
  ASSERT_EQ(2, sf.insertPattern(L"X(.{2,4})", L"%1%4s"));
  ASSERT_EQ(3, sf.insertPattern(L"Y(.{2,4})", L"%1%_4s"));
  ASSERT_EQ(4, sf.insertPattern(L"Z(.{2,4})", L"%1%-4s"));
  ASSERT_EQ(5, sf.insertPattern(L"V(.{2,4})", L"%1%--4s"));
  ASSERT_EQ(6, sf.insertPattern(L"A(.{2,4})", L"A%1%-_4SX"));
  std::wstring result;
  EXPECT_EQ(0, sf.format(L"Q234", result));
  EXPECT_EQ(1, sf.format(L"23-4", result));
  EXPECT_EQ(L"00023.004", result);
  EXPECT_EQ(2, sf.format(L"Xabc", result));
  EXPECT_EQ(L"abc ", result);
  EXPECT_EQ(3, sf.format(L"Yuu", result));
  EXPECT_EQ(L"uu__", result);
  EXPECT_EQ(4, sf.format(L"Zvv", result));
  EXPECT_EQ(L"  vv", result);
  EXPECT_EQ(5, sf.format(L"Vvv", result));
  EXPECT_EQ(L"--vv", result);
  EXPECT_EQ(6, sf.format(L"Avv", result));
  EXPECT_EQ(L"A__VVX", result);
}

TEST(converterTest, loginName) {
  string result;
  ASSERT_NO_THROW(result = getLoginName());
  cerr << "login " << result << endl;
}

TEST(converterTest, nodeName) {
  string result;
  ASSERT_NO_THROW(result = getNodeName());
  cerr << "node " << result << endl;
}

}


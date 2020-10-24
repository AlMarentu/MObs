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


#include "xmlwriter.h"
#include "xmlwriter.h"
#include "objgen.h"
#include "xmlout.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;


namespace {



TEST(writerTest, cdata1) {
  mobs::XmlWriter w;

  w.writeHead();
  w.writeTagBegin(L"aaa");
  w.writeCdata(L"");
  w.writeTagEnd();

  EXPECT_EQ(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<aaa><![CDATA[]]></aaa>
)", w.getString());
}

TEST(writerTest, cdata2) {
  mobs::XmlWriter w;

  w.writeHead();
  w.writeTagBegin(L"aaa");
  w.writeCdata(L"\"<a");
  w.writeTagEnd();

  EXPECT_EQ(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<aaa><![CDATA["<a]]></aaa>
)", w.getString());
}

TEST(writerTest, cdata3) {
  mobs::XmlWriter w;

  w.writeHead();
  w.writeTagBegin(L"aaa");
  w.writeCdata(L"<![CDATA[]]>");
  w.writeTagEnd();

  EXPECT_EQ(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<aaa><![CDATA[<![CDATA[]]]><![CDATA[]>]]></aaa>
)", w.getString());
}

TEST(writerTest, base64) {
  mobs::XmlWriter w;

  const char *s = "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark";
  std::vector<u_char> v;
  for (const char * cp = s; *cp; cp++)
    v.push_back(*cp);
  w.writeHead();
  w.writeTagBegin(L"aaa");
  w.writeBase64(v);
  w.writeTagEnd();

  EXPECT_EQ(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<aaa><![CDATA[UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJlbiwg
  Sm9naHVydCB1bmQgUXVhcms=]]></aaa>
)", w.getString());
}


class Adresse : virtual public mobs::ObjectBase {
public:
  ObjInit(Adresse);

  MemVar(int, lfdnr, XMLATTR);
  MemVar(std::string, strasse);
  MemVar(std::string, plz);
  MemVar(std::string, ort);
};



class Person : virtual public mobs::ObjectBase {
public:
  ObjInit(Person);

  MemVar(int, kundennr);
  MemVar(bool, firma);
  MemVar(std::string, name);
  MemVar(std::string, vorname, USENULL);
  MemObj(Adresse, adresse, USENULL);
  MemVarVector(std::string, hobbies);
};

TEST(writerTest, xml) {
  Person p;
  p.name("Schmied");
  p.adresse.ort("Dort");
  p.adresse.plz("12345");
  mobs::XmlWriter w;
  w.valueToken = L"V";
  mobs::XmlOut xo(&w, mobs::ConvObjToString().doIndent());
  w.writeHead();
  p.traverse(xo);

  EXPECT_EQ(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<root>
  <kundennr V="0"/>
  <firma V="false"/>
  <name V="Schmied"/>
  <vorname/>
  <adresse lfdnr="0">
    <strasse V=""/>
    <plz V="12345"/>
    <ort V="Dort"/>
  </adresse>
</root>
)", w.getString());

}

}


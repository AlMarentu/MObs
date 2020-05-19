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

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>





namespace mobs {


}

using namespace std;

namespace {


class Data : virtual public mobs::ObjectBase {
  public:
  ObjInit(Data);
  MemVar(int, id, KEYELEMENT1);
  MemVar(std::vector<u_char>, bin);
 };



TEST(blobTest, create) {
  Data d;
  
  string test = "BINARY DATA";
  d.id(1);
  std::vector<u_char> v;
  copy(test.cbegin(), test.cend(), back_inserter(v));
  d.bin(v);
  EXPECT_EQ("{id:1,bin:\"QklOQVJZIERBVEE=\"}", d.to_string());
  
  Data d0;
  EXPECT_NO_THROW(mobs::string2Obj(d.to_string(), d0));
  string r;
  for (auto c:d0.bin())
    r += c;
  EXPECT_EQ(test, r);
       
//  mobs::string2Obj(u8"{aa:2,bb:7,cc:12,ee:22}", o0);
//  mobs::string2Obj(u8"{id:12,xx:99}", o2);
//  mobs::string2Obj(u8"{i1:567,xx:\"qwert\",oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}", o1);
//  m.id(123);
//  m.xx(543);
//  mobs::ObjectBase *bp = o0.createMe(nullptr);
//  EXPECT_EQ("Obj0", bp->typeName());
//  EXPECT_EQ("Obj0", o0.typeName());
//
//  EXPECT_NO_THROW(bp->doCopy(o0));
//  EXPECT_NO_THROW(m.elements[1](o1));
//  EXPECT_EQ(u8"{id:123,xx:543,elements:[{Obj0:{aa:2,bb:7,cc:12,dd:0,ee:22}},{Obj1:{i1:567,xx:\"qwert\",zz:0,oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}},{Obj2:{id:0,xx:0}}]}", m.to_string());

//  std::cerr << m.to_string() << std::endl;

}
  TEST(blobTest, lang) {
    Data d;

    u_char i = 7;
    d.id(1);
    std::vector<u_char> v(1024);
    for (auto &c:v)
      c = i = u_char(i * i + 1);

    d.bin(v);
//    EXPECT_EQ("{id:1,bin:\"QklOQVJZIERBVEE=\"}", d.to_string());
    EXPECT_EQ("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><root><id>1</id><bin><![CDATA[MsWapVqlWqVapVq", d.to_string(mobs::ConvObjToString().exportXml()).substr(0, 100));
    string r = d.to_string(mobs::ConvObjToString().exportXml().doIndent());
//    cerr << r << std::endl;
    EXPECT_EQ("\n  pVqlWqVapVqlWqVapVqlWqVapVqlWqVapVqlWqVapVqlWqVapVqlWqVapVqlWqVapVql\n  WqVapQ==]]></bin>\n</root>\n", r.substr(r.length() -100));
    

    

  }
 
}

               

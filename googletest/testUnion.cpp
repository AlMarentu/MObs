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
#include "union.h"
#include "union.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>





namespace mobs {


}

using namespace std;

namespace {

class Obj0;
class Obj1;
class Obj2;

class BaseObj : virtual public mobs::ObjectBase {
public:
  ObjInit(BaseObj);
  virtual ~BaseObj() {};
  virtual Obj0 &toObj0() { throw runtime_error("invalid cast"); };
  virtual Obj1 &toObj1() { throw runtime_error("invalid cast"); };
  virtual Obj2 &toObj2() { throw runtime_error("invalid cast"); };
};




class Obj0 : virtual public BaseObj, virtual public mobs::ObjectBase {
public:
  ObjInit(Obj0);

  MemVar(int, aa);
  MemVar(int, bb);
  MemVar(int, cc);
  MemVar(int, dd);
  MemVar(int, ee);
  virtual Obj0 &toObj0() { return *this; };
};
ObjRegister(Obj0);

class Obj1 : virtual public BaseObj, virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj1);
  MemVar(int, i1,);
  MemVar(std::string, xx);
  MemVar(int, zz);
  MemObj(Obj0, oo, USENULL KEYELEMENT2);
  virtual Obj1 &toObj1() { return *this; };

};
ObjRegister(Obj1);

class Obj2 : virtual BaseObj, virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj2);
  MemVar(int, id, KEYELEMENT1);
  MemVar(int, xx);
  virtual Obj2 &toObj2() { return *this; };
};
ObjRegister(Obj2);


class Master : virtual public mobs::ObjectBase {
  public:
  ObjInit(Master);
  MemVar(int, id, KEYELEMENT1);
  MemVar(int, xx);
  MemVector(mobs::MobsUnion<BaseObj>, elements, USENULL);
};
//ObjRegister(Master);



TEST(unionTest, create) {
  Master m;
  Obj0 o0;
  Obj1 o1;
  Obj2 o2;
  
  mobs::string2Obj(u8"{aa:2,bb:7,cc:12,ee:22}", o0);
  mobs::string2Obj(u8"{id:12,xx:99}", o2);
  mobs::string2Obj(u8"{i1:567,xx:\"qwert\",oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}", o1);
  m.id(123);
  m.xx(543);
  mobs::ObjectBase *bp = o0.createMe(nullptr);
  EXPECT_EQ("Obj0", bp->typName());
  EXPECT_EQ("Obj0", o0.typName());

  EXPECT_NO_THROW(bp->doCopy(o0));
  EXPECT_NO_THROW(m.elements[1](o1));
  EXPECT_NO_THROW(m.elements[2].setType("Obj2"));
  EXPECT_EQ(u8"{id:123,xx:543,elements:[null,{Obj1:{i1:567,xx:\"qwert\",zz:0,oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}},{Obj2:{id:0,xx:0}}]}", m.to_string());
  EXPECT_NO_THROW(m.elements[0].activate());
  EXPECT_EQ(u8"{id:123,xx:543,elements:[{},{Obj1:{i1:567,xx:\"qwert\",zz:0,oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}},{Obj2:{id:0,xx:0}}]}", m.to_string());
//  std::cerr << m.to_string() << std::endl;
  EXPECT_NO_THROW(m.elements[0](o0));
  EXPECT_EQ(u8"{id:123,xx:543,elements:[{Obj0:{aa:2,bb:7,cc:12,dd:0,ee:22}},{Obj1:{i1:567,xx:\"qwert\",zz:0,oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}},{Obj2:{id:0,xx:0}}]}", m.to_string());

//  std::cerr << m.to_string() << std::endl;


 
}

TEST(unionTest, read) {
  Master m;
  std::string j1 = u8"{id:123,xx:543,elements:[{Obj0:{aa:2,bb:7,cc:12,dd:0,ee:22}},{Obj1:{i1:567,xx:\"qwert\",zz:0,oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}},{Obj2:{id:0,xx:0}}]}";
  std::string j2 = u8"{id:123,xx:543,elements:[{Obj2:{id:0,xx:0}},null,{Obj0:{aa:2,bb:7,cc:12,dd:0,ee:22}},{Obj1:{i1:567,xx:\"qwert\",zz:0,oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}}]}";
  EXPECT_NO_THROW(mobs::string2Obj(j1, m));
  EXPECT_EQ(j1, m.to_string());
//  LOG(LM_INFO, "======================================== " << m.elements.size());
  EXPECT_NO_THROW(mobs::string2Obj(j2, m));
//  std::cerr << m.to_string() << std::endl;
  EXPECT_EQ(j2, m.to_string());
  m.clear();
  EXPECT_NO_THROW(mobs::string2Obj(j2, m));
//  std::cerr << m.to_string() << std::endl;
  EXPECT_EQ(j2, m.to_string());
}


TEST(unionTest, access) {
  Master m;
  std::string j1 = u8"{id:123,xx:543,elements:[{Obj0:{aa:2,bb:7,cc:12,dd:0,ee:22}},{Obj1:{i1:567,xx:\"qwert\",zz:0,oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}},null,{Obj2:{id:0,xx:0}}]}";
  EXPECT_NO_THROW(mobs::string2Obj(j1, m));
  EXPECT_FALSE(m.elements[2]);
  // achtung operator -> ohne überprüfung
  ASSERT_TRUE(m.elements[1]);
  EXPECT_EQ(567, m.elements[1]->toObj1().i1());
  EXPECT_EQ(567, m.elements[1]().toObj1().i1());
  EXPECT_ANY_THROW(m.elements[1]().toObj2().xx());
  EXPECT_NO_THROW(m.elements[1]->toObj1().i1(12));
  
}


TEST(unionTest, copy) {
  Master m;
  std::string j1 = u8"{id:123,xx:543,elements:[{Obj0:{aa:2,bb:7,cc:12,dd:0,ee:22}},{Obj1:{i1:567,xx:\"qwert\",zz:0,oo:{aa:7,bb:6,cc:5,dd:4,ee:3}}},null,{Obj2:{id:0,xx:0}}]}";
  EXPECT_NO_THROW(mobs::string2Obj(j1, m));

  Master m2 = m;
  EXPECT_EQ(j1, m2.to_string());

}

}


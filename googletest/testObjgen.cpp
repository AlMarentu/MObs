//
//  testObjgen.cpp
//  mobs
//
//  Created by Matthias Lautner on 19.03.20.
//  Copyright © 2020 Matthias Lautner. All rights reserved.
//

#include "objgen.h"
#include "objgen.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>



namespace {
using namespace std;

class ObjDump : virtual public ObjTrav {
public:
  virtual void doObjBeg(ObjTrav &ot, ObjectBase &obj)
  {
    if (not fst)
      res << ",";
    fst = true;
    if (not obj.name().empty())
        res << obj.name() << ":";
    res << "{";
  };
  virtual void doObjEnd(ObjTrav &ot, ObjectBase &obj)
  {
    res << "}";
    fst = false;
  };
  virtual void doArrayBeg(ObjTrav &ot, MemBaseVector &vec)
  {
    if (not fst)
      res << ",";
    fst = false;
    res << vec.name() << ":[";
  };
  virtual void doArrayEnd(ObjTrav &ot, MemBaseVector &vec)
  {
    res << "]";
    fst = false;
  };
  virtual void doMem(ObjTrav &ot, MemberBase &mem)
  {
    if (not fst)
      res << ",";
    fst = false;
    res << boolalpha << mem.name() << ":";
    if (mem.is_specialized())
      mem.strOut(res);
    else
      res << '"' << mem.toStr() << '"';
  };
  std::string result() const { return res.str(); };
private:
  bool fst = true;
  stringstream res;
};

string to_string(ObjectBase &obj) {
  ObjDump od;
  obj.traverse(od);
  return od.result();
}

class Part : virtual public ObjectBase {
public:
  ObjInit(Part);
  
  MemVar(string, vorn);
  MemVar(string, hinten);
};
//ObjRegister(Part); unnötig,da kein Basis-Objekt

class Info : virtual public ObjectBase {
public:
  ObjInit(Info);
  
  MemVar(int, otto);
  MemVar(int, peter);
  MemVar(bool, pool);
  ObjVar(Part, pims);
  ObjVar(Part, bums);
  
  MemVar(string, mom);
  MemVector(string, susi);
  ObjVector(Part, luzifer);
  MemVector(string, friederich);
  
  virtual string objName() const { return typName() + "." + mom() + "." + std::to_string(otto()); };
  virtual void init() { otto.nullAllowed(true); pims.nullAllowed(true); pims.setNull(true);
    luzifer.nullAllowed(true); luzifer.setNull(true); keylist << peter << otto; };
};
ObjRegister(Info);



TEST(objgenTest, leer) {
  Info info;
  EXPECT_EQ(0, info.otto());
  EXPECT_EQ(0, info.peter());
  EXPECT_EQ(false, info.pool());
  EXPECT_EQ("", info.mom());
  EXPECT_EQ("Info", info.typName());
}

TEST(objgenTest, Negative) {
  Info info;
  Info info2;
  info.otto(7);
  info.peter(2);
  info.mom("Das war ein");
  cerr << to_string(info) << endl;
  info2.otto(99);
  info2.peter(105);
  info2.mom("HAL");
  
  EXPECT_EQ(7, info.otto());
  EXPECT_EQ(2, info.peter());
  EXPECT_EQ("Info", info.typName());
  
  
}

}


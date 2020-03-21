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

using namespace std;


namespace {

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
  
  virtual string objName() const { return typName() + "." + mom() + "." + mobs::to_string(otto()); };
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

class DataTypes : virtual public ObjectBase {
public:
  ObjInit(DataTypes);
  
  MemVar(bool, Bool);
  MemVar(char, Char);
  MemVar(char16_t, Char16_t);
  MemVar(char32_t, Char32_t);
  MemVar(wchar_t, Wchar_t);
  MemVar(signed char, SignedChar);
  MemVar(short int, ShortInt);
  MemVar(int, Int);
  MemVar(long int, LongInt);
  MemVar(long long int, LongLongInt);
  MemVar(unsigned char, UnsignedChar);
  MemVar(unsigned short int, UnsignedShortInt);
  MemVar(unsigned int, UnsignedInt);
  MemVar(unsigned long int, UnsignedLongLong);
  MemVar(unsigned long long int, UnsignedLongLongInt);
  MemVar(float, Float);
  MemVar(double, Double);
  MemVar(long double, LongDouble);
  MemVar(string, String);
  MemVar(wstring, Wstring);
  MemVar(u32string, U32string);
};

TEST(objgenTest, Types) {
  DataTypes dt;
  EXPECT_FALSE(dt.Bool.is_chartype());
  EXPECT_TRUE(dt.Char.is_chartype());
  EXPECT_TRUE(dt.Char16_t.is_chartype());
  EXPECT_TRUE(dt.Char32_t.is_chartype());
  EXPECT_TRUE(dt.Wchar_t.is_chartype());
  EXPECT_TRUE(dt.SignedChar.is_chartype());
  EXPECT_FALSE(dt.ShortInt.is_chartype());
  EXPECT_FALSE(dt.Int.is_chartype());
  EXPECT_FALSE(dt.LongInt.is_chartype());
  EXPECT_TRUE(dt.UnsignedChar.is_chartype());
  EXPECT_FALSE(dt.UnsignedShortInt.is_chartype());
  EXPECT_FALSE(dt.UnsignedInt.is_chartype());
  EXPECT_FALSE(dt.UnsignedLongLong.is_chartype());
  EXPECT_FALSE(dt.UnsignedLongLongInt.is_chartype());
  EXPECT_FALSE(dt.Float.is_chartype());
  EXPECT_FALSE(dt.Double.is_chartype());
  EXPECT_FALSE(dt.LongDouble.is_chartype());
  EXPECT_TRUE(dt.String.is_chartype());
  EXPECT_TRUE(dt.Wstring.is_chartype());
  EXPECT_TRUE(dt.U32string.is_chartype());
  string leer = R"({Bool:false,Char:"",Char16_t:"",Char32_t:"",Wchar_t:"",SignedChar:"",ShortInt:0,Int:0,LongInt:0,LongLongInt:0,UnsignedChar:"",UnsignedShortInt:0,UnsignedInt:0,UnsignedLongLong:0,UnsignedLongLongInt:0,Float:0,Double:0,LongDouble:0,String:"",Wstring:"",U32string:""})";
  EXPECT_EQ(leer, mobs::to_string(dt));
//  cerr << to_string(dt) << endl << leer << endl;
}

TEST(objgenTest, Negative) {
  Info info;
  Info info2;
  info.otto.setNull(true);
  info.peter(2);
  info.mom("Das war ein ßäöü <>\"' ss \"#  ö");
  cerr << mobs::to_string(info) << endl;
  info2.otto(99);
  info2.peter(105);
  info2.mom("HAL");
  
  EXPECT_TRUE(info.otto.isNull());
  EXPECT_EQ(2, info.peter());
  EXPECT_EQ("Info", info.typName());
  
//  cerr << "XXX " << numeric_limits<int>::digits << endl;
//  cerr << "XXX " << numeric_limits<char>::digits << endl;
//  cerr << "XXX " << numeric_limits<char32_t>::digits << endl;
//  cerr << "XXX " << numeric_limits<bool>::digits << endl;
}

}


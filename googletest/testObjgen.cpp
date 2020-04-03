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
#include "objgen.h"
#include "jsonparser.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;


namespace {




class Kontakt : virtual public mobs::ObjectBase {
public:
  ObjInit(Kontakt);
  enum device { fax, sms, mobil, privat, arbeit };
  
  /// Art des Kontaktes Fax/Mobil/SMS
  MemVar(int, art);
  /// Nummer
  MemVar(string, number);
};
//ObjRegister(Kontakt); unnötig,da kein Basis-Objekt

//Kontakt::device Kontakt::device(unsigned int i) { return static_cast<Kontakt::device>(i); };

// std::ostream& operator<<(std::ostream &s, Kontakt::device t) { s << int(t); return s; }
//template <>  bool string2x(const std::string &str, Kontakt::device &t)
//{
//  int i;
//  if (not mobs::string2x(str, i))
//    return false;
//  return true;
//};


class Adresse : virtual public mobs::ObjectBase {
public:
  ObjInit(Adresse);
  
  MemVar(std::string, strasse);
  MemVar(std::string, plz);
  MemVar(std::string, ort);
};
//ObjRegister(Kontakt); unnötig,da kein Basis-Objekt



class Person : virtual public mobs::ObjectBase {
  public:
  ObjInit(Person);
  
  MemVar(int, kundennr);
  MemVar(bool, firma);
  MemVar(std::string, name);
  MemVar(std::string, vorname);
  ObjVar(Adresse, adresse);
  ObjVector(Kontakt, kontakte);
  MemVector(std::string, hobbies);

  virtual void init() { adresse.nullAllowed(true); name.nullAllowed(true); vorname.setNull(true); };
};
ObjRegister(Person);

//
//class Info : virtual public mobs::ObjectBase {
//public:
//  ObjInit(Info);
//
//  MemVar(int, otto);
//  MemVar(int, peter);
//  MemVar(bool, pool);
//  ObjVar(Adresse, pims);
//  ObjVar(Adresse, bums);
//
//  MemVar(string, mom);
//  MemVector(string, susi);
//  MemVector(string, friederich);
//
//  virtual string objName() const { return typName() + "." + mom() + "." + mobs::to_string(otto()); };
//  virtual void init() { otto.nullAllowed(true); pims.nullAllowed(true); pims.setNull(true);
//    luzifer.nullAllowed(true); luzifer.setNull(true); keylist << peter << otto; };
//};




TEST(objgenTest, leer) {
  Person info;
  EXPECT_EQ(0, info.kundennr());
  EXPECT_EQ(false, info.firma());
  EXPECT_EQ("", info.name());
  EXPECT_EQ("Person", info.typName());
}

//Alle MemVar Datentypen Testen
class DataTypes : virtual public mobs::ObjectBase {
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
  MemVar(u16string, U16string);
  MemVar(u32string, U32string);
};

TEST(objgenTest, chartype) {
  DataTypes dt;
//  methode charrtyp
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
  EXPECT_TRUE(dt.U16string.is_chartype());
  EXPECT_TRUE(dt.U32string.is_chartype());
}

TEST(objgenTest, emptyVars) {
  DataTypes dt;
  EXPECT_EQ(false, dt.Bool());
  EXPECT_EQ('\0', dt.Char());
  EXPECT_EQ(u'\0', dt.Char16_t());
  EXPECT_EQ(U'\0', dt.Char32_t());
  EXPECT_EQ(L'\0', dt.Wchar_t());
  EXPECT_EQ('\0', dt.SignedChar());
  EXPECT_EQ(0, dt.ShortInt());
  EXPECT_EQ(0, dt.Int());
  EXPECT_EQ(0, dt.LongInt());
  EXPECT_EQ(0, dt.LongLongInt());
  EXPECT_EQ('\0', dt.UnsignedChar());
  EXPECT_EQ(0, dt.UnsignedShortInt());
  EXPECT_EQ(0, dt.UnsignedInt());
  EXPECT_EQ(0, dt.UnsignedLongLong());
  EXPECT_EQ(0, dt.UnsignedLongLongInt());
  EXPECT_EQ(0.0, dt.Float());
  EXPECT_EQ(0.0, dt.Double());
  EXPECT_EQ(0.0, dt.LongDouble());
  EXPECT_EQ(u8"", dt.String());
  EXPECT_EQ(L"", dt.Wstring());
  EXPECT_EQ(u"", dt.U16string());
  EXPECT_EQ(U"", dt.U32string());
}


TEST(objgenTest, objDump) {
  DataTypes dt;
  string leer = R"({Bool:false,Char:"",Char16_t:"",Char32_t:"",Wchar_t:"",SignedChar:"",ShortInt:0,Int:0,LongInt:0,LongLongInt:0,UnsignedChar:"",UnsignedShortInt:0,UnsignedInt:0,UnsignedLongLong:0,UnsignedLongLongInt:0,Float:0,Double:0,LongDouble:0,String:"",Wstring:"",U16string:"",U32string:""})";
  EXPECT_EQ(leer, mobs::to_string(dt));
//  cerr << to_string(dt) << endl << leer << endl;
}

TEST(objgenTest, inserter) {
  Person person;
  mobs::ObjectNavigator oi;
  oi.pushObject(person);
  EXPECT_TRUE(oi.find("kundennr"));
  EXPECT_TRUE(oi.find("adresse"));
  EXPECT_TRUE(oi.find("hobbies[4]"));
  EXPECT_TRUE(oi.find("kontakte[2].number"));
  EXPECT_FALSE(oi.find("kontakte[].number"));
  EXPECT_FALSE(oi.find("kontakte[2]number"));
  EXPECT_FALSE(oi.find("kontakte."));
  EXPECT_FALSE(oi.find(""));
}

TEST(objgenTest, setVars) {
  DataTypes dt;
  dt.Bool(true);
  EXPECT_EQ(true, dt.Bool());
  dt.Char('a');
  EXPECT_EQ('a', dt.Char());
  dt.Char16_t(u'b');
  EXPECT_EQ(u'b', dt.Char16_t());
  dt.Char32_t(U'c');
  EXPECT_EQ(U'c', dt.Char32_t());
  dt.Wchar_t(L'd');
  EXPECT_EQ(L'd', dt.Wchar_t());
  dt.SignedChar('e');
  EXPECT_EQ('e', dt.SignedChar());
  dt.ShortInt(42);
  EXPECT_EQ(42, dt.ShortInt());
  dt.Int(-9876543);
  EXPECT_EQ(-9876543, dt.Int());
  dt.LongInt(-45454545);
  EXPECT_EQ(-45454545, dt.LongInt());
  dt.LongLongInt(-34343434343434);
  EXPECT_EQ(-34343434343434, dt.LongLongInt());
  dt.UnsignedChar('f');
  EXPECT_EQ('f', dt.UnsignedChar());
  dt.UnsignedShortInt(999);
  EXPECT_EQ(999, dt.UnsignedShortInt());
  dt.UnsignedInt(88888);
  EXPECT_EQ(88888, dt.UnsignedInt());
  dt.UnsignedLongLong(109876543);
  EXPECT_EQ(109876543, dt.UnsignedLongLong());
  dt.UnsignedLongLongInt(1234567890);
  EXPECT_EQ(1234567890, dt.UnsignedLongLongInt());
  dt.Float(-21.3);
  EXPECT_FLOAT_EQ(-21.3, dt.Float());
  dt.Double(0.00001);
  EXPECT_DOUBLE_EQ(0.00001, dt.Double());
  dt.LongDouble(123.456);
  EXPECT_DOUBLE_EQ(123.456, dt.LongDouble());
  dt.String(u8"Anton");
  EXPECT_EQ(u8"Anton", dt.String());
  dt.Wstring(L"Berti");
  EXPECT_EQ(L"Berti", dt.Wstring());
  dt.U16string(u"Conni");
  EXPECT_EQ(u"Conni", dt.U16string());
  dt.U32string(U"Det");
  EXPECT_EQ(U"Det", dt.U32string());
  
  // nochmals Dump
  string inhalt = R"({Bool:true,Char:"a",Char16_t:"b",Char32_t:"c",Wchar_t:"d",SignedChar:"e",ShortInt:42,Int:-9876543,LongInt:-45454545,LongLongInt:-34343434343434,UnsignedChar:"f",UnsignedShortInt:999,UnsignedInt:88888,UnsignedLongLong:109876543,UnsignedLongLongInt:1234567890,Float:-21.3,Double:1e-05,LongDouble:123.456,String:"Anton",Wstring:"Berti",U16string:"Conni",U32string:"Det"})";
  EXPECT_EQ(inhalt, mobs::to_string(dt));

  // und das Ganze zurücklesen
  DataTypes dt2;
  ASSERT_NO_THROW(string2Obj(inhalt, dt2));
  EXPECT_EQ(inhalt, mobs::to_string(dt2));
}


TEST(objgenTest, setnull) {
  Person info;
  info.adresse.setNull(true);
  info.kundennr(2);
  info.name(u8"Das war ein ßäöü <>\"' ss \"#  ö");
//  cerr << mobs::to_string(info) << endl;

  EXPECT_TRUE(info.adresse.isNull());
  EXPECT_EQ(2, info.kundennr());
  EXPECT_EQ("Adresse", info.adresse.typName());
  EXPECT_EQ(R"({kundennr:2,firma:false,name:"Das war ein ßäöü <>\"' ss \"#  ö",vorname:"",adresse:null,kontakte:[],hobbies:[]})", mobs::to_string(info));
  info.name(u8"John");
  info.adresse.ort(u8"Berlin");
  // null wird aufgehoben wenn ein Unterelement gesetzt wird - rekursiv
  EXPECT_FALSE(info.adresse.isNull());
  EXPECT_EQ(R"({kundennr:2,firma:false,name:"John",vorname:"",adresse:{strasse:"",plz:"",ort:"Berlin"},kontakte:[],hobbies:[]})", mobs::to_string(info));
  EXPECT_EQ(u8"Berlin", info.adresse.ort());
}

TEST(objgenTest, Vectors) {
  Person info;
  info.adresse.setNull(true);
  info.kundennr(44);
  info.name(u8"Peter");
  info.kontakte[4].art(Kontakt::mobil);
  info.kontakte[4].number("+40 0000 1111 222");
  info.hobbies[1]("Piano");

  EXPECT_EQ("Adresse", info.adresse.typName());
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:null,kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", mobs::to_string(info));
  info.adresse.setNull(false);
//  cerr << mobs::to_string(info) << endl;
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", mobs::to_string(info));
// dito mit to_json
  EXPECT_EQ(R"({"kundennr":44,"firma":false,"name":"Peter","vorname":"","adresse":{"strasse":"","plz":"","ort":""},"kontakte":[{"art":0,"number":""},{"art":0,"number":""},{"art":0,"number":""},{"art":0,"number":""},{"art":2,"number":"+40 0000 1111 222"}],"hobbies":["","Piano"]})", mobs::to_json(info));

}

TEST(objgenTest, Pointer) {
  EXPECT_EQ(nullptr, mobs::ObjectBase::createObj("XXX"));

  Person *ip = dynamic_cast<Person *>(mobs::ObjectBase::createObj("Person"));
  ASSERT_NE(nullptr, ip);
  EXPECT_EQ("Person", ip->typName());
  ASSERT_NO_THROW(mobs::string2Obj( R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", *ip));
  // Gegenprobe
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", mobs::to_string(*ip));

  // Test ob Objekte nativ zurückgegeben werden und keine Kopien
  EXPECT_EQ(&ip->kontakte, ip->getVecInfo("kontakte"));
  EXPECT_EQ(&ip->kontakte[3], dynamic_cast<Kontakt *>(ip->kontakte.getObjInfo(3)));
  EXPECT_EQ(static_cast<mobs::ObjectBase *>(&ip->kontakte[3]), ip->kontakte.getObjInfo(3));
  EXPECT_EQ(ip, dynamic_cast<mobs::ObjectBase *>(ip));
  EXPECT_EQ(&ip->kontakte[0].number, dynamic_cast<mobs::Member<string> *>(ip->kontakte[0].getMemInfo("number")));
  EXPECT_EQ(&ip->kontakte[1].art, dynamic_cast<mobs::Member<int> *>(ip->kontakte[1].getMemInfo("art")));
//  cerr << typeid(&ip->kontakte[0].number).name() << endl;
//  cerr << typeid(ip->kontakte[0].getMemInfo("number")).name() << endl;
}

TEST(objgenTest, getSetVar) {
  Person p;
  mobs::ObjectBase *op = &p;
  EXPECT_TRUE(op->setVariable("kontakte[3].number", "00-00-00"));
  EXPECT_EQ("00-00-00", p.kontakte[3].number());
  EXPECT_TRUE(op->setVariable("kontakte[1].number", "---"));
  EXPECT_EQ("---", p.kontakte[1].number());
  EXPECT_EQ("00-00-00", op->getVariable("kontakte[3].number"));
  bool r = false;
  EXPECT_EQ("---", op->getVariable("kontakte[1].number", &r));
  EXPECT_TRUE(r);
  EXPECT_EQ("", op->getVariable("kontakte[1].bee", &r));
  EXPECT_FALSE(r);
//  std::cerr << to_string(p) << std::endl;
  
}

}


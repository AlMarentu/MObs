//#pragma ide diagnostic ignored "cert-err58-cpp"
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

//enum device { fax, sms, mobil, privat, arbeit };
//namespace mobs {std::string to_string(enum device t);}




namespace {

MOBS_ENUM_DEF(device, fax, sms, mobil, privat, arbeit );

MOBS_ENUM_VAL(device, "fax", "sms", "mobil", "privat", "arbeit");


class Kontakt : virtual public mobs::ObjectBase {
public:
  ObjInit(Kontakt);
//  enum device { fax, sms, mobil, privat, arbeit };

  /// Art des Kontaktes Fax/Mobil/SMS
//  MemEnumVar(enum device, art);
  MemMobsEnumVar(device, art);
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
  MemObj(Adresse, adresse, USENULL);
  MemVector(Kontakt, kontakte);
//  MemVector(MemVarType(std::string), hobbies);
  MemVarVector(std::string, hobbies);

  void init() override {  };
};
ObjRegister(Person)

//
//class Info : virtual public mobs::ObjectBase {
//public:
//  ObjInit(Info);
//
//  MemVar(int, otto);
//  MemVar(int, peter);
//  MemVar(bool, pool);
//  MemObj(Adresse, pims);
//  MemObj(Adresse, bums);
//
//  MemVar(string, mom);
//  MemVarVector(string, susi);
//  MemVarVector(string, friederich);
//
//  virtual string objName() const { return typeName() + "." + mom() + "." + mobs::to_string(otto()); };
//  virtual void init() { otto.nullAllowed(true); pims.nullAllowed(true); pims.setNull(true);
//    luzifer.nullAllowed(true); luzifer.setNull(true); keylist << peter << otto; };
//};




TEST(objgenTest, leer) {
  Person info;
  EXPECT_EQ(0, info.kundennr());
  EXPECT_EQ(false, info.firma());
  EXPECT_EQ("", info.name());
  EXPECT_EQ("Person", info.getObjectName());
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
  auto cth = mobs::ConvToStrHint(false);
//  methode charrtyp
  EXPECT_FALSE(dt.Bool.is_chartype(cth));
  EXPECT_TRUE(dt.Char.is_chartype(cth));
  EXPECT_TRUE(dt.Char16_t.is_chartype(cth));
  EXPECT_TRUE(dt.Char32_t.is_chartype(cth));
  EXPECT_TRUE(dt.Wchar_t.is_chartype(cth));
  EXPECT_TRUE(dt.SignedChar.is_chartype(cth));
  EXPECT_FALSE(dt.ShortInt.is_chartype(cth));
  EXPECT_FALSE(dt.Int.is_chartype(cth));
  EXPECT_FALSE(dt.LongInt.is_chartype(cth));
  EXPECT_TRUE(dt.UnsignedChar.is_chartype(cth));
  EXPECT_FALSE(dt.UnsignedShortInt.is_chartype(cth));
  EXPECT_FALSE(dt.UnsignedInt.is_chartype(cth));
  EXPECT_FALSE(dt.UnsignedLongLong.is_chartype(cth));
  EXPECT_FALSE(dt.UnsignedLongLongInt.is_chartype(cth));
  EXPECT_FALSE(dt.Float.is_chartype(cth));
  EXPECT_FALSE(dt.Double.is_chartype(cth));
  EXPECT_FALSE(dt.LongDouble.is_chartype(cth));
  EXPECT_TRUE(dt.String.is_chartype(cth));
  EXPECT_TRUE(dt.Wstring.is_chartype(cth));
  EXPECT_TRUE(dt.U16string.is_chartype(cth));
  EXPECT_TRUE(dt.U32string.is_chartype(cth));
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

//template<typename T>
//operator mobs::Member<T, mobs::StrConv<T>>(const T &s) { return mobs::Member<T, mobs::StrConv<T>>(s); };


TEST(objgenTest, setnull) {
  Person info;
  info.adresse.forceNull();
  info.kundennr(2);
  info.name(u8"Das war ein ßäöü <>\"' ss \"#  ö");
//  cerr << mobs::to_string(info) << endl;

  EXPECT_TRUE(info.adresse.isNull());
  EXPECT_EQ(2, info.kundennr());
  EXPECT_EQ("Adresse", info.adresse.getObjectName());
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
  info.adresse.forceNull();
  info.kundennr(44);
  info.name(u8"Peter");
  info.kontakte[4].art(mobil);
  info.kontakte[4].number("+40 0000 1111 222");
  info.hobbies[1]("Piano");

  EXPECT_EQ("Adresse", info.adresse.getObjectName());
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:null,kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", info.to_string(mobs::ConvObjToString().exportCompact()));
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:null,kontakte:[{art:"fax",number:""},{art:"fax",number:""},{art:"fax",number:""},{art:"fax",number:""},{art:"mobil",number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", mobs::to_string(info));
  info.adresse.setEmpty();
//  cerr << mobs::to_string(info) << endl;
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", info.to_string(mobs::ConvObjToString().exportCompact()));
// dito mit to_json
  EXPECT_EQ(R"({"kundennr":44,"firma":false,"name":"Peter","vorname":"","adresse":{"strasse":"","plz":"","ort":""},"kontakte":[{"art":"fax","number":""},{"art":"fax","number":""},{"art":"fax","number":""},{"art":"fax","number":""},{"art":"mobil","number":"+40 0000 1111 222"}],"hobbies":["","Piano"]})", info.to_string(mobs::ConvObjToString().exportJson()));

  info.hobbies.back()("Schlafen");
  info.hobbies[mobs::MemBaseVector::nextpos]("Schlafen");
  EXPECT_EQ(R"({"kundennr":44,"firma":false,"name":"Peter","vorname":"","adresse":{"strasse":"","plz":"","ort":""},"kontakte":[{"art":"fax","number":""},{"art":"fax","number":""},{"art":"fax","number":""},{"art":"fax","number":""},{"art":"mobil","number":"+40 0000 1111 222"}],"hobbies":["","Schlafen","Schlafen"]})", info.to_string(mobs::ConvObjToString().exportJson()));
  info.hobbies.clear();
  ASSERT_ANY_THROW(info.hobbies.back()("Schlafen"));
  const Person &copers = info;
  string s;
  ASSERT_ANY_THROW((s=copers.hobbies[mobs::MemBaseVector::nextpos]()));

}
  
TEST(objgenTest, iterator) {
  Person info;
  info.kontakte[0].number("+00");
  info.kontakte[1].number("+10");
  info.kontakte[2].number("+20");
  info.kontakte[3].number("+30");
  info.kontakte[4].number("+40");

  mobs::MemberVector<Kontakt>::iterator it = info.kontakte.begin();
  mobs::MemberVector<Kontakt>::iterator it2 = it;
  it += 4;
  EXPECT_EQ(u8"+40", it->number());
  int i = 0;
  for(auto a:info.kontakte)
    i++;
  EXPECT_EQ(5, i);
  std::string s;
  for(auto &a:info.kontakte)
    s += a.number();
  EXPECT_EQ(s, "+00+10+20+30+40");

  mobs::MemberVector<Kontakt>::reverse_iterator itr = info.kontakte.rbegin();
  EXPECT_FALSE(itr == info.kontakte.rend());
  s = "";
  for(; itr != info.kontakte.rend(); itr++)
    s += itr->number();
  EXPECT_EQ(s, "+40+30+20+10+00");
  it2->number("");


}

TEST(objgenTest, const_iterator) {
  Person info;
  info.kontakte[0].number("+00");
  info.kontakte[1].number("+10");
  info.kontakte[2].number("+20");
  info.kontakte[3].number("+30");
  info.kontakte[4].number("+40");

  const Person &cinfo = info;

  mobs::MemberVector<Kontakt>::const_iterator it = cinfo.kontakte.cbegin();
  EXPECT_EQ(&*info.kontakte.cbegin(), &*cinfo.kontakte.begin());
  mobs::MemberVector<Kontakt>::const_iterator it2 = it;
  it += 4;
  EXPECT_EQ(u8"+40", it->number());
  int i = 0;
  for(auto const a:cinfo.kontakte)
    i++;
  EXPECT_EQ(5, i);
  std::string s;
  for(auto const &a:cinfo.kontakte)
    s += a.number();
  EXPECT_EQ(s, "+00+10+20+30+40");

  mobs::MemberVector<Kontakt>::const_reverse_iterator itr = cinfo.kontakte.crbegin();
  EXPECT_FALSE(itr == cinfo.kontakte.crend());
  s = "";
  for(; itr != cinfo.kontakte.crend(); itr++)
    s += itr->number();
  EXPECT_EQ(s, "+40+30+20+10+00");
//  it2->number("");  // Übersetzt nicht wegen const
}

TEST(objgenTest, Pointer) {
  EXPECT_EQ(nullptr, mobs::ObjectBase::createObj("XXX"));

  Person *ip = dynamic_cast<Person *>(mobs::ObjectBase::createObj("Person"));
  ASSERT_NE(nullptr, ip);
  EXPECT_EQ("Person", ip->getObjectName());
  ASSERT_NO_THROW(mobs::string2Obj( R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", *ip));
  // Gegenprobe
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", ip->to_string(mobs::ConvObjToString().exportCompact()));

  // Test ob Objekte nativ zurückgegeben werden und keine Kopien
  EXPECT_EQ(&ip->kontakte, ip->getVecInfo("kontakte", mobs::ConvObjFromStr()));
  EXPECT_EQ(&ip->kontakte[3], dynamic_cast<Kontakt *>(ip->kontakte.getObjInfo(3)));
  EXPECT_EQ(static_cast<mobs::ObjectBase *>(&ip->kontakte[3]), ip->kontakte.getObjInfo(3));
  EXPECT_EQ(ip, dynamic_cast<mobs::ObjectBase *>(ip));
  EXPECT_EQ(&ip->kontakte[0].number, (dynamic_cast<MemVarType(string) *>(ip->kontakte[0].getMemInfo("number", mobs::ConvObjFromStr()))));
  EXPECT_EQ(&ip->kontakte[1].art, (dynamic_cast<MemMobsEnumVarType(device) *>(ip->kontakte[1].getMemInfo("art", mobs::ConvObjFromStr()))));
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

TEST(objgenTest, copy) {
  
  string inhalt = R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})";
  
  Person info;

  ASSERT_NO_THROW(string2Obj(inhalt, info));
  EXPECT_EQ(inhalt, info.to_string(mobs::ConvObjToString().exportCompact()));

  Person info2;
  info2 = info;

  EXPECT_EQ(inhalt, info2.to_string(mobs::ConvObjToString().exportCompact()));

  Person info3(info);
  EXPECT_EQ(inhalt, info3.to_string(mobs::ConvObjToString().exportCompact()));

  
}




class RechPos : virtual public mobs::ObjectBase {
  public:
  ObjInit(RechPos);

  MemVar(std::string, artikel);
  MemVar(unsigned int, anzahl);
  MemVar(float, einzelpreis);
};

class Rechnung : virtual public mobs::ObjectBase {
  public:
  ObjInit(Rechnung);
  MemVar(int, id, USENULL);

  MemObj(Person, kunde, USENULL);
  MemVector(RechPos, position, USENULL, USEVECNULL);

};
ObjRegister(Rechnung)

TEST(objgenTest, usenullAndIndent) {
  Rechnung rech;
  EXPECT_FALSE(rech.nullAllowed());
  EXPECT_TRUE(rech.id.nullAllowed());
  EXPECT_TRUE(rech.kunde.nullAllowed());
  EXPECT_TRUE(rech.position.nullAllowed());
  EXPECT_TRUE(rech.id.isNull());
  EXPECT_TRUE(rech.kunde.isNull());
  EXPECT_TRUE(rech.position.isNull());
  EXPECT_EQ("{id:null,kunde:null,position:null}", to_string(rech));
  rech.position.setEmpty();
  EXPECT_EQ("{id:null,kunde:null,position:[]}", to_string(rech));

  rech.position[3].anzahl(1);
  rech.position[2].anzahl(2);
  rech.position[2].einzelpreis(3);
  rech.position[2].artikel("nnn");
  EXPECT_EQ("{id:null,kunde:null,position:[null,null,{artikel:\"nnn\",anzahl:2,einzelpreis:3},{artikel:\"\",anzahl:1,einzelpreis:0}]}", to_string(rech));
  
  string xml =
  R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<root>
  <id/>
  <kunde/>
  <position/>
  <position/>
  <position>
    <artikel>nnn</artikel>
    <anzahl>2</anzahl>
    <einzelpreis>3</einzelpreis>
  </position>
  <position>
    <artikel></artikel>
    <anzahl>1</anzahl>
    <einzelpreis>0</einzelpreis>
  </position>
</root>
)";
    string json = R"({
  "id":null,
  "kunde":null,
  "position":[
  null,
  null,
  {
    "artikel":"nnn",
    "anzahl":2,
    "einzelpreis":3
  },{
    "artikel":"",
    "anzahl":1,
    "einzelpreis":0
  }]
}
)";

  // TODO korrekt? oder muss um Array ein <array> Tag?
  EXPECT_EQ(xml, rech.to_string(mobs::ConvObjToString().exportXml().doIndent()));
  EXPECT_EQ(json, rech.to_string(mobs::ConvObjToString().exportJson().doIndent()));
//  std::cerr << rech.to_string(mobs::ConvObjToString().exportJson().doIndent()) << std::endl;

  

}



class Obj0 : virtual public mobs::ObjectBase {
public:
  ObjInit(Obj0);

  MemVar(int, aa, KEYELEMENT2);
  MemVar(int, bb);
  MemVar(int, cc, KEYELEMENT1);
  MemVar(int, dd, KEYELEMENT1);
  MemVar(int, ee);
};
class Obj2;

class Obj1 : virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj1);
  MemVar(int, id, KEYELEMENT1);
  MemVar(int, xx);
  MemVar(int, yy, USENULL, KEYELEMENT3);
  MemVar(int, zz);
  MemObj(Obj0, oo, USENULL, KEYELEMENT2);
  // rekursive vorwärts Deklaration Compiliert nicht
  // MemObj(Obj2, rr, USENULL KEYELEMENT2);

};

class Obj2 : virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj2);
  MemVar(int, id, KEYELEMENT1);
  MemVar(int, xx);
  MemVector(Obj2, rekursiv_vector, USEVECNULL);

  // rekursive Definition Compiliert nicht
  // MemObj(Obj2, oo, USENULL KEYELEMENT2);
};

class KeyDump : virtual public mobs::ObjTravConst {
public:
  explicit KeyDump(const mobs::ConvObjToString &c) : quoteKeys(c.withQuotes() ? "\"":""), cth(c) { };
  bool doObjBeg(const mobs::ObjectBase &obj) override {
    if (not obj.getElementName().empty())
      objNames.push(obj.getName(cth) + ".");
    return true;
  };
  void doObjEnd(const mobs::ObjectBase &obj) override {
    if (not objNames.empty())
      objNames.pop();
  };
  bool doArrayBeg(const mobs::MemBaseVector &vec) override { return false; }
  void doArrayEnd(const mobs::MemBaseVector &vec) override { }
  void doMem(const mobs::MemberBase &mem) override {
    std::string name;
    if (not objNames.empty())
      name = objNames.top();
    name += mem.getName(cth);
    if (not fst)
      res << ",";
    fst = false;
    
    res << quoteKeys << name << quoteKeys << ":";
    if (inNull() or mem.isNull())
      res << "null";
    else if (mem.is_chartype(cth))
      res << mobs::to_quote(mem.toStr(cth));
    else
      res << mem.toStr(cth);
  };
  std::string result() { return res.str(); };
private:
  std::string quoteKeys;
  bool fst = true;
  stringstream res;
  stack<std::string> objNames;
  mobs::ConvObjToString cth;
};

std::string showKey(const mobs::ObjectBase &obj) {
  KeyDump kd(mobs::ConvObjToString().exportExtended());
  obj.traverseKey(kd);
  return kd.result();
}




TEST(objgenTest, keys) {
  Obj1 o;
  EXPECT_EQ(2, o.oo.keyElement());
  EXPECT_EQ(3, o.yy.keyElement());
  EXPECT_EQ(1, o.id.keyElement());
  EXPECT_EQ("0::::", o.keyStr());
  o.oo.bb(7);
  EXPECT_EQ("0:0:0:0:", o.keyStr());
  o.oo.cc(211);
  o.oo.dd(212);
  o.oo.aa(220);
  o.id(1);
  o.yy(3);
  EXPECT_EQ("1:211:212:220:3", o.keyStr());
  EXPECT_EQ("1:211:212:220:3", o.keyStr());
  // traversKey
  EXPECT_EQ("id:1,oo.cc:211,oo.dd:212,oo.aa:220,yy:3", showKey(o));
  o.oo.forceNull();
  EXPECT_EQ("id:1,oo.cc:null,oo.dd:null,oo.aa:null,yy:3", showKey(o));

  // TODO test mit quoted string
}

class ObjX : virtual public mobs::ObjectBase {
public:
ObjInit(ObjX, COLNAME(sonst));
  MemVar(int, id, KEYELEMENT1, ALTNAME(grimoald));
  MemVar(int, a, ALTNAME(pippin));
  MemVar(int, b, ALTNAME(karl));
  MemVar(int, c);
  MemObj(Obj0, o, USENULL, ALTNAME(karlmann));
  MemVector(MemVarType(std::string), d, ALTNAME(ludwig));
};

TEST(objgenTest, modified) {
  ObjX x;
  EXPECT_EQ("", x.to_string(mobs::ConvObjToString().exportModified()));
  x.c(7);
  x.o.ee(21);
  EXPECT_EQ("{c:7,o:{ee:21}}", x.to_string(mobs::ConvObjToString().exportModified()));
  x.clearModified();
  EXPECT_EQ("", x.to_string(mobs::ConvObjToString().exportModified()));
}

TEST(objgenTest, conftoken) {
  ObjX o;
  EXPECT_EQ("grimoald", o.getConf(mobs::AltNameBase));
  EXPECT_EQ("", o.getConf(mobs::AltNameEnd));
  EXPECT_EQ(mobs::AltNameBase + 0, o.id.cAltName());
  EXPECT_EQ(mobs::AltNameBase + 1, o.a.cAltName());
  EXPECT_EQ(mobs::AltNameBase + 2, o.b.cAltName());
  EXPECT_EQ(mobs::Unset, o.c.cAltName());
  EXPECT_EQ("grimoald", o.getConf(o.id.cAltName()));
  EXPECT_EQ("pippin", o.getConf(o.a.cAltName()));
  EXPECT_EQ("", o.getConf(o.c.cAltName()));
  EXPECT_EQ("karlmann", o.getConf(o.o.cAltName()));
  EXPECT_EQ("ludwig", o.getConf(o.d.cAltName()));
  EXPECT_EQ("sonst", o.getConf(o.hasFeature(mobs::ColNameBase)));

  mobs::ConvToStrHint cth1(false, false);
  EXPECT_EQ("{id:0,a:0,b:0,c:0,o:null,d:[]}", o.to_string(mobs::ConvObjToString()));
  mobs::ConvToStrHint cth2(false, true);
  EXPECT_EQ("{grimoald:0,pippin:0,karl:0,c:0,karlmann:null,ludwig:[]}", o.to_string(mobs::ConvObjToString().exportAltNames()));

  string xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<root>
  <grimoald>0</grimoald>
  <pippin>0</pippin>
  <karl>0</karl>
  <c>0</c>
  <karlmann>
    <aa>0</aa>
    <bb>0</bb>
    <cc>0</cc>
    <dd>0</dd>
    <ee>0</ee>
  </karlmann>
  <ludwig></ludwig>
</root>
)";
  o.d[0]("");
  o.o.setEmpty();
  EXPECT_EQ(xml, o.to_string(mobs::ConvObjToString().exportXml().exportAltNames().doIndent()));

  ObjX o2;

  EXPECT_NO_THROW(mobs::string2Obj("{grimoald:12,pippin:17,karl:22,c:33,karlmann:{aa:1,bb:2,cc:3,dd:4,ee:6},ludwig:[\"a\"]}", o2, mobs::ConvObjFromStr().useAlternativeNames()));
  EXPECT_EQ("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"a\"]}", o2.to_string(mobs::ConvObjToString()));

  o2.clear();
  EXPECT_NO_THROW(mobs::string2Obj("{grimoald:12,a:17,karl:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},ludwig:[\"a\"]}", o2, mobs::ConvObjFromStr().useAutoNames()));
  EXPECT_EQ("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"a\"]}", o2.to_string(mobs::ConvObjToString()));

  o2.clear();
  EXPECT_NO_THROW(mobs::string2Obj("{grimoald:12,a:17,karl:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},ludwig:[\"a\"]}", o2, mobs::ConvObjFromStr().useAlternativeNames()));
  EXPECT_EQ("{id:12,a:0,b:22,c:33,o:null,d:[\"a\"]}", o2.to_string(mobs::ConvObjToString()));

  // unknwon element
  o2.clear();
  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,XXX:7,d:[\"a\"]}", o2, mobs::ConvObjFromStr()));
  EXPECT_EQ("{id:12,a:17,b:22,c:33,o:null,d:[\"a\"]}", o2.to_string(mobs::ConvObjToString()));
  EXPECT_ANY_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,XXX:7,d:[\"a\"]}", o2, mobs::ConvObjFromStr().useExceptUnknown()));
  
  EXPECT_ANY_THROW(mobs::string2Obj("{id", o2, mobs::ConvObjFromStr()));
  EXPECT_ANY_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,o:7,d:[\"a\"]}", o2, mobs::ConvObjFromStr()));
}

TEST(objgenTest, readmulti) {
  ObjX o;
  EXPECT_NO_THROW(mobs::string2Obj("{grimoald:12,a:17,karl:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},ludwig:[\"a\"]}", o, mobs::ConvObjFromStr().useAlternativeNames()));
  EXPECT_EQ("{id:12,a:0,b:22,c:33,o:null,d:[\"a\"]}", o.to_string(mobs::ConvObjToString()));
  // mehrfaxhes Lesen führt zur selben  Inhalt
  EXPECT_NO_THROW(mobs::string2Obj("{grimoald:12,a:17,karl:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},ludwig:[\"a\"]}", o, mobs::ConvObjFromStr().useAlternativeNames()));
  EXPECT_EQ("{id:12,a:0,b:22,c:33,o:null,d:[\"a\"]}", o.to_string(mobs::ConvObjToString()));

  EXPECT_NO_THROW(mobs::string2Obj("{grimoald:12,a:17,karl:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},ludwig:[\"b\",\"c\"]}", o, mobs::ConvObjFromStr().useAlternativeNames()));
  EXPECT_EQ("{id:12,a:0,b:22,c:33,o:null,d:[\"b\",\"c\"]}", o.to_string(mobs::ConvObjToString()));

  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"x\"]}", o, mobs::ConvObjFromStr()));
  EXPECT_EQ("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"x\"]}", o.to_string(mobs::ConvObjToString()));

  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"x\",\"c\"]}", o, mobs::ConvObjFromStr()));
  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"j\"]}", o, mobs::ConvObjFromStr().useDontShrink()));
  EXPECT_EQ("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"j\",\"c\"]}", o.to_string(mobs::ConvObjToString()));

  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"x\",\"c\"]}", o, mobs::ConvObjFromStr()));
  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:null,c:33,o:null,d:null}", o, mobs::ConvObjFromStr().useDontShrink().useForceNull()));
  EXPECT_EQ("{id:12,a:17,b:null,c:33,o:null,d:null}", o.to_string(mobs::ConvObjToString()));

  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"x\",\"c\"]}", o, mobs::ConvObjFromStr()));
  EXPECT_NO_THROW(mobs::string2Obj("{id:1,a:2,b:null,c:33,o:null,d:null}", o, mobs::ConvObjFromStr().useDontShrink().useOmitNull()));
  EXPECT_EQ("{id:1,a:2,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"x\",\"c\"]}", o.to_string(mobs::ConvObjToString()));

  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"x\",\"c\"]}", o, mobs::ConvObjFromStr()));
  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:null,c:33,o:null,d:[null]}", o, mobs::ConvObjFromStr().useDontShrink().useForceNull()));
  EXPECT_EQ("{id:12,a:17,b:null,c:33,o:null,d:[null,\"c\"]}", o.to_string(mobs::ConvObjToString()));

  EXPECT_EQ("{id:12,a:17,c:33,d:[\"c\"]}", o.to_string(mobs::ConvObjToString().exportWoNull()));


}

TEST(objgenTest, readxml) {
  ObjX o, o2;

  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"x\"]}", o, mobs::ConvObjFromStr()));
  string xml = o.to_string(mobs::ConvObjToString().exportXml().doIndent());
  EXPECT_NO_THROW(mobs::string2Obj(xml, o2, mobs::ConvObjFromStr().useXml()));

  EXPECT_EQ("{id:12,a:17,b:22,c:33,o:{aa:1,bb:2,cc:3,dd:4,ee:6},d:[\"x\"]}", o2.to_string(mobs::ConvObjToString()));

}

TEST(objgenTest, readxml2) {
  ObjX o, o2;

  EXPECT_NO_THROW(mobs::string2Obj("{id:12,a:17,b:null,c:33,o:null,d:[null,\"c\"]}", o, mobs::ConvObjFromStr().useForceNull()));
  EXPECT_EQ("{id:12,a:17,b:null,c:33,o:null,d:[null,\"c\"]}", o.to_string(mobs::ConvObjToString()));
  string xml = o.to_string(mobs::ConvObjToString().exportXml().doIndent());
  EXPECT_EQ(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<root>
  <id>12</id>
  <a>17</a>
  <b/>
  <c>33</c>
  <o/>
  <d/>
  <d>c</d>
</root>
)", xml);

  EXPECT_NO_THROW(mobs::string2Obj(xml, o2, mobs::ConvObjFromStr().useXml().useForceNull()));
  EXPECT_EQ("{id:12,a:17,b:null,c:33,o:null,d:[null,\"c\"]}", o2.to_string(mobs::ConvObjToString()));
}


class ObjE1 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjE1);

  MemVar(int, aa);
  MemVar(int, bb, ALTNAME(bu));
  MemVar(int, cc);
};

class ObjE2 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjE2);

  MemVar(int, xx);
  MemObj(ObjE1, yy, EMBEDDED, PREFIX(a_));
  MemVar(int, aa);
  MemObj(ObjE1, ww, PREFIX(b_));

};

class ObjE3 : public mobs::ObjectBase {
public:
  ObjInit(ObjE3);

  MemVar(int, xx);
  MemObj(ObjE1, yy, EMBEDDED);
  MemVar(int, zz);
};

class ObjE3b : public ObjE1 {
public:
  ObjInit(ObjE3b);

  MemVar(int, xx);
  MemVar(int, zz);
};


TEST(objgenTest, embedded) {
  ObjE2 e2;
  ObjE3 e3;
  ObjE3b e4;

  EXPECT_NO_THROW(mobs::string2Obj("{xx:1,aa:2,bb:3,cc:4,zz:5}", e3, mobs::ConvObjFromStr().useExceptUnknown()));
  EXPECT_EQ("{xx:1,aa:2,bb:3,cc:4,zz:5}", e3.to_string(mobs::ConvObjToString()));

  EXPECT_NO_THROW(mobs::string2Obj("{xx:1,aa:2,bb:3,cc:4,zz:5}", e4, mobs::ConvObjFromStr().useExceptUnknown()));
  EXPECT_EQ("{aa:2,bb:3,cc:4,xx:1,zz:5}", e4.to_string(mobs::ConvObjToString()));

  e2.yy.bb(3);
  EXPECT_EQ("{xx:0,a_aa:0,a_bb:3,a_cc:0,aa:0,ww:{b_aa:0,b_bb:0,b_cc:0}}", e2.to_string(mobs::ConvObjToString().exportPrefix()));
  EXPECT_EQ("{xx:0,a_aa:0,a_bb:3,a_cc:0,aa:0,ww:{aa:0,bb:0,cc:0}}", e2.to_string(mobs::ConvObjToString()));
  EXPECT_NO_THROW(mobs::string2Obj("{xx:1,a_aa:2,a_bu:3,a_cc:4,aa:5,ww:{aa:0,bb:0,cc:0}}", e2, mobs::ConvObjFromStr().useAutoNames().useExceptUnknown()));

}

class ObjE4 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjE4);

  MemVar(int, Xx);
  MemVar(int, zz);
};


TEST(objgenTest, ignoreCase) {
  ObjE4 e4;

  EXPECT_ANY_THROW(mobs::string2Obj("{xx:1,zz:2}", e4, mobs::ConvObjFromStr().useExceptUnknown()));
  EXPECT_NO_THROW(mobs::string2Obj("{xx:1,zz:2}", e4, mobs::ConvObjFromStr().useExceptUnknown().useIgnoreCase()));
  EXPECT_EQ("{xx:1,zz:2}", e4.to_string(mobs::ConvObjToString().exportLowercase()));

}

class ObjE5 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjE5, COLNAME(AAA));

  MemVar(int, Xx);
  MemVar(int, zz, ALTNAME(BBB));
};
ObjRegister(ObjE5);

TEST(objgenTest, copyBug) {
  ObjE5 e5;

  EXPECT_EQ("AAA", e5.getConf(e5.hasFeature(mobs::MemVarCfg::ColNameBase)));
  EXPECT_EQ("BBB", e5.getConf(e5.zz.cAltName()));

  auto x = [&e5]() {
    EXPECT_EQ("AAA", e5.getConf(e5.hasFeature(mobs::MemVarCfg::ColNameBase)));
    EXPECT_EQ("BBB", e5.getConf(e5.zz.cAltName()));
  };
  auto y = [e5]() {
    // COLNAME geht verloren  im Copy-Construktor
    EXPECT_EQ("AAA", e5.getConf(e5.hasFeature(mobs::MemVarCfg::ColNameBase)));
    EXPECT_EQ("BBB", e5.getConf(e5.zz.cAltName()));
  };
  x();
  y();

  mobs::ObjectBase *e5p = mobs::ObjectBase::createObj("ObjE5");
  ASSERT_TRUE((e5p));
  EXPECT_EQ("AAA", e5p->getConf(e5p->hasFeature(mobs::MemVarCfg::ColNameBase)));

}


MOBS_ENUM_DEF(someTypes, K_t1, K_t2, K_t3 );

MOBS_ENUM_VAL(someTypes, "T1", "T2", "T3");


class VecEnum : virtual public mobs::ObjectBase {
public:
  ObjInit(VecEnum);

//  MemVector(MemMobsEnumVarType(someTypes), art);
  MemEnumVector(someTypes, art);
  MemVar(string, s);
};

class VecString : virtual public mobs::ObjectBase {
public:
  ObjInit(VecString);

  MemVarVector(string, token);
  MemVar(string, s, KEYELEMENT1);
  MemVar(string, p, KEYELEMENT2);
  MemVar(int, q, KEYELEMENT3);
};

class VecObjString : virtual public mobs::ObjectBase {
public:
  ObjInit(VecObjString);

  MemVector(VecString, objs);
  MemVar(string, s, KEYELEMENT1);
  MemVar(string, p, KEYELEMENT2);
  MemVar(int, q, KEYELEMENT3);
};


TEST(objgenTest, vectorEnum) {
  VecEnum v;
  v.s("A");
  v.art[mobs::MemBaseVector::nextpos](K_t1);
  v.art[mobs::MemBaseVector::nextpos](K_t2);
  v.art[mobs::MemBaseVector::nextpos](K_t3);
  EXPECT_EQ("{art:[\"T1\",\"T2\",\"T3\"],s:\"A\"}", v.to_string(mobs::ConvObjToString()));
  EXPECT_THROW(v.objNameKeyStr(), runtime_error);
}

TEST(objgenTest, findVectorSimple) {
  VecString v;
  v.s("KK");
  v.p("XX");
  v.q(42);
  v.token[mobs::MemBaseVector::nextpos]("S1");
  v.token[mobs::MemBaseVector::nextpos]("S2");
  v.token[mobs::MemBaseVector::nextpos]("S3");
  v.token.find(string("S2"));
  EXPECT_EQ(1, v.token.find(string("S2")));
  EXPECT_EQ(2, v.token.find(string("S3")));
  EXPECT_TRUE(mobs::MemBaseVector::npos == v.token.find(string("S4")));
  EXPECT_TRUE(mobs::MemBaseVector::npos == v.token.find(string("S1", 1)));
  EXPECT_TRUE(v.token.contains(string("S3")));
  EXPECT_FALSE(v.token.contains(string("S4")));

}

TEST(objgenTest, findVectorObject) {
  VecObjString v;
  v.s("K\\K");
  v.p("X:X");
  v.q(42);
  EXPECT_STREQ("VecObjString:K\\\\K:X\\:X:42", v.objNameKeyStr().c_str());
  mobs::string2Obj( "{s:\"S1\",p:\"P1\",q:1}", v.objs[mobs::MemBaseVector::nextpos], mobs::ConvObjFromStr());
  mobs::string2Obj( "{s:\"S2\",p:\"P2\",q:2}", v.objs[mobs::MemBaseVector::nextpos], mobs::ConvObjFromStr());
  mobs::string2Obj( "{s:\"S3\",p:\"P3\",q:3}", v.objs[mobs::MemBaseVector::nextpos], mobs::ConvObjFromStr());
  EXPECT_STREQ("VecString:S1:P1:1", v.objs[0].objNameKeyStr().c_str());
  EXPECT_EQ(0, v.objs.find(string("VecString:S1:P1:1")));
  EXPECT_EQ(1, v.objs.find(string("VecString:S2:P2:2")));
  EXPECT_TRUE(mobs::MemBaseVector::npos == v.objs.find(string("VecString:S1:P1:1"), 1));
  EXPECT_TRUE(mobs::MemBaseVector::npos == v.objs.find(string("VecString:S4:P4:4")));
  EXPECT_TRUE(v.objs.contains(string("VecString:S3:P3:3")));
  EXPECT_FALSE(v.objs.contains(string("VecString:S4:P4:4")));


}



#if 0
TEST(objgenTest, compare) {
  Person p;
  p.name("ABC");
  p.vorname("ABC");
  EXPECT_TRUE(p.name() == p.vorname());
  EXPECT_TRUE(p.name == p.vorname);
  
  

}
#endif


}



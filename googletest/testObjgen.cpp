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
#include "xmlout.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>



using namespace std;

//enum device { fax, sms, mobil, privat, arbeit };
//namespace mobs {std::string to_string(enum device t);}


MOBS_ENUM_DEF(device, fax, sms, mobil, privat, arbeit )
MOBS_ENUM_VAL(device, "fax", "sms", "mobil", "privat", "arbeit" )


namespace {



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
  ObjVar(Adresse, adresse, USENULL);
  MemVector(Kontakt, kontakte);
//  MemVector(MemVarType(std::string), hobbies);
  MemVarVector(std::string, hobbies);

  virtual void init() {  };
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
//  MemVarVector(string, susi);
//  MemVarVector(string, friederich);
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


TEST(objgenTest, setnull) {
  Person info;
  info.adresse.forceNull();
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
  info.adresse.forceNull();
  info.kundennr(44);
  info.name(u8"Peter");
  info.kontakte[4].art(mobil);
  info.kontakte[4].number("+40 0000 1111 222");
  info.hobbies[1]("Piano");

  EXPECT_EQ("Adresse", info.adresse.typName());
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:null,kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", info.to_string(mobs::ConvObjToString().exportCompact()));
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:null,kontakte:[{art:"fax",number:""},{art:"fax",number:""},{art:"fax",number:""},{art:"fax",number:""},{art:"mobil",number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", mobs::to_string(info));
  info.adresse.setEmpty();
//  cerr << mobs::to_string(info) << endl;
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", info.to_string(mobs::ConvObjToString().exportCompact()));
// dito mit to_json
  EXPECT_EQ(R"({"kundennr":44,"firma":false,"name":"Peter","vorname":"","adresse":{"strasse":"","plz":"","ort":""},"kontakte":[{"art":"fax","number":""},{"art":"fax","number":""},{"art":"fax","number":""},{"art":"fax","number":""},{"art":"mobil","number":"+40 0000 1111 222"}],"hobbies":["","Piano"]})", info.to_string(mobs::ConvObjToString().exportJson()));
}
  
TEST(objgenTest, iterator) {
  Person info;
  info.kontakte[4].number("+40 0000 1111 222");

  mobs::MemberVector<Kontakt>::iterator it= info.kontakte.begin();
  it += 4;
  EXPECT_EQ(u8"+40 0000 1111 222", it->number());
  int i = 0;
  for(auto a:info.kontakte)
    i++;
  EXPECT_EQ(5, i);
}

TEST(objgenTest, Pointer) {
  EXPECT_EQ(nullptr, mobs::ObjectBase::createObj("XXX"));

  Person *ip = dynamic_cast<Person *>(mobs::ObjectBase::createObj("Person"));
  ASSERT_NE(nullptr, ip);
  EXPECT_EQ("Person", ip->typName());
  ASSERT_NO_THROW(mobs::string2Obj( R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", *ip));
  // Gegenprobe
  EXPECT_EQ(R"({kundennr:44,firma:false,name:"Peter",vorname:"",adresse:{strasse:"",plz:"",ort:""},kontakte:[{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:0,number:""},{art:2,number:"+40 0000 1111 222"}],hobbies:["","Piano"]})", ip->to_string(mobs::ConvObjToString().exportCompact()));

  // Test ob Objekte nativ zurückgegeben werden und keine Kopien
  EXPECT_EQ(&ip->kontakte, ip->getVecInfo("kontakte"));
  EXPECT_EQ(&ip->kontakte[3], dynamic_cast<Kontakt *>(ip->kontakte.getObjInfo(3)));
  EXPECT_EQ(static_cast<mobs::ObjectBase *>(&ip->kontakte[3]), ip->kontakte.getObjInfo(3));
  EXPECT_EQ(ip, dynamic_cast<mobs::ObjectBase *>(ip));
  EXPECT_EQ(&ip->kontakte[0].number, (dynamic_cast<MemVarType(string) *>(ip->kontakte[0].getMemInfo("number"))));
  EXPECT_EQ(&ip->kontakte[1].art, (dynamic_cast<MemMobsEnumVarType(device) *>(ip->kontakte[1].getMemInfo("art"))));
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

  ObjVar(Person, kunde, USENULL);
  MemVector(RechPos, position, USENULL USEVECNULL);

};
ObjRegister(Rechnung);

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

class Obj1 : virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj1);
  MemVar(int, id, KEYELEMENT1);
  MemVar(int, xx);
  MemVar(int, yy, USENULL KEYELEMENT3);
  MemVar(int, zz);

  ObjVar(Obj0, oo, USENULL KEYELEMENT2);
};

TEST(objgenTest, keys) {
  Obj1 o;
  EXPECT_EQ(2, o.oo.key());
  EXPECT_EQ(3, o.yy.key());
  EXPECT_EQ(1, o.id.key());
  EXPECT_EQ("0....", o.keyStr());
  o.oo.bb(7);
  EXPECT_EQ("0.0.0.0.", o.keyStr());
  o.oo.cc(211);
  o.oo.dd(212);
  o.oo.aa(220);
  o.id(1);
  o.yy(3);
  EXPECT_EQ("1.211.212.220.3", o.keyStr());
  EXPECT_EQ("1.211.212.220.3", o.keyStr());
  // TODO test mit qouted string
}




class ObjX : virtual public mobs::ObjectBase {
public:
ObjInit(ObjX);
  MemVar(int, id, KEYELEMENT1 ALTNAME(grimoald));
  MemVar(int, a, ALTNAME(pippin));
  MemVar(int, b, ALTNAME(karl));
  MemVar(int, c);
  ObjVar(Obj0, o, USENULL ALTNAME(karlmann));
  MemVector(MemVarType(std::string), d, ALTNAME(ludwig));

};

TEST(objgenTest, conftoken) {
  ObjX o;
  EXPECT_EQ("grimoald", o.getConf(0));
  EXPECT_EQ("", o.getConf(99));
  EXPECT_EQ(0, o.id.cAltName());
  EXPECT_EQ(1, o.a.cAltName());
  EXPECT_EQ(2, o.b.cAltName());
  EXPECT_EQ(SIZE_T_MAX, o.c.cAltName());
  EXPECT_EQ("grimoald", o.getConf(o.id.cAltName()));
  EXPECT_EQ("pippin", o.getConf(o.a.cAltName()));
  EXPECT_EQ("", o.getConf(o.c.cAltName()));
  EXPECT_EQ("karlmann", o.getConf(o.o.cAltName()));
  EXPECT_EQ("ludwig", o.getConf(o.d.cAltName()));
  
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


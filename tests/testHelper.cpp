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
#include "helper.h"
#include "logging.h"

#include <sstream>
#include <chrono>
#include <gtest/gtest.h>



using namespace std;


namespace {

MOBS_ENUM_DEF(Colour, Green, Blue, Red, Yellow, Orange, Cyan);

MOBS_ENUM_VAL(Colour, "green", "blue", "red", "yellow", "orange", "cyan");


class SQLDBTestDesc : public mobs::SQLDBdescription {
public:

  virtual std::string tableName(const std::string &tabnam) override { return u8"D." + tabnam;  }

  virtual std::string valueStmtIndex(size_t i) override { return std::string(" ") + to_string(i);  }

  virtual std::string valueStmtText(const std::string &tx, bool isNull) override { return std::string(" ") + (isNull ? string("null"):mobs::to_squote(tx));  }

  virtual std::string createStmtIndex(std::string name) override {
    return "INT NOT NULL";
  }

  virtual std::string createStmtText(const std::string &name, size_t len) override {
    return string("VARCHAR(") + to_string(len) + ")";
  }

  virtual std::string createStmt(const mobs::MemberBase &mem, bool compact) override {
    std::stringstream res;
    mobs::MobsMemberInfo mi;
    mem.memInfo(mi);
    double d;
    if (mi.isTime and mi.granularity >= 86400000)
      res << "DATE";
    else if (mi.isTime)
      res << "DATETIME";
    else if (mem.toDouble(d))
      res << "FLOAT";
    else if (mem.is_chartype(mobs::ConvToStrHint(compact))) {
      mobs::MemVarCfg c = mem.hasFeature(mobs::LengthBase);
      size_t n = c ? (c - mobs::LengthBase) : 30;
      res << "VARCHAR(" << n << ")";
    } else if (mi.isSigned and mi.max <= 32767)
      res << "SMALLINT";
    else if (mi.isSigned)
      res << "INT";
    else if (mi.isUnsigned)
      res << "INT";
    else
      res << "SMALLINT";
    if (not mem.nullAllowed())
      res << " NOT NULL";
    return res.str();
  }

  virtual std::string valueStmt(const mobs::MemberBase &mem, bool compact, bool increment, bool inWhere) override {
    if (increment) {
      mobs::MobsMemberInfo mi;
      mem.memInfo(mi);
      if (mi.isUnsigned)
        return to_string(mi.u64 + 1);
      else if (mi.isSigned)
        return to_string(mi.i64 + 1);
      else
        throw std::runtime_error("VersionElememt is not int");
    }
    if (mem.isNull())
      return u8"null";
    if (mem.is_chartype(mobs::ConvToStrHint(compact)))
      return mobs::to_squote(mem.toStr(mobs::ConvToStrHint(compact)));

    return mem.toStr(mobs::ConvToStrHint(compact));
  }

  // Einlesen der Elemente - Mocking-Test, die Sequenz entspricht der Abfrage aus GenerateSql
  virtual void readValue(mobs::MemberBase &mem, bool compact) override {
    mobs::MobsMemberInfo mi;
    mem.memInfo(mi);
    if (mi.isUnsigned)
      mem.fromUInt64(1);
    else if (mi.isSigned)
      mem.fromInt64(2);
    else if (mem.is_chartype(mobs::ConvToStrHint(compact)))
      mem.fromStr("x", not compact ? mobs::ConvFromStrHint::convFromStrHintExplizit : mobs::ConvFromStrHint::convFromStrHintDflt);
  }

  virtual void readValueText(const std::string &name, std::string &text, bool &null) override {
    null = false;
    if (name == "ll")
      text = "[1,2,4]";
    else
      text = "{aa:2,bb:3,cc:4,col:\"green\"}";
  }

  virtual size_t readIndexValue(const std::string &name) override { return 1; }
  virtual void startReading() override {}
  virtual void finishReading() override {}

};

class ObjA1 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjA1);
  MemVar(std::string, a1bc, KEYELEMENT1);
  MemVar(int, c1de);
  MemVar(int, f1gh);

};

class ObjA2 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjA2);
  MemVar(int, k2kk, KEYELEMENT1);
  MemVar(std::string, s2s);
  MemVector(ObjA1, o2oo, ALTNAME(oo_ix));
};

class ObjA3 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjA3);
  MemVar(int, k3kk, KEYELEMENT1);
  MemVar(unsigned int, version, VERSIONFIELD);
  MemVar(std::string, p3p);
  MemObj(ObjA2, oa3, PREFIX(o_));
};



class ObjE1 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjE1);

  MemVar(int, aa);
  MemVar(int, bb, ALTNAME(bu));
  MemVar(int, cc, USENULL);
  MemMobsEnumVar(Colour, col);
};

class ObjE2 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjE2);

  MemVar(int, xx, KEYELEMENT2);
  MemVar(unsigned int, version, mobs::DbVersionField);
  MemObj(ObjE1, yy, EMBEDDED PREFIX(a_));
  MemVar(std::string, aa, KEYELEMENT1);
  MemObj(ObjE1, ww, PREFIX(b_) USENULL);
  MemMobsEnumVar(Colour, col, DBCOMPACT);

};

class ObjE3 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjE3);

  MemVar(int, xx);
  MemObj(ObjE1, yy, EMBEDDED);
  MemVar(int, zz);
};

class ObjJ1 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjJ1);

  MemVar(int, xx, KEYELEMENT1);
  MemObj(ObjE1, yy, DBJSON LENGTH(99));
  MemVar(int, zz);
  MemVarVector(int, ll, DBJSON LENGTH(88));
};





class SetModified  : virtual public mobs::ObjTrav {
  public:
    bool doObjBeg(mobs::ObjectBase &obj) override { obj.setModified(true); return true; }
    void doObjEnd(mobs::ObjectBase &obj) override {  }
    bool doArrayBeg(mobs::MemBaseVector &vec) override { vec.setModified(true); return true; }
    void doArrayEnd(mobs::MemBaseVector &vec) override { }
    void doMem(mobs::MemberBase &mem) override { mem.setModified(true); }
  };





class Elementor : virtual public mobs::ElementNames {
public:
  explicit Elementor(mobs::ConvObjToString c) : mobs::ElementNames(c) { };


  virtual void valueStmt(const std::string &name, const mobs::MemberBase &mem, bool compact) {
    res << " ";
    if (mem.isNull()) {
      res << name << ":null";
      return;
    }
    if (mem.is_chartype(mobs::ConvToStrHint(compact)))
      res << name << ":" << mobs::to_squote(mem.toStr(mobs::ConvToStrHint(compact)));
    else
      res << name << ":" << mem.toStr(mobs::ConvToStrHint(compact));
  }
  std::stringstream res;
  std::string result() { return res.str(); };
};




TEST(helperTest, fields) {
  ObjE2 e2;

  Elementor elk((mobs::ConvObjToString()));
  e2.traverseKey(elk);
  EXPECT_EQ(" aa:'' xx:0", elk.result());

  Elementor ele((mobs::ConvObjToString()));
  e2.traverse(ele);
  EXPECT_EQ(" xx:0 version:0 a_aa:0 a_bb:0 a_cc:null a_col:'green' aa:'' ww.aa:0 ww.bb:0 ww.cc:null ww.col:'green' col:0", ele.result());
}


TEST(helperTest, sql) {

  ObjA3 a3;

  SetModified sm;
  a3.traverse(sm);

  a3.clearModified();

  SQLDBTestDesc sd;
  mobs::SqlGenerator gsql(a3, sd);

  EXPECT_EQ("select mt.k3kk,mt.version,mt.p3p,mt.o_k2kk,mt.o_s2s from D.ObjA3 mt ;",
            gsql.queryBE());


  a3.oa3.o2oo[0].a1bc("XX");

  EXPECT_EQ("select distinct mt.k3kk from D.ObjA3 mt  left join D.ObjA3_o2oo on mt.k3kk = D.ObjA3_o2oo.k3kk where D.ObjA3_o2oo.a1bc='XX';",
            gsql.queryBE(mobs::SqlGenerator::Keys));

  EXPECT_EQ("drop table D.ObjA3;", gsql.dropStatement(true));
  EXPECT_FALSE(gsql.eof());
  EXPECT_EQ("drop table D.ObjA3_o2oo;", gsql.dropStatement(false));
  EXPECT_TRUE(gsql.eof());

  a3.clearModified();
  EXPECT_EQ("create table D.ObjA3(k3kk INT NOT NULL,version INT NOT NULL,p3p VARCHAR(30) NOT NULL,o_k2kk INT NOT NULL,o_s2s VARCHAR(30) NOT NULL, unique(k3kk));",
            gsql.createStatement(true));
  EXPECT_FALSE(gsql.eof());
  EXPECT_EQ("create table D.ObjA3_o2oo(k3kk INT NOT NULL,o_oo_ix INT NOT NULL,a1bc VARCHAR(30) NOT NULL,c1de INT NOT NULL,f1gh INT NOT NULL, unique(k3kk,o_oo_ix));",
            gsql.createStatement(false));
  EXPECT_TRUE(gsql.eof());


//  a3.oa3.o2oo[1].forceNull();
  a3.clearModified();
  a3.oa3.o2oo[1].forceNull();
  a3.oa3.o2oo[2].c1de(4);

  EXPECT_EQ("replace D.ObjA3(k3kk,version,p3p,o_k2kk,o_s2s) VALUES (0,1,'',0,'');",
            gsql.replaceStatement(true));
  EXPECT_EQ("replace D.ObjA3_o2oo(k3kk,o_oo_ix,a1bc,c1de,f1gh) VALUES (0, 0,'XX',0,0);",
            gsql.replaceStatement(false));
  EXPECT_EQ("delete from D.ObjA3_o2oo where k3kk=0 and o_oo_ix= 1;",
            gsql.replaceStatement(false));
  EXPECT_EQ("replace D.ObjA3_o2oo(k3kk,o_oo_ix,a1bc,c1de,f1gh) VALUES (0, 2,'',4,0);",
            gsql.replaceStatement(false));
  EXPECT_FALSE(gsql.eof());
  EXPECT_EQ("delete from D.ObjA3_o2oo where k3kk=0 and o_oo_ix> 2;", gsql.replaceStatement(false));
  EXPECT_TRUE(gsql.eof());
  a3.oa3.o2oo.resize(1);

  string upd;
  EXPECT_EQ("insert into D.ObjA3(p3p,o_k2kk,o_s2s,k3kk,version) values (?,?,?,?,?);", gsql.insertUpdStatement(true, upd));
  EXPECT_FALSE(gsql.eof());
  EXPECT_EQ("update D.ObjA3 set version=version+1,p3p=?,o_k2kk=?,o_s2s=? where k3kk=? and version=?;", upd);
  EXPECT_EQ("insert into D.ObjA3_o2oo(a1bc,c1de,f1gh,k3kk,o_oo_ix) values (?,?,?,?,?);", gsql.insertUpdStatement(false, upd));
  EXPECT_FALSE(gsql.eof());
  EXPECT_EQ("update D.ObjA3_o2oo set a1bc=?,c1de=?,f1gh=? where k3kk=? and o_oo_ix=?;", upd);
  EXPECT_EQ("delete from D.ObjA3_o2oo where k3kk=0 and o_oo_ix> 0;", gsql.insertUpdStatement(false, upd));
  EXPECT_EQ("", upd);
  EXPECT_TRUE(gsql.eof());


  EXPECT_EQ("update D.ObjA3 set version=1,p3p='',o_k2kk=0,o_s2s='' where k3kk=0 and version=0;", gsql.updateStatement(true));
  EXPECT_FALSE(gsql.eof());
  EXPECT_EQ("update D.ObjA3_o2oo set a1bc='XX',c1de=0,f1gh=0 where k3kk=0 and o_oo_ix= 0;", gsql.updateStatement(false));
  EXPECT_FALSE(gsql.eof());
  EXPECT_EQ("delete from D.ObjA3_o2oo where k3kk=0 and o_oo_ix> 0;", gsql.updateStatement(false));
  EXPECT_TRUE(gsql.eof());

  EXPECT_EQ("delete from D.ObjA3 where k3kk=0 and version=0;", gsql.deleteStatement(true));
  EXPECT_FALSE(gsql.eof());
  EXPECT_EQ("delete from D.ObjA3_o2oo where k3kk=0;", gsql.deleteStatement(false));
  EXPECT_TRUE(gsql.eof());
 
  EXPECT_EQ("select k3kk from D.ObjA3 where k3kk=0;", gsql.selectStatementFirst(true));
  EXPECT_EQ("select k3kk,version,p3p,o_k2kk,o_s2s from D.ObjA3 where k3kk=0;", gsql.selectStatementFirst());
    
  ObjA3 a3r;
  gsql.readObject(a3r);
  
  mobs::SqlGenerator::DetailInfo di;
  EXPECT_FALSE(gsql.eof());
  EXPECT_EQ("select o_oo_ix,a1bc,c1de,f1gh from D.ObjA3_o2oo where k3kk=0;", gsql.selectStatementArray(di));
  EXPECT_TRUE(gsql.eof());

  a3.k3kk.forceNull();
  EXPECT_EQ("select k3kk,version,p3p,o_k2kk,o_s2s from D.ObjA3 where k3kk is null;", gsql.selectStatementFirst());

}



TEST(helperTest, dbjson) {
  ObjJ1 j1,j2;

  EXPECT_NO_THROW(mobs::string2Obj("{xx:1,yy:{aa:2,bb:3,cc:4},zz:5,ll:[1,2,4]}", j1, mobs::ConvObjFromStr().useExceptUnknown()));
  SQLDBTestDesc sd;
  mobs::SqlGenerator gsql(j1, sd);
  EXPECT_EQ(R"(replace D.ObjJ1(xx,yy,zz,ll) VALUES (1, '{aa:2,bb:3,cc:4,col:"green"}',5, '[1,2,4]');)",
            gsql.replaceStatement(true));
  EXPECT_EQ(R"(create table D.ObjJ1(xx INT NOT NULL,yy VARCHAR(99),zz INT NOT NULL,ll VARCHAR(88), unique(xx));)",
            gsql.createStatement(true));

  gsql.readObject(j2);
  EXPECT_EQ(R"({xx:2,yy:{aa:2,bb:3,cc:4,col:"green"},zz:2,ll:[1,2,4]})", j2.to_string());

}


TEST(helperTest, auditTrail) {

  ObjA3 a3;
  a3.version(0);
  a3.oa3.o2oo[2].c1de(4);
  a3.p3p.forceNull();
  a3.startAudit();
  
  SetModified sm;
//a3.traverse(sm);
  a3.oa3.o2oo.resize(1);
  a3.oa3.o2oo[1].c1de(5);
  a3.p3p("abc");

  
  mobs::AuditActivity act;
  mobs::AuditTrail at(act);
//  at.destroyObj();
  a3.traverse(at);
  // TODO
  cerr << act.to_string(mobs::ConvObjToString().doIndent()) << endl;
 
  
  SQLDBTestDesc sd;
  mobs::SqlGenerator gsql(act, sd);
  for (bool first = true; first or not gsql.eof(); first = false)
    LOG(LM_INFO, "CR " << gsql.createStatement(first));


}


}

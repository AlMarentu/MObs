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
#include <gtest/gtest.h>



using namespace std;


namespace {

MOBS_ENUM_DEF(Colour, Green, Blue, Red, Yellow, Orange, Cyan);
MOBS_ENUM_VAL(Colour, "green", "blue", "red", "yellow", "orange", "cyan");



class ObjA1 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjA1);
  MemVar(std::string, abc, KEYELEMENT1);
  MemVar(int, cde);
  MemVar(int, fgh);

};

class ObjA2 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjA2);
  MemVar(int, kkk2, KEYELEMENT1);
  MemVar(std::string, ss);
  MemVector(ObjA1, ooo, DBDETAIL);
};

class ObjA3 : virtual public mobs::ObjectBase {
public:
  ObjInit(ObjA3);
  MemVar(int, kkk, KEYELEMENT1);
  MemVar(std::string, pp);
  MemObj(ObjA2, oa2, PREFIX(o_));
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

using mobs::GenerateSql;

class Extractor : virtual public mobs::ExtractSql {
public:
  explicit Extractor(Mode m, mobs::ConvObjToString c) : mobs::ExtractSql(m, c) { };

  virtual void readValue(mobs::MemberBase &mem, bool compact) {
    res << " " << mem.getName(cth);
  }

  std::stringstream res;
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

  GenerateSql gsf(GenerateSql::Fields, mobs::ConvObjToString().exportPrefix());
  e2.traverse(gsf);

  EXPECT_EQ("", gsf.result());

  LOG(LM_INFO, "FIELDS  " << gsf.result());

  Extractor esf(Extractor::Fields, mobs::ConvObjToString().exportPrefix());
  e2.traverse(esf);
  LOG(LM_INFO, "EXTRACT" << esf.res.str());

  GenerateSql gs3(GenerateSql::Keys, mobs::ConvObjToString().exportPrefix());
  e2.traverse(gs3);
  LOG(LM_INFO, "KEYFLD " << gs3.result());

  GenerateSql gsq(GenerateSql::Query, mobs::ConvObjToString().exportPrefix());
  e2.traverse(gsq);
  LOG(LM_INFO, "QUERY " << gsq.result());

  GenerateSql gsk(GenerateSql::Query, mobs::ConvObjToString().exportPrefix());
  e2.traverseKey(gsk);
  LOG(LM_INFO, "KEY   " << gsk.result());

  GenerateSql gsv(GenerateSql::Values, mobs::ConvObjToString().exportPrefix());
  e2.traverse(gsv);
  LOG(LM_INFO, "VALUES " << gsv.result());

  GenerateSql gss(GenerateSql::Update, mobs::ConvObjToString().exportPrefix());
  e2.traverse(gss);
  LOG(LM_INFO, "UPDATE " << gss.result());

  GenerateSql gs2(GenerateSql::UpdateValues, mobs::ConvObjToString().exportPrefix());
  e2.traverse(gs2);
  LOG(LM_INFO, "UPDVAL " << gs2.result());

  GenerateSql gsc(GenerateSql::Create, mobs::ConvObjToString().exportPrefix());
  e2.traverse(gsc);
  LOG(LM_INFO, "CREATE " << gsc.result());

  GenerateSql gsu(GenerateSql::Fields, mobs::ConvObjToString().exportPrefix());
  e2.traverseKey(gsu);
  LOG(LM_INFO, "UNIQE " << gsu.result());




  Elementor elk((mobs::ConvObjToString()));
  e2.traverseKey(elk);
  LOG(LM_INFO, "ELEMENT K" << elk.result());

  Elementor ele((mobs::ConvObjToString()));
  e2.traverse(ele);
  LOG(LM_INFO, "ELEMENT F" << ele.result());




  ObjA3 a3;
  GenerateSql ga3(GenerateSql::Query, mobs::ConvObjToString().exportPrefix());
  a3.traverse(ga3);
  LOG(LM_INFO, "AAAAA " << ga3.result());



  GenerateSql ga4(GenerateSql::Query, mobs::ConvObjToString().exportPrefix());
  ga4.parentMode = true;
  a3.oa2.ooo[0].traverseKey(ga4);
  LOG(LM_INFO, "KKKK " << ga4.result());

  GenerateSql gp3(GenerateSql::Query, mobs::ConvObjToString().exportPrefix());
  gp3.parentMode = true;
  gp3.startVec = &a3.oa2.ooo;
  a3.oa2.ooo.traverse(gp3);
  LOG(LM_INFO, "PPPP " << gp3.result());


  GenerateSql ga5(GenerateSql::Query, mobs::ConvObjToString().exportPrefix());
  ga5.parentMode = true;
  a3.oa2.ooo.traverseKey(ga5);
  LOG(LM_INFO, "KKKK2 " << ga5.result());


}



}

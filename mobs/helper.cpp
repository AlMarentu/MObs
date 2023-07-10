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

#include "helper.h"
#include "audittrail.h"
#include "queryorder.h"
#include "querygenerator.h"
#include "converter.h"
#include "xmlwriter.h"
//#include <strstream>

using namespace mobs;
using namespace std;



namespace {
const size_t Cleaning = SIZE_T_MAX -1;

string vecTableName(const mobs::MemBaseVector *v, const string &name) {
  mobs::MemVarCfg c = v->hasFeature(mobs::ColNameBase);
  if (c and v->getParentObject())
    return v->getParentObject()->getConf(c);
  else
    return name;
}

class GenerateSqlJoin : virtual public ObjTravConst {
public:

  explicit GenerateSqlJoin(const ConvObjToString& c, SQLDBdescription &sqlDbDescription) :
          cth(c.exportPrefix().exportAltNames()), sqldb(sqlDbDescription)  {}



  /// \private
  bool doObjBeg(const ObjectBase &obj) final
  {
//    LOG(LM_INFO, "OBEG " << level);
    if (level == 0) {
      mobs::MemVarCfg c = obj.hasFeature(mobs::ColNameBase);
      if (c)
        masterName = obj.getConf(c);
      else
        masterName = obj.getObjectName();

      class ObKey  : virtual public mobs::ObjTravConst {
      public:
        ObKey(ConvObjToString &c, vector<string> &k) : cth(c), keys(k) {}
        bool doObjBeg(const mobs::ObjectBase &obj) override { return true; }
        void doObjEnd(const mobs::ObjectBase &obj) override {  }
        bool doArrayBeg(const mobs::MemBaseVector &vec) override {  return false; }
        void doArrayEnd(const mobs::MemBaseVector &vec) override { }
        void doMem(const mobs::MemberBase &mem) override { keys.push_back(mem.getName(cth)); }
        ConvObjToString &cth;
        vector<string> &keys;
      };
      ObKey ok(cth, keys);
      obj.traverseKey(ok);
      tableName.push_back(masterName);
      useName.emplace_back("mt");
      for (const auto& k:keys) {
        if (not selectKeys.empty())
          selectKeys += ",";
        selectKeys += "mt.";
        selectKeys += k;
      }
    }
    else {
      if (inArray() and arrayIndex() > 0)
        return false;
      if (obj.hasFeature(mobs::DbDetail))
        return false;
      if (obj.isNull() and cth.omitNull())
        return false;
      if (obj.hasFeature(mobs::DbJson)) {
        if (cth.modOnly()) // where nur in QBE erzeugen
        {
          if (not obj.isNull())
            throw runtime_error(u8"Query on DBJSON element not allowed");
          if (not selectWhere.empty())
            selectWhere += " and ";
          size_t last = useName.size() - 1;
          selectWhere += useName[last] + "." + obj.getName(cth);
          string val = sqldb.valueStmtText("", true);
          if (sqldb.changeTo_is_IfNull)
            selectWhere += " is ";
          else
            selectWhere += "=";
          selectWhere += val;
        }
        return false;
      }
    }
    level++;
    return true;
  };
  /// \private
  void doObjEnd(const ObjectBase &obj) final
  {
    level--;
  };
  /// \private
  bool doArrayBeg(const MemBaseVector &vec) final
  {
//    LOG(LM_INFO, "VECTOR ");
    if (vec.hasFeature(mobs::DbDetail))
      return false;
    if (vec.isNull() and cth.omitNull())
      return false;
    if (vec.hasFeature(mobs::DbJson)) {
      if (cth.modOnly()) // where nur in QBE erzeugen
      {
        if (not vec.isNull())
          throw runtime_error(u8"Query on DBJSON element not allowed");
        if (not selectWhere.empty())
          selectWhere += " and ";
        size_t last = useName.size() - 1;
        selectWhere += useName[last] + "." + vec.getName(cth);
        string val;
        sqldb.valueStmtText("", true);
        if (sqldb.changeTo_is_IfNull)
          selectWhere += " is ";
        else
          selectWhere += "=";
        selectWhere += val;
      }
      return false;
    }
    if (noJoin)
      return false;
    if (inArray() and arrayIndex() > 0)
      return false;

    string name = tableName[tableName.size() -1];
    name += "_";
    name += vec.getElementName();
    tableName.push_back(name);
    mobs::MemVarCfg c = vec.hasFeature(mobs::ColNameBase);
    if (c and vec.getParentObject())
      name = vec.getParentObject()->getConf(c);
    useName.push_back(sqldb.tableName(name));
    keys.push_back(vec.getName(cth));
    arrayLevelJoin.push(false);
    level++;
    return true;
  };
  /// \private
  void doArrayEnd(const MemBaseVector &vec) final
  {
    level--;
    keys.pop_back();
    if (not arrayLevelJoin.empty()) {
      if (arrayLevelJoin.top()) { // Join benötigt
        size_t last = useName.size() - 1;
        selectJoin += " left join ";
        selectJoin += useName[last] + " on ";
        bool fst = false;
        for (auto const &k:keys) {
          if (fst)
            selectJoin += " and ";
          fst = true;
          selectJoin += STRSTR(useName[last - 1] << '.' << k << " = " << useName[last] << '.' << k);
        }
      }
      arrayLevelJoin.pop();
    }
    tableName.pop_back();
    useName.pop_back();
  };
  /// \private
  void doMem(const MemberBase &mem) final
  {
//    LOG(LM_INFO, "MMM " << level);
    if (inArray() and arrayIndex() > 0)
      return;

    bool compact = cth.compact();
    if (mem.is_chartype(cth) and mem.hasFeature(mobs::DbCompact))
      compact = true;
    string name = mem.getName(cth);
    size_t last = useName.size() -1;
    u_int pos;
    int dir;
    if (sort and sort->sortInfo(mem, pos, dir)) {
      selectOrder[pos] = STRSTR(useName[last] << '.' << name << (dir > 0? "": " descending"));
      if (sqldb.orderInSelect) {
        if (not arrayLevelJoin.empty() or not mem.keyElement())
          selectKeysXtra += STRSTR(',' << useName[last] << '.' << name);
        if (not arrayLevelJoin.empty())
          selectFieldXtra += STRSTR(',' << useName[last] << '.' << name);
      }
//      LOG(LM_INFO, "SORT " << pos << " " << selectOrder[pos]);
      if (not arrayLevelJoin.empty())
        arrayLevelJoin.top() = true;
    }
    auto ql = queryLookUp.find(&mem);
    if (ql != queryLookUp.end()) {
      ql->second = useName[last];
      ql->second += ".";
      ql->second += name;
      LOG(LM_INFO, "WHERE " << ql->second);
      if (not arrayLevelJoin.empty())
        arrayLevelJoin.top() = true;
    }
    if (arrayLevelJoin.empty()) {
      if (not selectField.empty())
        selectField += ",";
      selectField += "mt.";
      selectField += name;
    }
    if (not mem.isModified() and cth.modOnly())
      return;
    if (mem.isNull() and cth.omitNull())
      return;

    if (mem.isVersionField()) {
      if (cth.skipVersion())
        return;
    }

    if (not cth.modOnly()) // where nur in QBE erzeugen
      return;

    if (not arrayLevelJoin.empty())
      arrayLevelJoin.top() = true;
    if (not selectWhere.empty())
      selectWhere += " and ";
    selectWhere += useName[last] + "." + name;
    string val = sqldb.valueStmt(mem, compact, false, true);
    if (sqldb.changeTo_is_IfNull and mem.isNull())
      selectWhere += " is ";
    else
      selectWhere += "=";
    selectWhere += val;
  };
  void addWhere(const string &where) {
    if (not selectWhere.empty()) {
      selectWhere += " and (";
      selectWhere += where;
      selectWhere += ")";
    } else
      selectWhere = where;
  }
  string result(bool count, bool keyMode) {
    string s;
    if (count) {
      if (selectJoin.empty())
        s += "select count(*) from ";
      else {
        s += "select count(distinct ";
        s += selectKeys;
        s += ") from ";
      }
    } else {
      s += "select ";
      if (not selectJoin.empty())
        s += "distinct ";
      if (keyMode)
        s += selectKeys;
      else
        s += selectField;
      if (keyMode)
        s += selectKeysXtra;
      else
        s += selectFieldXtra;
      s += " from ";
    }
    s += sqldb.tableName(masterName);
    s += " mt ";
    s += selectJoin;
    if (not selectWhere.empty()) {
      s += " where ";
      s += selectWhere;
    }
    if (not count and not selectOrder.empty()) {
      s += " order by ";
      string del;
      for (auto &o:selectOrder) {
        s += del;
        del = ",";
        s += o.second;
      }
    }
    s += injectEnd;
    s += ";";
    return s;
  }

  string selectKeys;
  string selectField;
  string selectWhere;
  string selectJoin;
  string masterName;
  string selectKeysXtra;  ///< für orderInSelect
  string selectFieldXtra; ///< für orderInSelect
  map<u_int, string>  selectOrder;
  bool noJoin = false;  // ersetze joinGenerierung
  const QueryOrder *sort = nullptr;
  const QueryGenerator *queryGen = nullptr;
  std::string injectEnd;    ///< wird bei QueryWithJoin am Ende angehängt
  map<const MemberBase *, string> queryLookUp;

private:
  ConvObjToString cth;
  int level = 0;
  vector<string> tableName;
  vector<string> useName;
  vector<string> keys;
  stack<bool> arrayLevelJoin; // wird der Join des aktuellen Array-levels benötigt?
  SQLDBdescription &sqldb;
};


class ExtractSql : virtual public ObjTrav {
public:
  explicit ExtractSql(SQLDBdescription &s, const ConvObjToString& c) : current(nullptr, "", {}),
                                                                       cth(std::move(c.exportPrefix().exportAltNames())),
                                                                       sqldb(s) { }

  bool doObjBeg(ObjectBase &obj) final
  {
    if (level == 0) {
      level++;
      return true;
    }
    if (obj.hasFeature(mobs::DbDetail))
      return false;
    if (obj.hasFeature(mobs::DbJson)) {
      bool null;
      std::string tx;
      sqldb.readValueText(obj.getName(cth), tx, null);
      if (null)
        obj.forceNull();
      else
        string2Obj(tx, obj, ConvObjFromStr().useExceptUnknown());
      return false;
    }
      level++;
    return true;
  };

  void doObjEnd(ObjectBase &obj) final
  {
    level--;
  };

  bool doArrayBeg(MemBaseVector &vec) final
  {
    if (level == 0) {
//      LOG(LM_DEBUG, "START VEC " << vec.getName(cth));
      level++;
      return true;
    }
    if (vec.hasFeature(mobs::DbDetail))
      return false;
    if (vec.hasFeature(mobs::DbJson)) {
      bool null;
      std::string tx;
      sqldb.readValueText(vec.getName(cth), tx, null);
      if (null)
        vec.forceNull();
      else {  // Input und Vector zu einem Objekt umbauen
        string t = "{";
        t += vec.getElementName();
        t += ":";
        t += tx;
        t += "}";
        ObjectBase dummy{};
        dummy.regArray(&vec);
        string2Obj(t, dummy, ConvObjFromStr().useExceptUnknown());
      }
      return false;
    }

//    LOG(LM_INFO, "VECTOR FOUND = " << vec.getName(cth));
    vector<pair<string, size_t>> k = current.arrayKeys;
    size_t index = SIZE_T_MAX;
    k.emplace_back(make_pair(vec.getName(cth), index));
    detailVec.emplace_back(&vec, current.tableName + "_" + vec.getElementName(), k);
    // übergeordnete Vektoren auf Size=1 setzten, damit die Struktur für das nächste Statement vorhanden ist
    vec.resize(1);
    return false;
  };

  void doArrayEnd(MemBaseVector &vec) final
  {
    level--;
  };

  void doMem(MemberBase &mem) final
  {
    bool compact = cth.compact();
    if (mem.is_chartype(cth) and mem.hasFeature(mobs::DbCompact))
      compact = true;

    sqldb.readValue(mem, compact);
  };

//  std::string keyPrefix; // = "mt."; ///< für master table

  list<SqlGenerator::DetailInfo> detailVec;

  SqlGenerator::DetailInfo current;
private:
  ConvObjToString cth;
  SQLDBdescription &sqldb;
  int level = 0;
};



/////////////////////////////////////////////

class GenerateSql : virtual public ObjTravConst {
public:
  enum Mode { Where, Fields, Values, Update, Create, FldVal, FldVal2 };
  explicit GenerateSql(Mode m, SQLDBdescription &s, const ConvObjToString& c) : current(nullptr, "", {}), mode(m),
  cth(c.exportPrefix().exportAltNames()), sqldb(s) { sqldb.startWriting(); }

  ~GenerateSql() { sqldb.finishWriting(); }

  string delimiter() noexcept {  if (not fst) return mode == Where ? " and ": ","; fst = false; return string(); }

  /// \private
  bool doObjBeg(const ObjectBase &obj) final
  {
    if (level == 0) {
//      LOG(LM_DEBUG, "START OBJ " << obj.getName(cth));
      key.push(true);
      level++;
      return true;
    }
    if (obj.hasFeature(mobs::DbDetail))
      return false;
    if (obj.isNull() and cth.omitNull())
      return false;
    if (obj.hasFeature(mobs::DbJson)) {
      string tx = obj.to_string(ConvObjToString().exportExtended().exportWoNull());
      if (obj.keyElement())
        throw runtime_error("SQL: Key with DBJSON-element not allowed");
      if (mode == Values) {
        res << delimiter() << sqldb.valueStmtText(tx, obj.isNull());
        mobs::MemVarCfg c = obj.hasFeature(mobs::LengthBase);
        if (c and tx.length() > (c - mobs::LengthBase))
          throw runtime_error("SQL: DBJSON-element to big für column");
        return false;
      }
      string name = obj.getName(cth);
      if (mode == Where) {
        if (not obj.isNull())
          throw runtime_error("SQL: Query with DBJSON-element not allowed");
        res << delimiter() << name;
        string val = sqldb.valueStmtText(tx, obj.isNull());
        if (sqldb.changeTo_is_IfNull)
          res << " is ";
        else
          res << "=";
        res << val;
      } else if (mode == Update) {
        res << delimiter() << name << "=" << sqldb.valueStmtText(tx, obj.isNull());
      } else if (mode == Fields) {
        res << delimiter() << name;
      } else if (mode == FldVal or mode == FldVal2) {
        string d = delimiter();
        res << d << name;
        if (mode == FldVal2 and not d.empty())
          d = " and ";
        res2 << d << name << "=?";
        fields++;
        sqldb.valueStmtText(tx, obj.isNull());
        mobs::MemVarCfg c = obj.hasFeature(mobs::LengthBase);
        if (c and tx.length() > (c - mobs::LengthBase))
          throw runtime_error("SQL: DBJSON-element to big für column");
      } else if (mode == Create)  {
        if (dupl.find(mobs::toUpper(name)) != dupl.end())
          throw runtime_error(name + u8" is a duplicate id in SQL statement, use ALTNAME");
        dupl.insert(mobs::toUpper(name));
        mobs::MemVarCfg c = obj.hasFeature(mobs::LengthBase);
        size_t n = c ? (c - mobs::LengthBase) : 100;
        res << delimiter() << name << " " << sqldb.createStmtText(name, n);
      }
      return false;
    }
    if (not obj.isModified() and cth.modOnly())
      return false;
    if (mode != Values and mode != FldVal and inArray() and arrayIndex() > 0)
      return false;
    level++;
    key.push((obj.keyElement()));
    return true;
  };
  /// \private
  void doObjEnd(const ObjectBase &obj) final
  {
    level--;
    key.pop();
  };
  /// \private
  bool doArrayBeg(const MemBaseVector &vec) final
  {
    vector<pair<string, size_t>> k = current.arrayKeys;
//std::stringstream s; for (auto i:k)  s << " " << i.first << "," << i.second; LOG(LM_INFO, "KKKKKK " << s.str());
    if (level == 0) {
//      LOG(LM_DEBUG, "START VEC " << vec.getName(cth));
      key.push(true);
      // neuen Array-Namen hinten Liste anhängen
      for (const auto &i:k) {
        string name = i.first;
        if (mode == Values) {
          res << delimiter() << sqldb.valueStmtIndex(i.second);
        } else if (mode == FldVal2) {
          size_t index = i.second;
          if (index != SIZE_T_MAX) {
            string d = delimiter();
            res << d << name;
            res3 << d << name;
            if (not d.empty())
              d = " and ";
            res2 << d << name << "=" << sqldb.valueStmtIndex(index);
            fields++;
          }
        } else if (mode == Where) {
          size_t index = i.second;
          if (current.cleaning) {
            size_t sz = vec.size();
            if (sz and index == Cleaning)
              res << delimiter() << name << ">" << sqldb.valueStmtIndex(sz-1);
            else if (index != SIZE_T_MAX)
              res << delimiter() << name << "=" << sqldb.valueStmtIndex(index);
          }
          else if (index != SIZE_T_MAX)
            res << delimiter() << name << "=" << sqldb.valueStmtIndex(index);
        }
        else if (mode == Create) {
          res << delimiter() << name << " " << sqldb.createStmtIndex(i.first);
          if (dupl.find(mobs::toUpper(name)) != dupl.end())
            throw runtime_error(name + u8" is a duplicate id in SQL statement, use ALTNAME");
          dupl.insert(mobs::toUpper(name));
        }
        else if (mode == Fields) {
          res << delimiter() << name;
          res2 << name; // für order by
//        } else if (mode == Update) {
          // Keinen Key ausgeben
        }
//        if (mode = Values) {
//          size_t index = SIZE_T_MAX;
//          if (k.empty())
//            throw runtime_error("key list empty");
//          index = k.back().second;
//
//        }
      }

      bool cleaning = current.cleaning;
      if (not k.empty() and (mode == Values or mode == Update or mode == FldVal)) {
        size_t &currentIndex = k.back().second;
        LOG(LM_DEBUG, "IN ARRAY " << currentIndex);
        if (currentIndex == SIZE_T_MAX)
          return true;
        if (++currentIndex < vec.size()) {
          LOG(LM_DEBUG, "ARRAY FORTSETZUNG FOLGT " << currentIndex);
          const ObjectBase *vobj = vec.getObjInfo(currentIndex);
          if (vobj and vobj->isNull()) {
            for (;;) {  // Alle elemente = null löschen
              if (withCleaner and (not optimize or vobj->isModified()))
                detailVec.emplace_back(&vec, current.tableName, k, true);
              if (++currentIndex >= vec.size())
                break;
              vobj = vec.getObjInfo(currentIndex);
              if (not vobj or not vobj->isNull())
                break;
            }
          }
        }
        if (currentIndex >= vec.size()) {
          LOG(LM_DEBUG, "ARRAY ENDE DER VERANSTALTUNG " << currentIndex);
          cleaning = true;
          currentIndex = Cleaning;
          if (not withCleaner)
            return true;
        }
        detailVec.emplace_back(&vec, current.tableName, k, cleaning);
      }

      if (mode == Where or mode == FldVal2)
        return false;     // Nur keys machen
      level++;
      return true;
    } // Ende level == 0

    if (vec.hasFeature(mobs::DbDetail))
      return false;
    if (mode == Update and optimize and not vec.isModified())
      return false;
    if (vec.hasFeature(mobs::DbJson)) {
      string tx = vec.to_string(ConvObjToString().exportExtended().exportWoNull());
      if (mode == Values) {
        res << delimiter() << sqldb.valueStmtText(tx, vec.isNull());
        return false;
      }
      string name = vec.getName(cth);
      if (mode == Where) {
        if (not vec.isNull())
          throw runtime_error("SQL: Query with DBJSON-element not allowed");
        res << delimiter() << name;
        string val = sqldb.valueStmtText(tx, vec.isNull());
        if (sqldb.changeTo_is_IfNull)
          res << " is ";
        else
          res << "=";
        res << val;
      } else if (mode == Update) {
        res << delimiter() << name << "=" << sqldb.valueStmtText(tx, vec.isNull());
      } else if (mode == Fields) {
        res << delimiter() << name;
      } else if (mode == FldVal or mode == FldVal2) {
        string d = delimiter();
        res << d << name;
        if (mode == FldVal2 and not d.empty())
          d = " and ";
        res2 << d << name << "=";
        fields++;
        res2 << sqldb.valueStmtText(tx, vec.isNull());
      } else if (mode == Create)  {
        if (dupl.find(mobs::toUpper(name)) != dupl.end())
          throw runtime_error(name + u8" is a duplicate id in SQL statement, use ALTNAME");
        dupl.insert(mobs::toUpper(name));
        mobs::MemVarCfg c = vec.hasFeature(mobs::LengthBase);
        size_t n = c ? (c - mobs::LengthBase) : 100;
        res << delimiter() << name << " " << sqldb.createStmtText(name, n);
      }
      return false;
    }

    size_t index = SIZE_T_MAX;
    bool cleaning = current.cleaning;
    if (mode == Values or mode == Update or mode == FldVal) {
      // wenn Subarray elemente hat dann 0 zurück, ansonsten MAX für Aufräumer
      size_t sz = vec.size();
      if (sz == 0)
        cleaning = true;
      else {
        index = 0;
        const ObjectBase *vobj = vec.getObjInfo(0);
        if (vobj and vobj->isNull()) {
          cleaning = true;
        }
      }
      LOG(LM_DEBUG, "ARRAY VALUES " << index << " / " << sz);
    }

    if (not withCleaner and index == SIZE_T_MAX)
      return false;
    k.emplace_back(make_pair(vec.getName(cth), index));
    detailVec.emplace_back(&vec, current.tableName + "_" + vec.getElementName(), k, cleaning);
    return false;
  };

  /// \private
  void doArrayEnd(const MemBaseVector &vec) final
  {
    level--;
  };
  /// \private
  void doMem(const MemberBase &mem) final
  {
    if (mem.isNull() and cth.omitNull())
      return;
    if (not mem.isModified() and cth.modOnly())
      return;
    if (mem.isVersionField()) {
      if (cth.skipVersion())
        return;
    }
    if (mode != Values and mode != FldVal and inArray() and arrayIndex() > 0)
      return;
    bool compact = cth.compact();
    if (mem.is_chartype(cth) and mem.hasFeature(mobs::DbCompact))
      compact = true;
    if (mode == Values) {
      res << delimiter() << sqldb.valueStmt(mem, compact, mem.isVersionField(), false);
      return;
    }
    string name = mem.getName(cth);
    if (name.empty()) { // ist bei Arrays von MemberVars üblich
      if (mem.getParentVector())
        name = mem.getParentVector()->getName(cth);
      name += "Value";
    }
    if ((mode == Update or mode == FldVal) and mem.keyElement() and key.top()) { // Key or version element
      if (mode == FldVal and mem.isVersionField())  // version increment
        res2 << name << '=' << name << "+1,";
      if (mode == FldVal or not mem.isVersionField())
        return;
    }

    if (mode == Where) {
      res << delimiter() << name;
      string val = sqldb.valueStmt(mem, compact, false, true);
      if (sqldb.changeTo_is_IfNull and mem.isNull())
        res << " is ";
      else
        res << "=";
      res << val;
    } else if (mode == Update) {
        res << delimiter() << name << "=" << sqldb.valueStmt(mem, compact, mem.isVersionField(), false);
    } else if (mode == Fields) {
      res << delimiter() << name;
    } else if (mode == FldVal or mode == FldVal2) {
      string d = delimiter();
      res << d << name;
      if (mode == FldVal2)
        res3 << d << name;
      if (mode == FldVal2 and not d.empty())
        d = " and ";
      if (mode == FldVal and mem.isVersionField())
        res2 << d << name << "=";
      else
        res2 << d << name << "=";
      fields++;
      res2 << sqldb.valueStmt(mem, compact, mem.isVersionField(), false);
    } else if (mode == Create)  {
      if (dupl.find(mobs::toUpper(name)) != dupl.end())
        throw runtime_error(name + u8" is a duplicate id in SQL statement, use ALTNAME");
      dupl.insert(mobs::toUpper(name));
      res << delimiter() << name << " " << sqldb.createStmt(mem, compact);
    }
  };

  void completeInsert() {
    for (u_int i = 0; i < fields; i++)
      res << delimiter() << '?';
  }
  string result() { return res.str(); }
  string result2() { return res2.str(); }
  string result3() { return res3.str(); }
  void setMode(Mode m) { mode = m; }
  void addText(const string &tx) { res << tx; fst = true; }
  void addText2(const string &tx) { res2 << tx; fst = true; }
  void addTextAll(const string &tx) { res << tx; res2 << tx; fst = true; }
  list<SqlGenerator::DetailInfo> detailVec;

  SqlGenerator::DetailInfo current;
  bool withCleaner = true;
  bool optimize = true; // Update

private:
  enum Mode mode;
  ConvObjToString cth;
  set<string> dupl;
  stack<bool> key;
  bool fst = true;
  int level = 0;
  stringstream res;
  stringstream res2;
  stringstream res3;
  SQLDBdescription &sqldb;
  u_int fields = 0;
};


}

namespace mobs {

SqlGenerator::DetailInfo::DetailInfo(const MemBaseVector *v, std::string t,
                                     std::vector<std::pair<std::string, size_t>> k, bool c) :
        vec(v), vecNc(nullptr), tableName(std::move(t)), arrayKeys(std::move(k)), cleaning(c) {}

SqlGenerator::DetailInfo::DetailInfo(mobs::MemBaseVector *v, std::string t,
                                     std::vector<std::pair<std::string, size_t>> k) :
        vec(v), vecNc(v), tableName(std::move(t)), arrayKeys(std::move(k)), cleaning(false) {}

SqlGenerator::DetailInfo::DetailInfo() : vec(nullptr), vecNc(nullptr), tableName(string()), arrayKeys({}), cleaning(false) {}

string SqlGenerator::tableName() const {
  mobs::MemVarCfg c = obj.hasFeature(mobs::ColNameBase);
  if (c)
    return obj.getConf(c);
  else
    return obj.getObjectName();
}

string SqlGenerator::doDelete(SqlGenerator::DetailInfo &di) {
  GenerateSql gs(GenerateSql::Where, sqldb, mobs::ConvObjToString());
  gs.current = di;
  gs.withCleaner = not di.cleaning; // nicht doppelt cleanen
  gs.addText("delete from ");

  if (di.vec) {
    gs.addText(sqldb.tableName(vecTableName(di.vec, di.tableName)));
  }
  else
    gs.addText(sqldb.tableName(di.tableName));

  gs.addText(" where ");
  if (not di.vec)
    gs.withVersionField = true;
  obj.traverseKey(gs);
  if (di.vec) {
    di.vec->traverse(gs);
  }
  gs.addText(";");
  string s = gs.result();
 
  gs.setMode(GenerateSql::Fields);
  gs.detailVec.clear();
  if (di.vec) {
    gs.current.vec->traverse(gs);
  }
  else {
    // Auf Schattenobjekt arbeiten, um alle Vektoren auf Size=1 zu setzen
    // Objekt muss während der Verarbeitung stabil bleiben (C-Pointer)
    ObjectBase *o2 = obj.createNew();
    deleteLater(o2);
    SetArrayStructure sas;
    o2->traverse(sas);
    o2->traverse(gs);
  }
  detailVec.splice(detailVec.end(), gs.detailVec);
  return s;
}

SqlGenerator::~SqlGenerator() {
  for (auto o:m_deleteLater)
    delete o;
}

string SqlGenerator::doUpdate(SqlGenerator::DetailInfo &di) {
  if (di.cleaning)
    return doDelete(di);

  GenerateSql gs(GenerateSql::Update, sqldb, mobs::ConvObjToString());
  gs.current = di;

  gs.addText("update ");

  if (di.vec) {
    gs.addText(sqldb.tableName(vecTableName(di.vec, di.tableName)));
  } else
    gs.addText(sqldb.tableName(di.tableName));

  gs.addText(" set ");
  gs.detailVec.clear();
  if (di.vec) {
    size_t index  = 0;
    if (not di.arrayKeys.empty())
      index = di.arrayKeys.back().second;
    gs.current.vec->traverseSingle(gs, index);
  }
  else
    obj.traverse(gs);
  detailVec.splice(detailVec.end(), gs.detailVec);

  gs.addText(" where ");
  gs.setMode(GenerateSql::Where);
  if (not di.vec)
    gs.withVersionField = true;
  obj.traverseKey(gs);
  if (di.vec) {
    di.vec->traverse(gs);
  }
  gs.detailVec.clear();
  gs.addText(";");
  return gs.result();
}





string SqlGenerator::doInsertUpd(SqlGenerator::DetailInfo &di, std::string &upd) {
  upd = "";
  if (di.cleaning)
    return doDelete(di);

  GenerateSql gs(GenerateSql::FldVal, sqldb, mobs::ConvObjToString());
  gs.current = di;

  gs.addText("insert into ");
  gs.addText2("update ");

  if (di.vec) {
    if (sqldb.withInsertOnConflict)
      gs.addText(sqldb.tableName(vecTableName(di.vec, di.tableName)));
    else
      gs.addTextAll(sqldb.tableName(vecTableName(di.vec, di.tableName)));
  } else {
    if (sqldb.withInsertOnConflict)
      gs.addText(sqldb.tableName(di.tableName));
    else
      gs.addTextAll(sqldb.tableName(di.tableName));
  }
  gs.addText("(");
  gs.addText2(" set ");
  gs.detailVec.clear();
  if (di.vec) {
    size_t index  = 0;
    if (not di.arrayKeys.empty())
      index = di.arrayKeys.back().second;
    LOG(LM_INFO, "TRAVERSE CURRENT INDEX " << index);
    gs.current.vec->traverseSingle(gs, index);
  }
  else
    obj.traverse(gs);
  detailVec.splice(detailVec.end(), gs.detailVec);

  gs.setMode(GenerateSql::FldVal2);
  gs.addText(",");
  gs.addText2(" where ");
  if (not di.vec)
    gs.withVersionField = true;
  obj.traverseKey(gs);
  if (di.vec) {
    di.vec->traverse(gs);
  }
  gs.detailVec.clear();

  gs.addText(") values (");
  gs.completeInsert();

  gs.addText(")");
  if (sqldb.withInsertOnConflict) {
    gs.addText(" ON CONFLICT (");
    gs.addText(gs.result3());
    gs.addText(") DO ");
    gs.addText(gs.result2());
  }

  gs.addTextAll(";");
  upd = gs.result2();
  return gs.result();
}





string SqlGenerator::doInsert(SqlGenerator::DetailInfo &di, bool replace) {
//  size_t vecSz = 0;
  if (di.cleaning)
    return doDelete(di);

//  if (di.vec) {
//    vecSz = di.vec->size();
//    if (not di.arrayKeys.empty())
//      index = di.arrayKeys.back().second;
//  }

  GenerateSql gs(GenerateSql::Fields, sqldb, mobs::ConvObjToString());
  gs.current = di;
  if (replace) {
    gs.addText("replace ");
    if (sqldb.replaceWithInto)
      gs.addText("into ");
  } else {
    gs.withCleaner = false;

    gs.addText("insert into ");
  }
  if (di.vec) {
    gs.addText(sqldb.tableName(vecTableName(di.vec, di.tableName)));
//    vecSz = di.vec->size();

  }
  else
    gs.addText(sqldb.tableName(di.tableName));

  gs.addText("(");
  if (di.vec) {
    obj.traverseKey(gs);
    di.vec->traverse(gs);
  }
  else
    obj.traverse(gs);
  gs.addText(") VALUES (");

  gs.setMode(GenerateSql::Values);
  if (di.vec) {
    obj.traverseKey(gs);
  }
  gs.detailVec.clear();
  if (di.vec) {
    size_t index  = 0;
    if (not di.arrayKeys.empty())
      index = di.arrayKeys.back().second;
    LOG(LM_INFO, "TRAVERSE CURRENT INDEX " << index);
    gs.current.vec->traverseSingle(gs, index);
  }
  else
    obj.traverse(gs);
  detailVec.splice(detailVec.end(), gs.detailVec);
  gs.addText(");");

  return gs.result();
}


string SqlGenerator::insertStatement(bool first) {
  string s;
  if (first) {
    detailVec.clear();
    DetailInfo di(nullptr, tableName(), {});
    s = doInsert(di, false);
  } else if (eof())
    return "";
  else {
    s = doInsert(detailVec.front(), false);
    detailVec.erase(detailVec.begin());
  }
  return s;
}

string SqlGenerator::replaceStatement(bool first) {
  string s;
  string upd;
  if (first) {
    detailVec.clear();
    DetailInfo di(nullptr, tableName(), {});
    if (sqldb.withInsertOnConflict)
      s = doInsertUpd(di, upd);
    else
      s = doInsert(di, true);
  } else if (eof())
    return "";
  else {
    if (sqldb.withInsertOnConflict)
      s = doInsertUpd(detailVec.front(), upd);
    else
      s = doInsert(detailVec.front(), true);
    detailVec.erase(detailVec.begin());
  }
  return s;
}

string SqlGenerator::updateStatement(bool first) {
  string s;
  if (first) {
    detailVec.clear();
    DetailInfo di(nullptr, tableName(), {});
    s = doUpdate(di);
  } else if (eof())
    return "";
  else {
//    s = doInsert(detailVec.front(), true);
    s = doUpdate(detailVec.front());
    detailVec.erase(detailVec.begin());
  }
  return s;
}

string SqlGenerator::deleteStatement(bool first) {
  string s;
  if (first) {
    detailVec.clear();
    DetailInfo di(nullptr, tableName(), {}, false);
    s = doDelete(di);
  } else if (eof())
    return "";
  else {
    s = doDelete(detailVec.front());
    detailVec.erase(detailVec.begin());
  }
  return s;
}




string SqlGenerator::doDrop(SqlGenerator::DetailInfo &di) {
  GenerateSql gs(GenerateSql::Create, sqldb, mobs::ConvObjToString());
  gs.current = di;
  gs.addText("drop table ");
  if (sqldb.dropWith_IfExists)
    gs.addText("if exists ");
  if (di.vec)
    gs.addText(sqldb.tableName(vecTableName(di.vec, di.tableName)));
  else
    gs.addText(sqldb.tableName(di.tableName));
  gs.addText(";");
  string s = gs.result();

  gs.detailVec.clear();
  if (di.vec)
    di.vec->traverse(gs);
  else
    obj.traverse(gs);
  detailVec.splice(detailVec.end(), gs.detailVec);

  return s;
}


string SqlGenerator::dropStatement(bool first) {
  string s;
  if (first) {
    detailVec.clear();
    DetailInfo di(nullptr, tableName(), {});
    s = doDrop(di);
  } else if (eof())
    return "";
  else {
    s = doDrop(detailVec.front());
    detailVec.erase(detailVec.begin());
  }
  return s;
}

string SqlGenerator::doCreate(SqlGenerator::DetailInfo &di) {
  GenerateSql gs(GenerateSql::Create, sqldb, mobs::ConvObjToString());
  gs.current = di;
  gs.addText("create table ");
  if (sqldb.createWith_IfNotExists)
    gs.addText("if not exists ");
  if (di.vec)
    gs.addText(sqldb.tableName(vecTableName(di.vec, di.tableName)));
  else
    gs.addText(sqldb.tableName(di.tableName));
  gs.addText("(");
  if (di.vec) {
    obj.traverseKey(gs);
  }
  gs.detailVec.clear();
  if (di.vec)
    di.vec->traverse(gs);
  else {
    // Auf Schattenobjekt arbeiten, um alle Vektoren auf Size=1 zu setzen
    // Objekt muss während der Verarbeitung stabil bleiben (C-Pointer)
    ObjectBase *o2 = obj.createNew();
    deleteLater(o2);
    SetArrayStructure sas;
    o2->traverse(sas);
    o2->traverse(gs);
  }
  detailVec.splice(detailVec.end(), gs.detailVec);

  gs.addText(", primary key (");
  gs.setMode(GenerateSql::Fields);
  obj.traverseKey(gs);
  if (di.vec) {
    gs.doArrayBeg(*di.vec);
  }
  gs.addText("));");
  return gs.result();
}

string SqlGenerator::createStatement(bool first) {
  string s;
  if (first) {
    detailVec.clear();
    DetailInfo di(nullptr, tableName(), {});
    s = doCreate(di);
  } else if (eof())
    return "";
  else {
    s = doCreate(detailVec.front());
    detailVec.erase(detailVec.begin());
  }
  return s;
}

string SqlGenerator::selectStatementFirst(bool keys) {
  GenerateSql gs(GenerateSql::Fields, sqldb, mobs::ConvObjToString());
  gs.withCleaner = false;
  gs.addText("select ");
  if (keys)
    obj.traverseKey(gs);
  else
    obj.traverse(gs);
  gs.addText(" from ");
  gs.addText(sqldb.tableName(tableName()));
  gs.addText(" where ");
  gs.setMode(GenerateSql::Where);
  gs.withVersionField = false;
  obj.traverseKey(gs);
  gs.detailVec.clear();
  gs.addText(";");
  detailVec.clear();
  return gs.result();
}


string SqlGenerator::doSelect(SqlGenerator::DetailInfo &di) {
  if (not di.vec)
    throw runtime_error("error in doSelect");
  GenerateSql gs(GenerateSql::Fields, sqldb, mobs::ConvObjToString());
  // In ExtractSql wurden die Vektoren der nächsten Ebene auf Size=1 gesetzt
  gs.withCleaner = false;
  gs.current = di;
  // bei Select mit Laufindex des aktuellen Objektes beginnen
  while (gs.current.arrayKeys.size() > 1)
    gs.current.arrayKeys.erase(gs.current.arrayKeys.begin());
  gs.addText("select ");
  di.vec->traverse(gs);
  gs.addText(" from ");
  gs.addText(sqldb.tableName(vecTableName(di.vec, di.tableName)));
  gs.addText(" where ");
  gs.setMode(GenerateSql::Where);
  // bei der Where-Bedingung wieder alle Keys prüfen
  gs.current = di;
  obj.traverseKey(gs);
  di.vec->traverse(gs);
  gs.detailVec.clear();
  gs.addText(" order by ");
  gs.addText(gs.result2());
  gs.addText(";");
  return gs.result();
}


void SqlGenerator::readObject(const DetailInfo &di) {
  MemBaseVector *vec = di.vecNc;
  // indexWert aus db lesen
  if (vec == nullptr or di.arrayKeys.empty())
    throw runtime_error("invalid DetailInfo in readObject");

  sqldb.startReading();
  size_t index = sqldb.readIndexValue(vec->getName(mobs::ConvObjToString().exportPrefix().exportAltNames()));
  if (index >= INT_MAX)
    throw runtime_error("no index position in readObject");
  vec->resize(index+1);
  ObjectBase *vobj = vec->getObjInfo(index);
  MemberBase *mobj = nullptr;
  if (not vobj) {
    mobj =vec->getMemInfo(index);
    if (not mobj)
      throw runtime_error("Object missing in readObject");
  }
  ExtractSql es(sqldb, mobs::ConvObjToString());
  // aktuellen Index in di ablegen
  es.current = di;
  es.current.arrayKeys.back().second = index;
  if (vobj)
    vobj->traverse(es);
  else
    mobj->traverse(es);
  sqldb.finishReading();
  detailVec.splice(detailVec.end(), es.detailVec);

}

void SqlGenerator::readObject(mobs::ObjectBase &o) {
  ExtractSql es(sqldb, mobs::ConvObjToString());
  DetailInfo di(nullptr, tableName(), {});
  es.current = di;
  sqldb.startReading();
  o.traverse(es);
  sqldb.finishReading();
  detailVec.splice(detailVec.end(), es.detailVec);
}

void SqlGenerator::readObjectKeys(mobs::ObjectBase &o) {
  ExtractSql es(sqldb, mobs::ConvObjToString());
  DetailInfo di(nullptr, tableName(), {});
  es.current = di;
  sqldb.startReading();
  o.traverseKey(es);
  sqldb.finishReading();
}

std::string SqlGenerator::selectStatementArray(DetailInfo &di) {
  if (eof())
    return "";
  di = detailVec.front();
  string s = doSelect(detailVec.front());
  detailVec.erase(detailVec.begin());
  return s;
}


uint64_t SqlGenerator::getVersion() const {
  class GetVers  : virtual public mobs::ObjTravConst {
  public:
    bool doObjBeg(const mobs::ObjectBase &) override { return true; }
    void doObjEnd(const mobs::ObjectBase &) override {  }
    bool doArrayBeg(const mobs::MemBaseVector &) override {  return false; }
    void doArrayEnd(const mobs::MemBaseVector &) override { }
    void doMem(const mobs::MemberBase &mem) override {
      if (version < 0 and mem.isVersionField()) {
        MobsMemberInfo mi;
        mem.memInfo(mi);
        if (mi.isUnsigned) {
          if (mi.u64 > mi.max)
            throw std::runtime_error("VersionElement overflow");
          version = mi.u64;
        } else if (mi.isSigned) {
          version = mi.i64;
        }
      }
    }
    int64_t version = -1;
  };
  GetVers ok;
  ok.withVersionField = true;
  obj.traverseKey(ok);
  return static_cast<uint64_t>(ok.version);
}

std::string
SqlGenerator::query(QueryMode querMode, const QueryOrder *sort, const QueryGenerator *where, const std::string &join,
                    const std::string &atEnd) {
  GenerateSqlJoin gsjoin((mobs::ConvObjToString()), sqldb);
  gsjoin.injectEnd = atEnd;
  gsjoin.noJoin = not join.empty();
  gsjoin.sort = sort;
  gsjoin.queryGen = where;
  if (where)
    where->createLookup(gsjoin.queryLookUp);
  obj.traverse(gsjoin);
  if (not join.empty())
    gsjoin.selectJoin = join;
  if (where) {
    gsjoin.selectWhere = where->show(gsjoin.queryLookUp, &sqldb);
  }
  querywJoin = not gsjoin.selectJoin.empty();
  return gsjoin.result(querMode == Count, querMode == Keys);
}

std::string
SqlGenerator::queryBE(QueryMode querMode, const QueryOrder *sort, const QueryGenerator *where, const std::string &atEnd) {
  GenerateSqlJoin gsjoin((mobs::ConvObjToString().exportModified()), sqldb);
  gsjoin.injectEnd = atEnd;
  gsjoin.sort = sort;
  gsjoin.queryGen = where;
  if (where)
    where->createLookup(gsjoin.queryLookUp);
  obj.traverse(gsjoin);
  querywJoin = not gsjoin.selectJoin.empty();
  if (where) {
    gsjoin.selectWhere = where->show(gsjoin.queryLookUp, &sqldb);
  }
  return gsjoin.result(querMode == Count, querMode == Keys);
}


std::string SqlGenerator::insertUpdStatement(bool first, string &upd) {
  string s;
  if (first) {
    detailVec.clear();
    DetailInfo di(nullptr, tableName(), {});
    s = doInsertUpd(di, upd);
  } else if (eof())
    return "";
  else {
    s = doInsertUpd(detailVec.front(), upd);
    detailVec.erase(detailVec.begin());
  }
  return s;
}


//////////////////////////////// ElementNames ////////////////////////////////

class ElementNamesData {
public:
  ConvObjToString cth;
  std::stack<std::string> names{};
  const QueryOrder *sort = nullptr;
  std::map<u_int, std::pair<std::string, int>> selectOrder;
  std::map<const MemberBase *, std::string> *lookUp = nullptr;
};

ElementNames::ElementNames(ConvObjToString c) {
  data = std::unique_ptr<ElementNamesData>(new ElementNamesData);
  data->names.push("");
  data->cth = std::move(c);
}

ElementNames::~ElementNames() = default;

bool ElementNames::doObjBeg(const ObjectBase &obj) {
  if (data->sort and inArray() and arrayIndex() > 0)
    return false;
  if (obj.hasFeature(mobs::DbDetail))
    return false;
  if (obj.isNull() and data->cth.omitNull())
    return false;
  if (not obj.isModified() and data->cth.modOnly())
    return false;
  if (inArray() and arrayIndex() > 0)
    return false;
  std::string name = obj.getName(data->cth);
  if (not name.empty())
    name += ".";
  data->names.push(data->names.top() + name);
  return true;
}

void ElementNames::doObjEnd(const ObjectBase &obj) {
  data->names.pop();
}

bool ElementNames::doArrayBeg(const MemBaseVector &vec) {
  if (vec.hasFeature(mobs::DbDetail))
    return false;
  if (vec.isNull() and data->cth.omitNull())
    return false;
  if (not vec.isModified() and data->cth.modOnly())
    return false;
  data->names.push(data->names.top() + vec.getName(data->cth) + ".");
  return true;
}

void ElementNames::doArrayEnd(const MemBaseVector &vec) {
  data->names.pop();
}

void ElementNames::doMem(const MemberBase &mem) {
  if (data->sort) {
    int dir;
    u_int pos;
    if (inArray() and arrayIndex() > 0)
      return;
    if (data->sort->sortInfo(mem, pos, dir))
      data->selectOrder[pos] = make_pair(data->names.top() + mem.getName(data->cth), dir);
    return;
  } else if (data->lookUp) {
    if (inArray() and arrayIndex() > 0)
      return;
    auto ql = data->lookUp->find(&mem);
    if (ql != data->lookUp->end()) {
      ql->second = data->names.top();
      ql->second += mem.getName(data->cth);
    }
    return;
  }
  if (mem.isNull() and data->cth.omitNull())
    return;
  if (not mem.isModified() and data->cth.modOnly())
    return;
  if (inArray() and arrayIndex() > 0)
    return;
  bool compact = data->cth.compact();
  if (mem.is_chartype(data->cth) and mem.hasFeature(mobs::DbCompact))
    compact = true;

  valueStmt(data->names.top() + mem.getName(data->cth), mem, compact);
}

void ElementNames::startOrder(const QueryOrder &s) {
  data->sort = &s;
}

void ElementNames::finishOrder() {
  for (auto const &o:data->selectOrder) {
    orderStmt(o.second.first, o.second.second);
  }

}

void ElementNames::startLookup(map<const MemberBase *, std::string> &lookUp) {
  data->lookUp = &lookUp;
}






//////////////////////////////// AuditTrail ////////////////////////////////

AuditTrail::AuditTrail(AuditActivity &at) : act(at), cth(ConvObjToString().exportAltNames()) { names.push(""); m_auditMode = true; }

bool AuditTrail::s_saveInitialValues = false;

void AuditTrail::destroyObj() { destroyMode = true; initial = true; }

bool AuditTrail::doObjBeg(const ObjectBase &obj) {
  if (key.empty()) { // Neues Objekt
    int64_t version;
    std::string objKey = obj.keyStr(&version);
    act.objects[MemBaseVector::nextpos].objectKey(objKey);
    MemVarCfg c = obj.hasFeature(ColNameBase);
    act.objects.back().objectName(c ? obj.getConf(c) : obj.getObjectName());
    if (version < 0)
      act.objects.back().initialVersion.forceNull();
    else
      act.objects.back().initialVersion(static_cast<const int &>(version));
    if (destroyMode)
      act.objects.back().destroy(true);
    else if (version == 0) {
      initial = true;
      if (not s_saveInitialValues)
        return false;
    }
  }
  else if (obj.hasFeature(mobs::DbDetail))
    return false;
  if (obj.isNull() and cth.omitNull())
    return false;
  if (not inDelAudit() and not initial and not obj.isModified())
    return false;
  std::string name = names.top();
  if (inArray()) {
    name += u8"[";
    name += to_string(arrayIndex());
    name += "]";
  }
  else {
    if (not name.empty())
      name += ".";
    name += obj.getName(cth);
  }
  names.push(name);
  key.push(key.empty() or (key.top() and obj.keyElement()));
  return true;
}

void AuditTrail::doObjEnd(const ObjectBase &obj) {
  names.pop();
  key.pop();
}

bool AuditTrail::doArrayBeg(const MemBaseVector &vec) {
  if (vec.hasFeature(mobs::DbDetail))
    return false;
  if (vec.isNull() and cth.omitNull())
    return false;
  if (not inDelAudit() and not initial and not vec.isModified())
    return false;
  std::string name = names.top();
  if (not name.empty())
    name += ".";
  name += vec.getName(cth);
  names.push(name);
  key.push(false);
  return true;
}

void AuditTrail::doArrayEnd(const MemBaseVector &vec) {
  size_t old = vec.getInitialSize();
  if (old != vec.size()) {
    act.objects.back().changes[MemBaseVector::nextpos].field(names.top());
    string val;
    bool null = false;
    if (initial or inDelAudit())
      val = std::to_string(vec.size());
    else
      val = std::to_string(old);
    
    act.objects.back().changes.back().value(val);
    act.objects.back().changes.back().nullVal(null);
  }
  names.pop();
  key.pop();
}

void AuditTrail::doMem(const MemberBase &mem) {
  if (mem.isNull() and cth.omitNull())
    return;
  if (not inDelAudit() and not initial and not mem.isModified())
    return;
  if ((mem.keyElement() or mem.isVersionField()) and key.top())
    return;
  std::string name = names.top();
  if (inArray()) {
    name += u8"[";
    name += to_string(arrayIndex());
    name += "]";
  }
  else {
    if (not name.empty())
      name += ".";
    name += mem.getName(cth);
  }
  string val;
  bool null = false;
  if (initial or inDelAudit()) {
    null = mem.isNull();
    if (not null) {
      val = mem.auditValue();
      if (val == mem.auditEmpty() and not mem.hasFeature(mobs::InitialNull))
        return;
    }
    else if (mem.hasFeature(mobs::InitialNull))
      return;
  } else {
    mem.getInitialValue(val, null);
    if (null and mem.isNull())
      return;
    if (not null and not mem.isNull() and val == mem.auditValue())
      return;
  }

  do {
    string v;
    if (maxValSize > 0 and val.length() > maxValSize) {
      v = val.substr(0, maxValSize -1);
      v += '\\';  // Marker am Zeilenende, wenn Folgezeile kommt, um trailing spaces zu kapseln
      val.erase(0, maxValSize -1);
    } else
      v.swap(val);
    act.objects.back().changes[MemBaseVector::nextpos].field(name);
    act.objects.back().changes.back().value(v);
    act.objects.back().changes.back().nullVal(null);
  } while (not val.empty());
}


std::string convLikeToRegexp(const string &like) {
  std::string result;
  bool esc = false;
  bool first = true;
  std::string append;
  for (auto c:like) {
    if (first and c != '%')
      result = "^";
    result += append;
    append = "";
    switch (c) {
      case '\\':
        esc = true;
        break;
      case '%':
        if (esc) {
          result += c;
          esc = false;
        } else if (not first)
          append = ".*";
        break;
      case '_':
        if (esc)
          result += c;
        else
          result += ".";
        esc = false;
        break;
      case '.':
      case '*':
      case '^':
      case '$':
        result += '\\';
        // fall into
      default:
        result += c;
    }
    first = false;
  }
  if (append.empty())
    result += '$';
  return result;
}


class XsdDump : virtual public ObjTravConst {
public:
  explicit XsdDump(const ConvObjToString &c, const string &nsd) : cth(c), ns(to_wstring(nsd)) { };
  bool doObjBeg(const ObjectBase &obj) override
  {
    if (attribute) {
      if (level > attribute)
        return false;
      level++;
      if (level < attribute+1)
        return true;
      level--;
      return false;
    }
    if (sequence) {
      if (level > sequence)
        return false;
      level++;
      if (level < sequence+1)
        return true;

      xw.writeTagBegin(L"element");
      string n = obj.getObjectName();
      xw.writeAttribute(L"type", L"urn:" + to_wstring(n + "Type"));
      xw.writeAttribute(L"name", to_wstring(obj.getName(cth)));
      xw.writeAttribute(L"xmlns:urn", ns);
      xw.writeTagEnd();
      level--;
      return false;
    }
    if (level == 0) {
      xw.writeHead();
      xw.setPrefix(L"xs:");
      xw.writeTagBegin(L"schema");
      xw.writeAttribute(L"attributeFormDefault", L"unqualified");
      xw.writeAttribute(L"elementFormDefault", L"qualified");
      xw.writeAttribute(L"targetNamespace", ns);
      xw.writeAttribute(L"xmlns:xs", L"http://www.w3.org/2001/XMLSchema");
    }
    level++;
    return true;
  };
  void doObjEnd(const ObjectBase &obj) override
  {
    level--;
    if (sequence or attribute)
      return;
    xw.writeTagBegin(L"complexType");
    string n = obj.getObjectName();
    xw.writeAttribute(L"name", to_wstring(n + "Type"));
    xw.writeTagBegin(L"sequence");
    sequence = level+1;
    obj.traverse(*this);
    inarray = false;
    sequence = 0;
    xw.writeTagEnd();
    attribute = level+1;
    obj.traverse(*this);
    attribute = 0;
    xw.writeTagEnd();

    if (level == 0) {
      //      xw.writeTagBegin(L"complexType");
//      xw.writeAttribute(L"type", L"rootType");
//      xw.writeAttribute(L"xmlns:urn", ns);
//      xw.writeTagEnd();
      xw.writeTagEnd(true);
    }
  };
  bool doArrayBeg(const MemBaseVector &vec) override
  {
    if (arrayIndex() > vec.size()) // traverseElement
      return true;
    if (attribute)
      return false;
    if (sequence) {
      xw.writeTagBegin(L"element");
      string n = vec.contentObjName();
      xw.writeAttribute(L"type", L"urn:" + to_wstring(n + "Type"));
      xw.writeAttribute(L"name", to_wstring(vec.getName(cth)));
      xw.writeAttribute(L"maxOccurs", L"unbounded");
      xw.writeAttribute(L"minOccurs", L"0");
      xw.writeAttribute(L"xmlns:urn", ns);
      xw.writeTagEnd();
      return false;
    }

    fname = to_wstring(vec.getName(cth));
    inarray = true;
    vec.traverseElement(*this);
    inarray = false;
    return false;
  };
  void doArrayEnd(const MemBaseVector &vec) override
  {
  };
  void doMem(const MemberBase &mem) override
  {
    bool useSimlpeType = false;
    size_t sLen = 0;
    bool compact = cth.compact();
    if (mem.is_chartype(cth) and mem.hasFeature(mobs::DbCompact))
      compact = true;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    mi.changeCompact(compact);
    wstring type;
//    if (mi.hasCompact)
//      xw.writeComment(L"COMP", true);
    if (mi.isEnum) {
      type = L"xs:NMTOKEN";
      useSimlpeType = true;
    }
    else if (mi.isUnsigned and mi.max == 1)
      type = L"xs:bool";
    else if (mi.isSigned or mi.isUnsigned)
      type = L"xs:integer";
    else if (mi.isTime)
      type = mi.granularity >= 86400000000 ? L"xs:date" : L"xs:time";
    else if (mi.isFloat)
      type = L"xs:float";
    else
    {
      type = L"xs:string";
      MemVarCfg c = mem.hasFeature(LengthBase);
      if (c) {
        sLen = (c - LengthBase);
        useSimlpeType = true;
      }
    }

    if (sequence and not mem.hasFeature(MemVarCfg::XmlAsAttr)) {
      xw.writeTagBegin(L"element");
      if (not useSimlpeType)
        xw.writeAttribute(L"type", type);
      xw.writeAttribute(L"name", to_wstring(mem.getName(cth)));
      if (inarray) {
        xw.writeAttribute(L"maxOccurs", L"unbounded");
        xw.writeAttribute(L"minOccurs", L"0");
      }
      if (useSimlpeType) {
        xw.writeTagBegin(L"simpleType");
        xw.writeTagBegin(L"restriction");
        xw.writeAttribute(L"base", type);
        if (mi.isEnum) {
          for (int i = 0; i <= mi.max; i++) {
            auto s = mi.eToStr(i);
            xw.writeTagBegin(L"enumeration");
            xw.writeAttribute(L"value", to_wstring(s));
            xw.writeTagEnd();
          }
        }
        else if (sLen > 0) {
          xw.writeTagBegin(L"maxLength");
          xw.writeAttribute(L"value", to_wstring(sLen));
          xw.writeTagEnd();
        }
        xw.writeTagEnd();
        xw.writeTagEnd();
      }
      xw.writeTagEnd();
    }
    if (attribute and mem.hasFeature(MemVarCfg::XmlAsAttr)) {
      xw.writeTagBegin(L"attribute");
      if (not useSimlpeType)
        xw.writeAttribute(L"type", type);
      xw.writeAttribute(L"name", to_wstring(mem.getName(cth)));
      if (useSimlpeType) {
        xw.writeTagBegin(L"simpleType");
        xw.writeTagBegin(L"restriction");
        xw.writeAttribute(L"base", type);
        if (mi.isEnum) {
          for (int i = 0; i <= mi.max; i++) {
            auto s = mi.eToStr(i);
            xw.writeTagBegin(L"enumeration");
            xw.writeAttribute(L"value", to_wstring(s));
            xw.writeTagEnd();
          }
        }
        else if (sLen > 0) {
          xw.writeTagBegin(L"maxLength");
          xw.writeAttribute(L"value", to_wstring(sLen));
          xw.writeTagEnd();
        }
        xw.writeTagEnd();
        xw.writeTagEnd();
      }
      xw.writeTagEnd();
    }
    if (mem.isNull() and cth.omitNull())
      return;

  };
  std::string result() { return xw.getString(); };
private:
  int level = 0;
  ConvObjToString cth;

  wstring ns;
  mobs::XmlWriter xw;
  wstring fname;
  bool inarray = false;
  int sequence = 0;
  int attribute = 0;
};

std::string generateXsd(const ObjectBase &obj, const std::string &nameSpace) {
  try {
    ConvObjToString c;
    XsdDump xd(c, nameSpace);
    obj.traverse(xd);
    return xd.result();
  } catch (exception &e)
  {
    LOG(LM_ERROR, "XSD error " << e.what());
    return {};
  }
}


}


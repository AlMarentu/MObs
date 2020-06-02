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

/** \file helper.h
 \brief  Hilfsfunktionen für Objekt Traversierung */

#ifndef MOBS_HELPER_H
#define MOBS_HELPER_H

#include "objgen.h"
#include <sstream>
#include <set>

namespace mobs {

class ElementNames : virtual public ObjTravConst {
public:
  explicit ElementNames(ConvObjToString c) : cth(std::move(c)) { names.push(""); };

  virtual void valueStmt(const std::string &name, const MemberBase &mem, bool compact) = 0;

  /// \private
  bool doObjBeg(const ObjectBase &obj) final
  {
    if (obj.hasFeature(mobs::DbDetail))
      return false;
    if (obj.isNull() and cth.omitNull())
      return false;
    if (not obj.isModified() and cth.modOnly())
      return false;
    if (inArray() and arrayIndex() > 0)
      return false;
    std::string name = obj.getName(cth);
    if (not name.empty())
      name += ".";
    names.push(names.top() + name);
    return true;
  };
  /// \private
  void doObjEnd(const ObjectBase &obj) final
  {
    names.pop();
  };
  /// \private
  bool doArrayBeg(const MemBaseVector &vec) final
  {
    if (vec.hasFeature(mobs::DbDetail))
      return false;
    if (vec.isNull() and cth.omitNull())
      return false;
    if (not vec.isModified() and cth.modOnly())
      return false;
    names.push(names.top() + vec.getName(cth) + ".");
    return true;
  };
  /// \private
  void doArrayEnd(const MemBaseVector &vec) final
  {
    names.pop();
  };
  /// \private
  void doMem(const MemberBase &mem) final
  {
    if (mem.isNull() and cth.omitNull())
      return;
    if (not mem.isModified() and cth.modOnly())
      return;
    if (inArray() and arrayIndex() > 0)
      return;
    bool compact = cth.compact();
    if (mem.is_chartype(cth) and mem.hasFeature(mobs::DbCompact))
      compact = true;

    valueStmt(names.top() + mem.getName(cth), mem, compact);
  };


protected:

private:
  ConvObjToString cth;
  std::stack<std::string> names{};
};








/// Basisklasse zur Ablage von Objekten in Tabellenstruktur
class GenerateSql : virtual public ObjTravConst {
public:
  enum Mode { Fields, Keys, Query, Values, Update, Create, Join, UpdateValues, UpdateFields };
  explicit GenerateSql(Mode m, ConvObjToString c) : mode(m), cth(std::move(c)) {
    if (mode == Create)
      arrayStructureMode = true;
  };

  virtual std::string createStmt(const MemberBase &mem, bool compact) {
    std::stringstream res;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    double d;
    if (mi.isTime and mi.granularity >= 86400000)
      res << "DATE";
    else if (mi.isTime)
      res << "DATETIME";
    else if (mem.toDouble(d))
      res << "FLOAT";
    else if (mem.is_chartype(mobs::ConvToStrHint(compact)))
      res << "VARCHAR";
    else if (mi.isSigned and mi.max <= 32767)
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
  virtual std::string valueStmt(const MemberBase &mem, bool compact) {
    if (mem.isNull()) {
      if (mode == Query)
        return u8" is null";
      if (mode != Values and mode != UpdateValues)
        return u8"=null";
      return u8"null";
    }
    std::string pf;
    if (mode != Values and mode != UpdateValues)
      pf = "=";
    if (mem.is_chartype(mobs::ConvToStrHint(compact)))
      return pf + mobs::to_squote(mem.toStr(mobs::ConvToStrHint(compact)));

     return pf + mem.toStr(mobs::ConvToStrHint(compact));
  }

  
  /// \private
  bool doObjBeg(const ObjectBase &obj) final
  {
    if (obj.hasFeature(mobs::DbDetail))
      return false;
    if (obj.isNull() and cth.omitNull())
      return false;
    if (not obj.isModified() and cth.modOnly())
      return false;
    if (inArray() and arrayIndex() > 0)
      return false;
    level++;
    return true;
  };
  /// \private
  void doObjEnd(const ObjectBase &obj) final
  {
    if (obj.isNull() and cth.omitNull())
      return;
    level--;
    fst = false;
  };
  /// \private
  bool doArrayBeg(const MemBaseVector &vec) final
  {
    if (&vec == startVec) {
      if (mode == Query or mode == Join)
        return true;
      if (not fst)
        res << ",";
      fst = false;
      if (mode == Create) {
        std::string name = "a_idx";
        if (dupl.find(name) != dupl.end())
          throw std::runtime_error(name + u8" is a duplicate id in SQL statement");
        dupl.insert(name);
        res << name << " INT NOT NULL"; // createStmt(mem, compact);
        return true;
      }
      if (mode != Values and mode != UpdateValues)
        res << keyPrefix << "a_idx";
      if (mode == Fields or mode == Keys or mode == UpdateFields)
        return true;

      res << arrayIndex();
      return true;
    }
//    if (vec.hasFeature(mobs::DbDetail)) {
//      detailVec.push_back(&vec);
//      return false;
//    }
//    LOG(LM_DEBUG, "DBDETAIL implizit " << vec.getName(cth));
    detailVec.push_back(&vec);
    return false;

    if (vec.isNull() and cth.omitNull())
      return false;
    if (not vec.isModified() and cth.modOnly())
      return false;
    if (not fst)
      res << ",";
    fst = false;
    res << keyPrefix << vec.getName(cth) << ":";
    if (vec.isNull())
    {
      res << "null";
      fst = false;
      return false;
    }
    return true;
  };
  /// \private
  void doArrayEnd(const MemBaseVector &vec) final
  {
    fst = false;
  };
  /// \private
  void doMem(const MemberBase &mem) final
  {
    if (mem.isNull() and cth.omitNull())
      return;
    if (not mem.isModified() and cth.modOnly())
      return;
    if ((mode == Update or mode == UpdateValues or mode == UpdateFields) and mem.key() > 0)
      return;
    if (mode == Keys and mem.key() == 0)
      return;
    bool compact = cth.compact();
    if (mem.is_chartype(cth) and mem.hasFeature(mobs::DbCompact))
      compact = true;

    if (mode == Join) {
      if (not fst)
        res << " and ";
      fst = false;
      std::string name = mem.getName(cth);
      res << "mt." << name << "=" << keyPrefix << name;
      return;
    }
    if (not fst)
      res << ",";
    fst = false;
    if (mode == Create) {
      std::string name = mem.getName(cth);
      if (dupl.find(name) != dupl.end())
        throw std::runtime_error(name + u8" is a duplicate id in SQL statement");
      dupl.insert(name);
      res << name << " " << createStmt(mem, compact);
      return;
    }

    if (mode != Values and mode != UpdateValues)
      res << keyPrefix << mem.getName(cth);
    if (mode == Fields or mode == Keys or mode == UpdateFields)
      return;

    res << valueStmt(mem, compact);
  };
  std::string result() { return res.str(); };

  std::string keyPrefix; // = "mt."; ///< für master table
  const MemBaseVector *startVec = nullptr;
  std::list<const MemBaseVector *> detailVec;

protected:
  enum Mode mode;

private:
  ConvObjToString cth;
  std::set<std::string> dupl;
  bool fst = true;
  int level = 0;
  std::stringstream res;
};




/// Basisklasse zum Einlesen von Objekten aus einer Tabellenstruktur
/// \see GenerateSql
class ExtractSql : virtual public ObjTrav {
public:
  enum Mode { Fields, Keys };
  explicit ExtractSql(Mode m, ConvObjToString c) : mode(m), cth(std::move(c)) { };

  /// Einlesen der Elemente, die Sequenz entspricht der Abfrage aus GenerateSql
  virtual void readValue(MemberBase &mem, bool compact) = 0;


  /// \private
  bool doObjBeg(ObjectBase &obj) final
  {
    if (obj.hasFeature(mobs::DbDetail))
      return false;
    if (obj.isNull() and cth.omitNull())
      return false;
    if (not obj.isModified() and cth.modOnly())
      return false;
    level++;
    return true;
  };
  /// \private
  void doObjEnd(ObjectBase &obj) final
  {
    if (obj.isNull() and cth.omitNull())
      return;
    level--;
    fst = false;
  };
  /// \private
  bool doArrayBeg(MemBaseVector &vec) final
  {
    detailVec.push_back(&vec);
    return false;

//    if (vec.hasFeature(mobs::DbDetail))
//      return false;
//    if (vec.isNull() and cth.omitNull())
//      return false;
//    if (not vec.isModified() and cth.modOnly())
//      return false;
//    if (vec.isNull())
//      return false;
    return false;
  };
  /// \private
  void doArrayEnd(MemBaseVector &vec) final
  {
  };
  /// \private
  void doMem(MemberBase &mem) final
  {
    if (mem.isNull() and cth.omitNull())
      return;
    if (not mem.isModified() and cth.modOnly())
      return;
    if (mode == Keys and mem.key() == 0)
      return;
//    if (not inArray())
    bool compact = cth.compact();
    if (mem.is_chartype(cth) and mem.hasFeature(mobs::DbCompact))
      compact = true;

    readValue(mem, compact);
  };

  std::string keyPrefix; // = "mt."; ///< für master table
  std::list<MemBaseVector *> detailVec;

protected:
  enum Mode mode;
  ConvObjToString cth;

private:
  bool fst = true;
  int level = 0;
};




class FindDetailTables : virtual public ObjTrav {
public:
  enum Mode { Load, Save };
  explicit FindDetailTables(Mode m, ConvObjToString c) : mode(m), cth(std::move(c)) { };

  std::list<mobs::ObjectBase *> &objectList() { return objects; }
  std::list<mobs::MemBaseVector *> &vectorList() { return vectors; }


  /// \private
  bool doObjBeg(ObjectBase &obj) final
  {
    if (obj.isNull() and cth.omitNull())
      return false;
    if (not obj.isModified() and cth.modOnly())
      return false;
    if (obj.hasFeature(mobs::DbDetail))
      objects.push_back(&obj);
    level++;
    return true;
  };
  /// \private
  void doObjEnd(ObjectBase &obj) final
  {
    level--;
  };
  /// \private
  bool doArrayBeg(MemBaseVector &vec) final
  {
    if (vec.isNull() and cth.omitNull())
      return false;
    if (not vec.isModified() and cth.modOnly())
      return false;
    if (vec.hasFeature(mobs::DbDetail))
      vectors.push_back(&vec);
    level++;
    return true;
  };
  /// \private
  void doArrayEnd(MemBaseVector &vec) final
  {
    level--;
  };
  /// \private
  void doMem(MemberBase &mem) final
  {
  };


protected:
  Mode mode;
  ConvObjToString cth;
  std::list<mobs::ObjectBase *> objects{};
  std::list<mobs::MemBaseVector *> vectors{};

private:
  int level = 0;
};






}

#endif //MOBS_HELPER_H

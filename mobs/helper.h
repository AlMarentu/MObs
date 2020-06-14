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
 \brief  Hilfsfunktionen für interne Datenbank-Module */

#ifndef MOBS_HELPER_H
#define MOBS_HELPER_H

#include "objgen.h"
#include <sstream>
#include <set>

namespace mobs {

/// Klasse die Datenbank-Typ abhängige Definitionen enthält
class SQLDBdescription {
public:

  virtual std::string tableName(const std::string &tabnam) = 0;

  virtual std::string valueStmtIndex(size_t i) = 0;

  virtual std::string createStmtIndex(std::string name) = 0;

  virtual std::string createStmt(const MemberBase &mem, bool compact) = 0;

  virtual std::string valueStmt(const MemberBase &mem, bool compact, bool increment, bool inWhere) = 0;

  /// Einlesen der Elemente, die Sequenz entspricht der Abfrage aus GenerateSql
  virtual void readValue(MemberBase &mem, bool compact) = 0;

  virtual size_t readIndexValue() = 0;
  virtual void startReading() {};
  virtual void finishReading() {};

  /// in einer Where-Bedingung wird bei Null-Werten ein "is" vorangestellt, statt "=", kann is \c valueStmt angepasst werden
  bool changeTo_is_IfNull = true;
};

/// Generator-Klasse für SQL-Statements für Leesen und Schreiben, benötigt ein SQL-Beschreibungs-Objeht \c SQLDBdescription
class SqlGenerator {
public:
  /// Handle für selectStatementArray - readObject-Paare
  class DetailInfo {
  public:
    DetailInfo(const mobs::MemBaseVector *v, std::string t, std::vector<std::pair<std::string, size_t>> k, bool c);
    DetailInfo(mobs::MemBaseVector *v, std::string t, std::vector<std::pair<std::string, size_t>> k);
    DetailInfo();

    /// \private
    const mobs::MemBaseVector *vec;
    /// \private
    mobs::MemBaseVector *vecNc; // für Schreibzugriff
    /// \private
    std::string tableName;
    /// \private
    std::vector<std::pair<std::string, size_t>> arrayKeys;
    /// \private
    bool cleaning;
  };
  SqlGenerator(const mobs::ObjectBase &object, SQLDBdescription &sqldBdescription) : obj(object),
                                                                                     sqldb(sqldBdescription) {}
  /// Tabellenname (Master) ohne DB-Spezifische Erweiterung
  std::string tableName() const;

  enum QueryMode { Normal, Keys, Count };
  std::string queryBE(QueryMode querMode = Normal, const std::string &where = "");
  std::string query(QueryMode querMode, const std::string &where, const std::string &join = "");

  std::string createStatement(bool first);
  std::string dropStatement(bool first);
  std::string insertStatement(bool first);
  std::string replaceStatement(bool first);
  std::string updateStatement(bool first);
  std::string deleteStatement(bool first);

  std::string selectStatementFirst(bool keys = false);
  void readObject(mobs::ObjectBase &obj);

  std::string selectStatementArray(DetailInfo &di);
  void readObject(const DetailInfo &di);

  /// ermittle die Version des Objektes
  uint64_t getVersion() const;

  /// es liegen keine Sub-Statements mehr an
  bool eof() { return detailVec.empty(); }
  /// hatte letztes Query-Statement einenJoin ?
  bool queryWithJoin() const { return querywJoin; }

private:
  std::string doCreate(DetailInfo &);
  std::string doDrop(DetailInfo &);
  std::string doDelete(DetailInfo &);
  std::string doUpdate(DetailInfo &);
  std::string doInsert(DetailInfo &, bool replace);
  std::string doSelect(DetailInfo &);

  const mobs::ObjectBase &obj;
  SQLDBdescription &sqldb;
  std::list<DetailInfo> detailVec{};
  int64_t version = -1;
  bool querywJoin = false;
};


/// Ermittle Elementnamen in mit kompletten Pfad zB.: a.b.c
class ElementNames : virtual public ObjTravConst {
public:
  explicit ElementNames(ConvObjToString c);

  virtual void valueStmt(const std::string &name, const MemberBase &mem, bool compact) = 0;

  /// \private
  bool doObjBeg(const ObjectBase &obj) final;
  /// \private
  void doObjEnd(const ObjectBase &obj) final;
  /// \private
  bool doArrayBeg(const MemBaseVector &vec) final;
  /// \private
  void doArrayEnd(const MemBaseVector &vec) final;
  /// \private
  void doMem(const MemberBase &mem) final;

private:
  ConvObjToString cth;
  std::stack<std::string> names{};
};


}

#endif //MOBS_HELPER_H

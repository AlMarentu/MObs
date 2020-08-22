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
#include "mchrono.h"
#include <sstream>
#include <set>

namespace mobs {

/// Klasse die Datenbank-Typ abhängige Definitionen enthält
class SQLDBdescription {
public:

  /// Aufbau des Tabellennamen
  virtual std::string tableName(const std::string &tabnam) = 0;

  /// Erzeuge SQL-Fragment für einen Index-Wert
  virtual std::string valueStmtIndex(size_t i) = 0;

  /// Erzeuge SQL-Fragment für einen Index Eintrag
  virtual std::string createStmtIndex(std::string name) = 0;

  /// Erzeuge SQL-Fragment für einen Index-Wert
  virtual std::string valueStmtText(const std::string &t, bool isNull) = 0;

  /// Erzeuge SQL-Fragment für einen DBJSON Eintrag
  virtual std::string createStmtText(const std::string &name, size_t len) = 0;

  /// Erzeuge SQL-Create-Statement
  virtual std::string createStmt(const MemberBase &mem, bool compact) = 0;

  /// Erzeuge SQL-Fragment für einen Wert Eintrag
  virtual std::string valueStmt(const MemberBase &mem, bool compact, bool increment, bool inWhere) = 0;

  /// Einlesen der Elemente, die Sequenz entspricht der Abfrage aus GenerateSql
  virtual void readValue(MemberBase &mem, bool compact) = 0;

  /// Einlesen der DBJSON Elemente, die Sequenz entspricht der Abfrage aus GenerateSql
  virtual void readValueText(const std::string &name, std::string &text, bool &null) = 0;

  /// Einlesen der Index-Elemente, die Sequenz entspricht der Abfrage aus GenerateSql
  virtual size_t readIndexValue(const std::string &name) = 0;


  /// Callback zum Start des Einlesens
  virtual void startReading() {};
  /// Callback zum Ende des Einlesens
  virtual void finishReading() {};

  /// in einer Where-Bedingung wird bei Null-Werten ein "is" vorangestellt, statt "=", kann is \c valueStmt angepasst werden
  bool changeTo_is_IfNull = true;
  /// Erzeuge create if not exists - Statement
  bool createWith_IfNotExists = false;
};

/// Generator-Klasse für SQL-Statements für Lesen und Schreiben, benötigt ein SQL-Beschreibungs-Objekt \c SQLDBdescription
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
  ~SqlGenerator();
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
  std::string insertUpdStatement(bool first, std::string &upd);
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
  /// löscht temporäre Objekte im Destruktor
  void deleteLater(ObjectBase *o) { m_deleteLater.push_back(o); }

private:
  std::string doCreate(DetailInfo &);
  std::string doDrop(DetailInfo &);
  std::string doDelete(DetailInfo &);
  std::string doUpdate(DetailInfo &);
  std::string doInsertUpd(DetailInfo &di, std::string &upd);

  std::string doInsert(DetailInfo &, bool replace);
  std::string doSelect(DetailInfo &);

  const mobs::ObjectBase &obj;
  SQLDBdescription &sqldb;
  std::list<DetailInfo> detailVec{};
  bool querywJoin = false;
  std::list<ObjectBase *> m_deleteLater;
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

/// Traversier-Klasse: Setzt alle Vektoren eines Objektes auf Größe 1
class SetArrayStructure  : virtual public mobs::ObjTrav {
public:
  /// \private
  bool doObjBeg(mobs::ObjectBase &obj) final { return true; }
  /// \private
  void doObjEnd(mobs::ObjectBase &obj) final {  }
  /// \private
  bool doArrayBeg(mobs::MemBaseVector &vec) final { vec.resize(1); return true; }
  /// \private
  void doArrayEnd(mobs::MemBaseVector &vec) final { }
  /// \private
  void doMem(mobs::MemberBase &mem) final { }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////   Audit Trail   ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Datenbank-Objekt für Audit-Trail
class AuditChanges : public mobs::ObjectBase {
public:
  ObjInit(AuditChanges);
  MemVar(std::string, field, LENGTH(100));
  MemVar(std::string, value, LENGTH(200));
  MemVar(bool,        nullVal);

};

/// Datenbank-Objekt für Audit-Trail
class AuditObjects : public mobs::ObjectBase {
public:
  ObjInit(AuditObjects);
  
  MemVar(int, initialVersion); // wenn 0, dann Startwerte, sonst alte Werte
  MemVar(bool, destroy);  // wenn true, letzter Wert
  MemVar(std::string, objectName, LENGTH(20));
  MemVar(std::string, objectKey, LENGTH(80));
  MemVector(AuditChanges, changes, COLNAME(auditChanges));

};

/// Datenbank-Objekt für Audit-Trail
class AuditActivity : public mobs::ObjectBase {
public:
  ObjInit(AuditActivity);
  
  MemVar(MTime, time, KEYELEMENT1 DBCOMPACT);
  MemVar(int, userId, KEYELEMENT2);
  MemVar(std::string, comment, USENULL LENGTH(200));
  MemVector(AuditObjects, objects, COLNAME(auditObjects));
  
};


/// Ermittle Elementnamen in mit kompletten Pfad zB.: a.b.c
class AuditTrail : virtual public ObjTravConst {
public:
  explicit AuditTrail(AuditActivity &at);
  /// Objekt soll gelöscht werden
  void destroyObj();

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

  /// Wenn Startwerte nicht gespeichert werden sollen (sind eigentlich redundant) auf false setzen
  static bool s_saveInitialValues;
  
private:
  AuditActivity &act;
  ConvObjToString cth;
  std::stack<std::string> names{};
  std::stack<bool> key{};
  bool initial = false;
  bool destroyMode = false;
};


}

#endif //MOBS_HELPER_H

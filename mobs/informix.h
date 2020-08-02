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

/** \file informix.h
 \brief Datenbank-Interface für Zugriff auf IBM Informix

 IBM Informix is a registered trademark of IBM Corp.
 \see www.ibm.com
 */


#ifndef MOBS_INFORMIX_H
#define MOBS_INFORMIX_H


#include "dbifc.h"

namespace mobs {

class InformixDatabaseConnection : virtual public DatabaseConnection, public ConnectionInformation {
public:
  /// \private
  friend DatabaseManagerData;
  /// \private
  explicit InformixDatabaseConnection(const ConnectionInformation &connectionInformation) :
          DatabaseConnection(), ConnectionInformation(connectionInformation) { };
  ~InformixDatabaseConnection() override;

  /// \private
  void open();
  /// \private
  bool load(DatabaseInterface &dbi, ObjectBase &obj) override;
  /// \private
  void save(DatabaseInterface &dbi, const ObjectBase &obj) override;
  /// \private
  bool destroy(DatabaseInterface &dbi, const ObjectBase &obj) override;
  /// \private
  void dropAll(DatabaseInterface &dbi, const ObjectBase &obj) override;
  /// \private
  void structure(DatabaseInterface &dbi, const ObjectBase &obj) override;
  /// \private
  std::shared_ptr<DbCursor> query(DatabaseInterface &dbi, ObjectBase &obj, const std::string &query, bool qbe) override;
  /// \private
  void retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) override;
  /// \private
  void startTransaction(DatabaseInterface &dbi, DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) override;
  /// \private
  void endTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) override;
  /// \private
  void rollbackTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) override;
  // ------------------------------


  /// Ermittle den Collection-Namen zu einem Objekt
  static std::string tableName(const ObjectBase &obj, const DatabaseInterface &dbi) ;

  /// führe ein SQL-Statement auf der DB-Connection aus
  /// \return Anzahl betroffener Zeilen
  /// \throw mysql_exception
  size_t doSql(const std::string &sql);


private:
  int conNr = 0;
  DbTransaction * currentTransaction = nullptr;
};

}


#endif //MOBS_INFORMIX_H

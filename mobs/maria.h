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

/** \file maria.h
 \brief Datenbank-Interface für Zugriff auf MariaDB

 MariaDB is a registered trademarks of MariaDB.
 \see www.mariadb.com
 */


#ifndef MOBS_MARIA_H
#define MOBS_MARIA_H

#include "dbifc.h"

#include <mysql.h>


namespace mobs {

  /** \brief Datenbank-Verbindung zu einer MariaDB.
   *
   * MariaDB is a registered trademarks of MariaDB.
   * \see www.mariadb.com
   */
  class MariaDatabaseConnection : virtual public DatabaseConnection, public ConnectionInformation {
  public:
    /// \private
    friend DatabaseManagerData;
    /// \private
    explicit MariaDatabaseConnection(const ConnectionInformation &connectionInformation) :
            DatabaseConnection(), ConnectionInformation(connectionInformation) { };
    ~MariaDatabaseConnection() override;

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
    std::shared_ptr<DbCursor> query(DatabaseInterface &dbi, ObjectBase &obj, bool qbe, const QueryGenerator *query,
                                    const QueryOrder *sort) override;
    /// \private
    void retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) override;
    /// \private
    void startTransaction(DatabaseInterface &dbi, DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) override;
    /// \private
    void endTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) override;
    /// \private
    void rollbackTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) override;
    /// \private
    size_t maxAuditChangesValueSize(const DatabaseInterface &dbi) const override;
    // ------------------------------


    /// Ermittle den Collection-Namen zu einem Objekt
    static std::string tableName(const ObjectBase &obj, const DatabaseInterface &dbi) ;

    /// führe ein SQL-Statement auf der DB-Connection aus
    /// \return Anzahl betroffener Zeilen
    /// \throw mysql_exception
    size_t doSql(const std::string &sql);

    /// Direkt-Zugriff auf die MariaDB
    MYSQL *getConnection();

  private:
    MYSQL *connection = nullptr;
    DbTransaction * currentTransaction = nullptr;
  };
};

#endif //MOBS_MARIA_H

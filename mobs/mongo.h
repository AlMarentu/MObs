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

/** \file mongo.h
 \brief Datenbank-Interface für Zugriff auf MongoDB

 Mongo and MongoDB are registered trademarks of MongoDB, Inc.
 \see www.mongodb.com
 */

#ifndef MOBS_MONGO_H
#define MOBS_MONGO_H

#include <utility>

#include "dbifc.h"
#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>

namespace mobs {


  /** \brief Datenbank-Verbindung zu einer MongoDB.
   *
   * Mongo and MongoDB are registered trademarks of MongoDB, Inc.
   * \see www.mongodb.com
   */
  class MongoDatabaseConnection : virtual public DatabaseConnection, public ConnectionInformation {
  public:
    /// \private
    friend DatabaseManagerData;
    /// \private
    explicit MongoDatabaseConnection(const ConnectionInformation &connectionInformation);
    /// \private
    MongoDatabaseConnection(const MongoDatabaseConnection &) = delete;
    ~MongoDatabaseConnection() override = default;

    /// Typ der Datenbank
    std::string connectionType() const override { return u8"Mongo"; }

    /// \private
    void open();
    /// \private
    void close();
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

    /// \private
    void uploadFile(DatabaseInterface &dbi, const std::string &id, std::istream &source) override;
    /// \private
    std::string uploadFile(DatabaseInterface &dbi, std::istream &source) override;
    /// \private
    void downloadFile(DatabaseInterface &dbi, const std::string &id, std::ostream &dest) override;
    /// \private
    void deleteFile(DatabaseInterface &dbi, const std::string &id) override;

    // ------------------------------

    /// private
    void create(DatabaseInterface &dbi, const ObjectBase &obj);

    /// private
    static bool changedReadConcern(mongocxx::read_concern &rc, const DatabaseInterface &dbi);

    /// Ermittle den Collection-Namen zu einem Objekt
    static std::string collectionName(const ObjectBase &obj) ;

    /// Direkt-Zugriff auf die MongoDb
    mongocxx::database getDb(DatabaseInterface &dbi);

  private:
    class Entry {
    public:
      explicit Entry(mongocxx::pool &pool);
      ~Entry() = default;
      mongocxx::client &client() { return *entry; }
      mongocxx::pool::entry entry;
      static std::unique_ptr<mongocxx::instance> inst;
    };
    std::unique_ptr<Entry> entry;

    static std::map<std::string, mongocxx::pool> pools;
  };

}

#endif //MOBS_MONGO_H

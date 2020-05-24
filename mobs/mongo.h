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

namespace mobs {


  /** \brief Datenbank-Verbindung zu einer MongoDB.
   *
   * Mongo and MongoDB are registered trademarks of MongoDB, Inc.
   * \see www.mongodb.com
   */
  class mongoDatabaseConnection : virtual public DatabaseConnection, public ConnectionInformation {
  public:
    /// \private
    friend DatabaseManagerData;
    /// \private
    explicit mongoDatabaseConnection(const ConnectionInformation &connectionInformation) :
            DatabaseConnection(), ConnectionInformation(connectionInformation) { };
    ~mongoDatabaseConnection() override = default;

    /// \private
    void open();
    /// \private
    bool load(DatabaseInterface &dbi, ObjectBase &obj) override;
    /// \private
    void save(DatabaseInterface &dbi, const ObjectBase &obj) override;
    /// \private
    bool destroy(DatabaseInterface &dbi, const ObjectBase &obj) override;
    /// \private
    std::shared_ptr<DbCursor> query(DatabaseInterface &dbi, ObjectBase &obj, const std::string &query) override;
    /// \private
    std::shared_ptr<DbCursor> qbe(DatabaseInterface &dbi, ObjectBase &obj) override;
    /// \private
    void retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) override;

    // ------------------------------

    void createPrimaryKey(DatabaseInterface &dbi, ObjectBase &obj);

    void create(DatabaseInterface &dbi, const ObjectBase &obj);

    /// Ermittle den Collection-Namen zu einem Objekt
    static std::string collectionName(const ObjectBase &obj) ;

    /// Direkt-Zugriff auf die MongoDb
    mongocxx::database getDb(DatabaseInterface &dbi);

  private:
    mongocxx::client client;
    bool connected = false;

  };

}

#endif //MOBS_MONGO_H

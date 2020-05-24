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


#include "dbifc.h"

#include <utility>
#include "mongo.h"


namespace mobs {
namespace {

  class Database {
  public:
//    ConnectionInformation connectionInformation;
    std::shared_ptr<DatabaseConnection> connection;
    std::string database;
  };
}




  class DatabaseManagerData {
  public:
    void addConnection(const std::string &connectionName, const ConnectionInformation &connectionInformation);
    void copyConnection(const std::string &connectionName, const std::string &oldConnectionName, const std::string &database);

    DatabaseInterface getDbIfc(const std::string &connectionName);

    std::map<std::string, Database> connections;
  };

  void DatabaseManagerData::addConnection(const std::string &connectionName,
                                          const ConnectionInformation &connectionInformation) {
    // zuständiges Datenbankmodul ermitteln
    size_t pos = connectionInformation.m_url.find(':');
    if (pos == std::string::npos)
      throw std::runtime_error(u8"invalid URL");
    std::string db = connectionInformation.m_url.substr(0, pos);
    if (db == "mongodb")
    {
      Database &dbCon = connections[connectionName];
      auto dbi = std::make_shared<mongoDatabaseConnection>(connectionInformation);
      dbCon.database = connectionInformation.m_database;
      dbCon.connection = dbi;
    } else
      throw std::runtime_error(db + u8" is not a supported database");
  }

  void DatabaseManagerData::copyConnection(const std::string &connectionName, const std::string &oldConnectionName,
                                           const std::string &database) {
    auto i = connections.find(oldConnectionName);
    if (i == connections.end())
      throw std::runtime_error(oldConnectionName + u8" is not a valid connection");
    Database &dbCon = connections[connectionName];
    dbCon.connection = i->second.connection;
    dbCon.database = database;
  }

  DatabaseInterface DatabaseManagerData::getDbIfc(const std::string &connectionName) {
    auto i = connections.find(connectionName);
    if (i == connections.end())
      throw std::runtime_error(connectionName + u8" is not a valid connection");
    Database &dbCon = i->second;

    return DatabaseInterface(dbCon.connection, dbCon.database);
  }






  //////////////////////////////////////////////////////////////////////////////////////////////////////

  DatabaseManager::DatabaseManager() {
    if (manager)
      throw std::runtime_error(u8"DatabaseManager already exists");
    manager = this;
    data = new DatabaseManagerData;
  }

  DatabaseManager::~DatabaseManager() {
    delete data;
    manager = nullptr;
  }


  DatabaseManager *DatabaseManager::manager = nullptr;

  void DatabaseManager::addConnection(const std::string &connectionName,
                                      const ConnectionInformation &connectionInformation) {
    data->addConnection(connectionName, connectionInformation);
  }

  DatabaseInterface DatabaseManager::getDbIfc(const std::string &connectionName) {
    return data->getDbIfc(connectionName);
  }

  void DatabaseManager::copyConnection(const std::string &connectionName, const std::string &oldConnectionName,
                                       const std::string &database) {

  }


  //////////////////////////////////////////////////////////////////////////////////////////////////////

  DatabaseInterface::DatabaseInterface(std::shared_ptr<DatabaseConnection> dbi, std::string dbName)
          : dbCon(std::move(dbi)), databaseName(std::move(dbName)), timeout(0) {  }

  bool DatabaseInterface::load(ObjectBase &obj) {
    return dbCon->load(*this, obj);
  }

  void DatabaseInterface::save(const ObjectBase &obj) {
    dbCon->save(*this, obj);
  }

  bool DatabaseInterface::destroy(const ObjectBase &obj) {
    return dbCon->destroy(*this, obj);
  }

  std::shared_ptr<DbCursor> DatabaseInterface::query(ObjectBase &obj, const std::string &query) {
    return dbCon->query(*this, obj, query);
  }

  std::shared_ptr<DbCursor> DatabaseInterface::qbe(ObjectBase &obj) {
    return dbCon->qbe(*this, obj);
  }

  void DatabaseInterface::retrieve(ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) {
    dbCon->retrieve(*this, obj, cursor);
  }


}
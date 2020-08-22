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

#ifdef USE_MONGO
#include "mongo.h"
#endif
#ifdef USE_MARIA
#include "maria.h"
#endif
#ifdef USE_INFORMIX
#include "informix.h"
#endif

#include "helper.h"
#include "mchrono.h"
#include <unistd.h> // getuid

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
  std::string connectionName(std::shared_ptr<mobs::DatabaseConnection> dbCon, const std::string &dbName) const;

  std::map<std::string, Database> connections;
};

void DatabaseManagerData::addConnection(const std::string &connectionName,
                                        const ConnectionInformation &connectionInformation) {
  // zuständiges Datenbankmodul ermitteln
  size_t pos = connectionInformation.m_url.find(':');
  if (pos == std::string::npos)
    throw std::runtime_error(u8"invalid URL");
  std::string db = connectionInformation.m_url.substr(0, pos);
#ifdef USE_MONGO
  if (db == "mongodb")
  {
    Database &dbCon = connections[connectionName];
    auto dbi = std::make_shared<MongoDatabaseConnection>(connectionInformation);
    dbCon.database = connectionInformation.m_database;
    dbCon.connection = dbi;
    return;
  }
#endif
#ifdef USE_MARIA
  if (db == "mariadb")
  {
    Database &dbCon = connections[connectionName];
    auto dbi = std::make_shared<MariaDatabaseConnection>(connectionInformation);
    dbCon.database = connectionInformation.m_database;
    dbCon.connection = dbi;
    return;
  }
#endif
#ifdef USE_INFORMIX
  if (db == "informix")
  {
    Database &dbCon = connections[connectionName];
    auto dbi = std::make_shared<InformixDatabaseConnection>(connectionInformation);
    dbCon.database = connectionInformation.m_database;
    dbCon.connection = dbi;
    return;
  }
#endif
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

std::string DatabaseManagerData::connectionName(std::shared_ptr<mobs::DatabaseConnection> dbCon, const std::string &dbName) const {
  for (auto &i:connections) {
    if (i.second.connection == dbCon and i.second.database == dbName)
      return i.first;
  }
  return std::string();
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
  data->copyConnection(connectionName, oldConnectionName, database);
}


void DatabaseManager::execute(DatabaseManager::transaction_callback &cb) {
  DbTransaction transaction{};
  LOG(LM_DEBUG, "TRANSACTION STARTING " << to_string(transaction.startTime()));

  try {
    cb(&transaction);
    transaction.writeAuditTrail();
  } catch (std::runtime_error &e) {
    LOG(LM_DEBUG, "TRANSACTION FAILED runtime " << e.what());
    transaction.finish(false);
    throw std::runtime_error(std::string(u8"DbTransaction error: ") + e.what());
  } catch (std::exception &e) {
    LOG(LM_DEBUG, "TRANSACTION FAILED 2 " << e.what());
    transaction.finish(false);
    throw std::runtime_error(std::string(u8"DbTransaction error: ") + e.what());
  } catch (...) {
    LOG(LM_DEBUG, "TRANSACTION FAILED ...");
    transaction.finish(false);
    throw std::runtime_error(u8"DbTransaction error: unknown exception");
  }
  transaction.finish(true);
  DbTransaction::MTime end = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - transaction.startTime()).count();
  LOG(LM_DEBUG, "TRANSACTION FINISHED " << duration << " µs");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

class DbTransactionData {
public:
  class DTI {
  public:
    std::shared_ptr<TransactionDbInfo> tdb{};
    std::shared_ptr<DatabaseConnection> dbCon{};
    // je Database eigenen AuditTrail-Buffer
    std::map<std::string, AuditActivity> audit;
  };
  std::map<const DatabaseConnection *, DTI> connections;
  DbTransaction::IsolationLevel isolationLevel = DbTransaction::RepeatableRead;
  DbTransaction::MTime start = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());

  std::string comment;
  static int s_uid;
};

int DbTransactionData::s_uid = -1;

void DbTransaction::setUid(int i) {
  if (i >= 0)
    DbTransactionData::s_uid = i;
}

void DbTransaction::setComment(const std::string &comment) {
  data->comment = comment;
}

DbTransaction::MTime DbTransaction::startTime() const {
  return data->start;
}

void DbTransaction::doAuditSave(const ObjectBase &obj, const DatabaseInterface &dbi) {
  auto s = data->connections.find(&*dbi.getConnection());
  if (s == data->connections.end())
    return;
  
  AuditTrail at(s->second.audit[dbi.database()]);
  obj.traverse(at);
}

void DbTransaction::doAuditDestroy(const ObjectBase &obj, const DatabaseInterface &dbi) {
  auto s = data->connections.find(&*dbi.getConnection());
  if (s == data->connections.end())
    return;

  AuditTrail at(s->second.audit[dbi.database()]);
  at.destroyObj();
  obj.traverse(at);
}

void DbTransaction::writeAuditTrail() {
  for (auto &i:data->connections) {
    auto dti = i.second;
    for (auto a:dti.audit) {
      LOG(LM_DEBUG, "Writing audit");
      a.second.time(startTime());
      if (DbTransactionData::s_uid < 0)
        DbTransactionData::s_uid = getuid();
      a.second.userId(DbTransactionData::s_uid);
      if (not data->comment.empty())
        a.second.comment(data->comment);
      DatabaseInterface dbi_t(dti.dbCon, a.first);
      dbi_t.transaction = this;
      dti.dbCon->save(dbi_t, a.second);
    }
  }
}

DbTransaction::DbTransaction() : data(new DbTransactionData) { }

DbTransaction::~DbTransaction() {
  delete data;
}


DatabaseInterface DbTransaction::getDbIfc(DatabaseInterface &dbiIn) {
  DatabaseInterface dbi = DatabaseInterface(dbiIn.dbCon, dbiIn.databaseName);
 
  DbTransactionData::DTI &dti = data->connections[&*dbi.dbCon];
  if (not dti.dbCon)
    dti.dbCon = dbi.dbCon;
  dbi.dbCon->startTransaction(dbi, this, dti.tdb);
  dbi.transaction = this;
  return dbi;
}

DatabaseInterface DbTransaction::getDbIfc(const std::string &connectionName) {
  DatabaseManager *dbm = DatabaseManager::instance();
  if (not dbm)
    throw std::runtime_error("DatabaseManager invalid");
  DatabaseInterface dbi = dbm->getDbIfc(connectionName);

  DbTransactionData::DTI &dti = data->connections[&*dbi.dbCon];
  if (not dti.dbCon)
    dti.dbCon = dbi.dbCon;
  dbi.dbCon->startTransaction(dbi, this, dti.tdb);
  dbi.transaction = this;
  return dbi;
}

void DbTransaction::finish(bool good) {
  bool error = false;
  std::string msg;
  for (auto &i:data->connections) {
    auto dti = i.second;
    try {
      if (good)
        dti.dbCon->endTransaction(this, dti.tdb);
      else
        dti.dbCon->rollbackTransaction(this, dti.tdb);
    } catch (std::exception &e) {
      error = true;
      if (good) {
        LOG(LM_ERROR, "TRANSACTION FINISH FAILED " << e.what());
        msg += " ";
        msg += e.what();
      }
      else
        LOG(LM_DEBUG, "Transaction rollback " << e.what());
    } catch (...) {
      error = true;
      if (good) {
        LOG(LM_ERROR, "TRANSACTION FINISH FAILED Unknown exception");
        msg += " unknown exception";
      }
      else
        LOG(LM_DEBUG, "Transaction rollback unknown exception");
    }
  }
  if (error and good)
    throw std::runtime_error(std::string(u8"DbTransaction Commit error: ") + msg);
}

TransactionDbInfo *DbTransaction::transactionDbInfo(const DatabaseInterface &dbi) {
  auto it = data->connections.find(&*dbi.dbCon);
  if (it != data->connections.end())
    return it->second.tdb.get();
  else
    LOG(LM_ERROR, "TransactionDbInfo not found");
  return nullptr;
}

DbTransaction::IsolationLevel DbTransaction::getIsolation() const {
  return data->isolationLevel;
}

void DbTransaction::setIsolation(DbTransaction::IsolationLevel level) {
  data->isolationLevel = level;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
class ObjectSaved : virtual public ObjTrav {
public:
  bool doObjBeg(ObjectBase &obj) override {
    if (obj.hasFeature(mobs::DbDetail))
      return false;
    obj.setModified(false);
    return true;
  }

  void doObjEnd(ObjectBase &obj) override {}

  bool doArrayBeg(MemBaseVector &vec) override {
    if (vec.hasFeature(mobs::DbDetail))
      return false;
    vec.setModified(false);
    return true;
  }

  void doArrayEnd(MemBaseVector &vec) override {}

  void doMem(MemberBase &mem) override {
    if (mem.isVersionField()) {
      MobsMemberInfo mi;
      mem.memInfo(mi);
      if (mi.isUnsigned) {
        mi.u64++;
        if (not mem.fromMemInfo(mi))
          throw std::runtime_error(u8"VersionVariable can't assign");
      } else if (mi.isSigned) {
        mi.i64++;
        if (not mem.fromMemInfo(mi))
          throw std::runtime_error(u8"VersionVariable can't assign");
      } else
        throw std::runtime_error("VersionElement is not int");
    }

    mem.setModified(false);
  }
};
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

DatabaseInterface::DatabaseInterface(std::shared_ptr<DatabaseConnection> dbi, std::string dbName)
        : dbCon(std::move(dbi)), databaseName(std::move(dbName)), timeout(0) {  }

bool DatabaseInterface::load(ObjectBase &obj) {
  if (not dbCon->load(*this, obj))
    return false;
  if (obj.hasFeature(DbAuditTrail))
    obj.startAudit();
  return true;
}

void DatabaseInterface::save(const ObjectBase &obj) {
  if (transaction) {
    if (obj.hasFeature(DbAuditTrail))
      transaction->doAuditSave(obj, *this);
    dbCon->save(*this, obj);
    return;
  }
  if (not obj.hasFeature(DbAuditTrail)) {
    dbCon->save(*this, obj);
    return;
  }

  DatabaseManager::transaction_callback tcb = [this, &obj](mobs::DbTransaction *trans) {
    DatabaseInterface t_dbi = trans->getDbIfc(*this);
    trans->doAuditSave(obj, t_dbi);
    dbCon->save(t_dbi, obj);
  };
  DatabaseManager::execute(tcb);
}

void DatabaseInterface::save(ObjectBase &obj) {
  save((const ObjectBase &)obj);
  ObjectSaved os;
  obj.traverse(os);
}

bool DatabaseInterface::destroy(const ObjectBase &obj) {
  if (transaction) {
    if (obj.hasFeature(DbAuditTrail))
      transaction->doAuditDestroy(obj, *this);
    return dbCon->destroy(*this, obj);
  }
  if (not obj.hasFeature(DbAuditTrail))
    return dbCon->destroy(*this, obj);

  bool ok = true;
    DatabaseManager::transaction_callback tcb = [this, &ok, &obj](mobs::DbTransaction *trans) {
      DatabaseInterface t_dbi = trans->getDbIfc(*this);
      trans->doAuditDestroy(obj, t_dbi);
      ok = dbCon->destroy(t_dbi, obj);
    };
    DatabaseManager::execute(tcb);
  return ok;
}


std::shared_ptr<DbCursor> DatabaseInterface::query(ObjectBase &obj, const std::string &query) {
  return dbCon->query(*this, obj, query, false);
}

std::shared_ptr<DbCursor> DatabaseInterface::qbe(ObjectBase &obj) {
  return dbCon->query(*this, obj, "", true);
}

void DatabaseInterface::retrieve(ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) {
  if (not cursor->valid())
    throw std::runtime_error("DatabaseInterface: cursor is not valid");
  dbCon->retrieve(*this, obj, cursor);
  if (obj.hasFeature(DbAuditTrail))
    obj.startAudit();
}

void DatabaseInterface::dropAll(const ObjectBase &obj) {
  dbCon->dropAll(*this, obj);
}

void DatabaseInterface::structure(const ObjectBase &obj) {
  dbCon->structure(*this, obj);
  if (obj.hasFeature(DbAuditTrail)) {
    AuditActivity aa;
    dbCon->structure(*this, aa);
  }
}

std::string DatabaseInterface::connectionName() const {
  DatabaseManager *dbm = DatabaseManager::instance();
  if (not dbm)
    throw std::runtime_error("DatabaseManager invalid");

  return dbm->data->connectionName(dbCon, databaseName);
}

TransactionDbInfo *DatabaseInterface::transactionDbInfo() const {
  if (transaction)
    return transaction->transactionDbInfo(*this);
  return nullptr;
}


}

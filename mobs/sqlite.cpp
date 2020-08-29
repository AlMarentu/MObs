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

// TODO speichern von datum überarbeiten

#include "sqlite.h"

#include "dbifc.h"
#include "logging.h"
#include "objgen.h"
#include "unixtime.h"
#include "helper.h"
#include "mchrono.h"

#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>
#include <chrono>


namespace {
using namespace mobs;
using namespace std;

class sqlite_exception : public std::runtime_error {
public:
  sqlite_exception(const std::string &e, sqlite3 *con) : std::runtime_error(e + ": " + sqlite3_errmsg(con)) {
//    LOG(LM_DEBUG, "sqlite: Error " << sqlite3_errmsg(con));
  }
};


class SQLSQLiteDescription : public mobs::SQLDBdescription {
public:
  explicit SQLSQLiteDescription(const string &) {
    createWith_IfNotExists = true;
    dropWith_IfExists = true;
    replaceWithInto = true;
    withInsertOnConflict = false; ///< true: insert or update; false: insert or replace
  }

  std::string tableName(const std::string &tabnam) override { return tabnam;  }

  std::string valueStmtIndex(size_t i) override {
    if (useBind) {
      MobsMemberInfo mi{};
      mi.isUnsigned = true;
      mi.u64 = i;
      binding.emplace_back(make_pair(mi, nullptr));
      stringstream s;
      s << '?' << setw(3) << setfill('0') << ++pos;
      return s.str();
    }
    return std::to_string(i);
  }

  std::string valueStmtText(const std::string &tx, bool isNull) override {
    if (useBind) {
      if (isNull)
        binding.emplace_back(make_pair(MobsMemberInfo(), nullptr));
      else {
        char *cp = (char *)malloc(tx.length()+1);
        strcpy(cp, tx.c_str());
        binding.emplace_back(make_pair(MobsMemberInfo(), cp));
      }
      stringstream s;
      s << '?' << setw(3) << setfill('0') << ++pos;
      return s.str();
    }
    return isNull ? string("null"):mobs::to_squote(tx);
  }

  std::string createStmtIndex(std::string name) override { return "INT NOT NULL"; }

  std::string createStmtText(const std::string &name, size_t len) override { return "TEXT"; }

  std::string createStmt(const MemberBase &mem, bool compact) override {
    std::stringstream res;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    mi.changeCompact(compact);
    if (mi.isUnsigned or mi.isSigned)
      res << "INTEGER";
    else if (mi.isTime and mi.granularity >= 1000000)
      res << "INTEGER";
    else if (mi.isFloat or mi.isTime)
      res << "REAL";
    else if (mi.isBlob)
      res << "BLOB";
    else if (mem.is_chartype(mobs::ConvToStrHint(compact)))
      res << "TEXT";
// TODO Date = Numeric?
    if (not mem.nullAllowed())
      res << " NOT NULL";
    return res.str();
  }

  std::string valueStmt(const MemberBase &mem, bool compact, bool increment, bool inWhere) override {
    MobsMemberInfo mi;
    mem.memInfo(mi);
    mi.changeCompact(compact);
    if (increment) {
      if (mi.isUnsigned) {
        if (mi.u64 == mi.max)
          throw std::runtime_error("VersionElement overflow");
        mi.u64++;
      } else if (mi.isSigned) {
        if (mi.i64 == mi.max)
          throw std::runtime_error("VersionElement overflow");
        mi.i64++;
      } else
        throw std::runtime_error("VersionElement is not int");
    }
    if (useBind) {
      if (mem.isNull())
        binding.emplace_back(make_pair(MobsMemberInfo(), nullptr));
      else if (mi.isSigned or mi.isUnsigned or mi.isFloat or mi.isBlob)
        binding.emplace_back(make_pair(mi, nullptr));
      else {
        const string &value = mem.toStr(mobs::ConvToStrHint(compact));
        char *cp = (char *)malloc(value.length()+1);
        strcpy(cp, value.c_str());
        binding.emplace_back(make_pair(MobsMemberInfo(), cp));
      }
      stringstream s;
      s << '?' << setw(3) << setfill('0') << ++pos;
      return s.str();
    }
    if (mem.isNull())
      return u8"null";
    if (mi.isTime and mi.granularity >= 86400000000) { // nur Datum
      std::stringstream s;
      struct tm ts{};
      mi.toLocalTime(ts);
      s << std::put_time(&ts, "%F");
      return mobs::to_squote(s.str());
    }
    else if (mi.isTime) {
      MTime t;
      if (not from_number(mi.i64, t))
        throw std::runtime_error("Time Conversion");
      return mobs::to_squote(to_string_ansi(t));
    }
    else if (mi.isUnsigned and mi.max == 1) // bool
      return (mi.u64 ? "1" : "0");
    else if (mem.is_chartype(mobs::ConvToStrHint(compact)))
      return mobs::to_squote(mem.toStr(mobs::ConvToStrHint(compact)));

    return mem.toStr(mobs::ConvToStrHint(compact));
  }

  void readValue(MemberBase &mem, bool compact) override {
//    LOG(LM_DEBUG, "Read " << mem.getName(ConvObjToString().exportAltNames()) << " " << pos);
    const unsigned char *cp = sqlite3_column_text(stmt, pos);
    int n = sqlite3_column_bytes(stmt, pos);
    if (cp) {
      string value((const char *)(cp), n);
      MobsMemberInfo mi;
      mem.memInfo(mi);
      mi.changeCompact(compact);
      bool ok;
      if (mi.isTime and mi.granularity >= 86400000) {
        std::istringstream s(value);
        std::tm t = {};
        s >> std::get_time(&t, "%Y-%m-%d");
        ok = not s.fail();
        if (ok) {
          mi.fromLocalTime(t);
          ok = mem.fromMemInfo(mi);
        }
      } else if (mi.isTime) {
        ok = mem.fromStr(value, not compact ? ConvFromStrHint::convFromStrHintExplizit : ConvFromStrHint::convFromStrHintDflt);
      } else if (mi.isUnsigned and mi.max == 1) {// bool
        mi.u64 = value == "0" ? 0 : 1;
        ok = mem.fromMemInfo(mi);
      } else if (mi.isBlob) {
        mi.blob = sqlite3_column_blob(stmt, pos);
        if (not mi.blob)
          throw runtime_error(u8"is not a blob");
        mi.u64 = n;
        ok = mem.fromMemInfo(mi);
      } else //if (mem.is_chartype(mobs::ConvToStrHint(compact)))
        ok = mem.fromStr(value, not compact ? ConvFromStrHint::convFromStrHintExplizit : ConvFromStrHint::convFromStrHintDflt);

      if (not ok)
        throw runtime_error(u8"conversion error in " + mem.name() + " Value=" + value);
    } else
      mem.forceNull();
    pos++;
  }

  void readValueText(const std::string &name, std::string &text, bool &null) override {
//    LOG(LM_DEBUG, "Read " << name << " " << pos);
    const unsigned char *cp = sqlite3_column_text(stmt, pos++);
    null = not cp;
    if (cp)
      text = string((const char *)(cp));
  }

  size_t readIndexValue(const std::string &name) override {
//    LOG(LM_DEBUG, "Read " << name << " " << pos);
    return sqlite3_column_int(stmt, pos++);
  }

  void startReading() override {
    pos = 0;
    if (not stmt)
      throw runtime_error("Cursor read error");
  }
  void finishReading() override {}

  void bindValues(sqlite3 *connection, sqlite3_stmt *pStmt) {
    int i = 0;
//    LOG(LM_DEBUG, "bindValues " << binding.size());
    for (auto &v:binding) {
      i++;
//      LOG(LM_DEBUG, "BIND " << pos);
      int rc;
      MobsMemberInfo &mi = v.first;
      if (mi.isSigned)
        rc = sqlite3_bind_int64(pStmt, i, mi.i64);
      else if (mi.isUnsigned) {
        if (mi.u64 > INT64_MAX)
          throw runtime_error(u8"uint64 exceeds range");
        rc = sqlite3_bind_int64(pStmt, i, mi.u64);
      } else if (mi.isFloat)
        rc = sqlite3_bind_double(pStmt, i, mi.d);
      else if (mi.isBlob)
        rc = sqlite3_bind_blob64(pStmt, i, mi.blob, mi.u64, nullptr);
      else if (v.second)
        rc = sqlite3_bind_text(pStmt, i, v.second, -1, nullptr);
      else
        rc = sqlite3_bind_null(pStmt, i);

      if (rc != SQLITE_OK)
        throw sqlite_exception(u8"bind failed", connection);
    }
  }

  void clearBinds() {
    for (auto &v:binding) {
      if (v.second)
        free((void *)v.second);
    }
    binding.clear();
    pos = 0;
  }

  sqlite3_stmt *stmt = nullptr;
  bool useBind = false;

private:
  u_int pos = 0;
  list<pair<MobsMemberInfo, const char *>> binding;
};




class CountCursor : public virtual mobs::DbCursor {
  friend class mobs::SQLiteDatabaseConnection;
public:
  explicit CountCursor(size_t size) { cnt = size; }
  ~CountCursor() override = default;;
  bool eof() override  { return true; }
  bool valid() override { return false; }
  void operator++() override { }
};




class SQLiteCursor : public virtual mobs::DbCursor {
  friend class mobs::SQLiteDatabaseConnection;
public:
  explicit SQLiteCursor(sqlite3_stmt *stmt, std::shared_ptr<DatabaseConnection> dbi, std::string dbName) :
          stmt(stmt), dbCon(std::move(dbi)), databaseName(std::move(dbName)) { }
  ~SQLiteCursor() override { if (stmt) sqlite3_finalize(stmt); stmt = nullptr; }
  bool eof() override  { return not stmt; }
  bool valid() override { return not eof(); }
  void operator++() override {
    if (eof()) return;
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
      sqlite3_finalize(stmt);
      stmt = nullptr;
      if (rc != SQLITE_DONE) {
        auto mdb = dynamic_pointer_cast<SQLiteDatabaseConnection>(dbCon);
        if (mdb)
          throw sqlite_exception(u8"cursor: query row failed", mdb->getConnection());
      }
    }
    cnt++;
  }
private:
  sqlite3_stmt *stmt = nullptr;
  std::shared_ptr<DatabaseConnection> dbCon;  // verhindert das Zerstören der Connection
  std::string databaseName;  // unused
};

}

namespace mobs {

std::string SQLiteDatabaseConnection::tableName(const ObjectBase &obj, const DatabaseInterface &dbi) {
  MemVarCfg c = obj.hasFeature(ColNameBase);
  if (c)
    return dbi.database() + "." + obj.getConf(c);
  return dbi.database() + "." + obj.typeName();
}

void SQLiteDatabaseConnection::open() {
//  LOG(LM_DEBUG, "SQLite open " << PARAM(connection));
  if (connection)
    return;

  size_t pos = m_url.find("//");
  if (pos == string::npos or pos +2 == m_url.length())
    throw runtime_error("sqlite: error in url");
  string file = m_url.substr(pos+2);
  LOG(LM_INFO, "SQLite open " << file);
  int rc = sqlite3_open_v2(file.c_str(), &connection, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (rc)
    throw sqlite_exception(u8"connection failed", connection);
}

SQLiteDatabaseConnection::~SQLiteDatabaseConnection() {
  LOG(LM_DEBUG, "SQLite close");
  if (connection)
    sqlite3_close(connection);
  connection = nullptr;
}

bool SQLiteDatabaseConnection::load(DatabaseInterface &dbi, ObjectBase &obj) {
  open();
  SQLSQLiteDescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  string s = gsql.selectStatementFirst();
  LOG(LM_DEBUG, "SQL: " << s);
  sqlite3_stmt *ppStmt = nullptr;
  int rc = sqlite3_prepare_v2(connection, s.c_str(), s.length(), &ppStmt, nullptr);
  if (rc != SQLITE_OK)
    throw sqlite_exception(u8"prepare load failed", connection);
  rc = sqlite3_step(ppStmt);
  if (rc != SQLITE_ROW)
  {
    sqlite3_finalize(ppStmt);
    if (rc != SQLITE_DONE)
      throw sqlite_exception(u8"step load failed", connection);
    return false;
  }
  auto cursor = std::make_shared<SQLiteCursor>(ppStmt, dbi.getConnection(), dbi.database());
  retrieve(dbi, obj, cursor);
  return true;
}

void SQLiteDatabaseConnection::save(DatabaseInterface &dbi, const ObjectBase &obj) {
  SQLSQLiteDescription sd(dbi.database());
  sd.useBind = true;
  mobs::SqlGenerator gsql(obj, sd);
  try {
    open();
    // Transaktion benutzen zwecks Atomizität
    if (currentTransaction == nullptr) {
      string s = "BEGIN TRANSACTION;";
      LOG(LM_DEBUG, "SQL " << s);
      doSql(s);
      // Wenn DBI mit Transaktion, dann in Transaktion bleiben
    } else if (currentTransaction != dbi.getTransaction())
      throw std::runtime_error("transaction mismatch");
    else {
      string s = "SAVEPOINT MOBS;";
      LOG(LM_DEBUG, "SQL " << s);
      doSql(s);
    }
  } catch (std::exception &e) {
    THROW(u8"SQLite save transaction failed: " << e.what());
  }

  int64_t version = gsql.getVersion();
  LOG(LM_DEBUG, "VERSION IS " << version);

  try {
    string s;
    if (version == -1)
      s = gsql.replaceStatement(true);
    else if (version > 0)
      s = gsql.updateStatement(true);
    else
      s = gsql.insertStatement(true);
    LOG(LM_DEBUG, "SQL " << s);
    sqlite3_stmt *ppStmt = nullptr;
    int rc = sqlite3_prepare_v2(connection, s.c_str(), s.length(), &ppStmt, nullptr);
    if (rc != SQLITE_OK)
      throw sqlite_exception(u8"prepare save failed", connection);
    sd.bindValues(connection, ppStmt);
    sqlite3_step(ppStmt);
    sd.clearBinds();
    rc = sqlite3_finalize(ppStmt);
    if (rc != SQLITE_OK)
      throw sqlite_exception(u8"save failed", connection);

    int sz = sqlite3_changes(connection);
    LOG(LM_DEBUG, "ROWS " << sz);
    // wenn sich, obwohl gefunden, nichts geändert hat wird hier auch 0 geliefert - die Version muss sich aber immer ändern
    if (version > 0 and sz != 1)
      throw runtime_error(u8"number of processed rows is " + to_string(sz) + " should be 1");

    while (not gsql.eof()) {
      s = gsql.replaceStatement(false);
      LOG(LM_DEBUG, "SQL " << s);
      rc = sqlite3_prepare_v2(connection, s.c_str(), s.length(), &ppStmt, nullptr);
      if (rc != SQLITE_OK)
        throw sqlite_exception(u8"prepare save failed", connection);
      sd.bindValues(connection, ppStmt);
      sqlite3_step(ppStmt);
      sd.clearBinds();
      rc = sqlite3_finalize(ppStmt);
      if (rc != SQLITE_OK)
        throw sqlite_exception(u8"save failed", connection);    }
  } catch (runtime_error &e) {
    string s = "ROLLBACK TRANSACTION";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    try {
      doSql(s);
    } catch (std::exception &e) {
      LOG(LM_ERROR, u8"SQLite save rollback error: " << e.what());
    }
    THROW(u8"SQLite save: " << e.what());
  } catch (exception &e) {
    string s = "ROLLBACK TRANSACTION";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    try {
      doSql(s);
    } catch (std::exception &e) {
      LOG(LM_ERROR, u8"SQLite save rollback error: " << e.what());
    }
    THROW(u8"SQLite save: " << e.what());
  }

  try {
    string s;
    if (currentTransaction)
      s = "RELEASE SAVEPOINT MOBS;";
    else
      s = "COMMIT TRANSACTION;";
    LOG(LM_DEBUG, "SQL " << s);
    doSql(s);
  } catch (std::exception &e) {
    THROW(u8"SQLite save transaction failed: " << e.what());
  }
}



bool SQLiteDatabaseConnection::destroy(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  SQLSQLiteDescription sd(dbi.database());
  sd.useBind = true;
  mobs::SqlGenerator gsql(obj, sd);
  try {
    open();
    // Transaktion benutzen zwecks Atomizität
    if (currentTransaction == nullptr) {
      string s = "BEGIN TRANSACTION;";
      LOG(LM_DEBUG, "SQL " << s);
      doSql(s);
      // Wenn DBI mit Transaktion, dann in Transaktion bleiben
    } else if (currentTransaction != dbi.getTransaction())
      throw std::runtime_error("transaction mismatch");
    else {
      string s = "SAVEPOINT MOBS;";
      LOG(LM_DEBUG, "SQL " << s);
      doSql(s);
    }
  } catch (std::exception &e) {
    THROW(u8"SQLite destroy transaction failed: " << e.what());
  }

  int64_t version = gsql.getVersion();
  LOG(LM_DEBUG, "VERSION IS " << version);

  bool found = false;
  try {
    if (version == 0)
      throw runtime_error(u8"version = 0 cannot destroy");
    for (bool first = true; first or not gsql.eof(); first = false) {
      string s = gsql.deleteStatement(first);
      LOG(LM_DEBUG, "SQL " << s);
      sqlite3_stmt *ppStmt = nullptr;
      int rc = sqlite3_prepare_v2(connection, s.c_str(), s.length(), &ppStmt, nullptr);
      if (rc != SQLITE_OK)
        throw sqlite_exception(u8"prepare destroy failed", connection);
      sd.bindValues(connection, ppStmt);
      sqlite3_step(ppStmt);
      sd.clearBinds();
      rc = sqlite3_finalize(ppStmt);
      if (rc != SQLITE_OK)
        throw sqlite_exception(u8"save failed", connection);
      if (first) {
        found = sqlite3_changes(connection) > 0;
        if (version > 0 and not found)
          throw runtime_error(u8"Object with appropriate version not found");
      }
    }
  } catch (runtime_error &e) {
    string s = "ROLLBACK TRANSACTION";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    try {
      doSql(s);
    } catch (std::exception &e) {
      LOG(LM_ERROR, u8"SQLite destroy rollback error: " << e.what());
    }
    THROW(u8"SQLite destroy: " << e.what());
  } catch (exception &e) {
    string s = "ROLLBACK TRANSACTION";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    try {
      doSql(s);
    } catch (std::exception &e) {
      LOG(LM_ERROR, u8"SQLite destroy rollback error: " << e.what());
    }
    THROW(u8"SQLite destroy: " << e.what());
  }

  try {
    string s;
    if (currentTransaction)
      s = "RELEASE SAVEPOINT MOBS;";
    else
      s = "COMMIT TRANSACTION;";
    LOG(LM_DEBUG, "SQL " << s);
    doSql(s);
  } catch (std::exception &e) {
    THROW(u8"SQLite destroy transaction failed: " << e.what());
  }
  return found;
}

std::shared_ptr<DbCursor>
SQLiteDatabaseConnection::query(DatabaseInterface &dbi, ObjectBase &obj, const string &query, bool qbe) {
  SQLSQLiteDescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  try {
    open();
    string s;
    if (qbe)
      s = gsql.queryBE(dbi.getCountCursor() ? SqlGenerator::Count : SqlGenerator::Normal);
    else
      s = gsql.query(dbi.getCountCursor() ? SqlGenerator::Count : SqlGenerator::Normal, query);
    // TODO  s += " LOCK IN SHARE MODE WAIT 10 "; / NOWAIT

    LOG(LM_INFO, "SQL: " << s);
    sqlite3_stmt *ppStmt = nullptr;
    int rc = sqlite3_prepare_v2(connection, s.c_str(), s.length(), &ppStmt, nullptr);
    if (rc != SQLITE_OK)
      throw sqlite_exception(u8"prepare query failed", connection);
    rc = sqlite3_step(ppStmt);
    if (rc != SQLITE_ROW) {
      sqlite3_finalize(ppStmt);
      ppStmt = nullptr;
      if (rc != SQLITE_DONE)
        throw sqlite_exception(u8"query failed", connection);
    }

    if (dbi.getCountCursor()) {
      if (not ppStmt)
        throw runtime_error(u8"count without result");
      int64_t cnt = sqlite3_column_int64(ppStmt, 0);
      if (cnt < 0 or sqlite3_column_bytes(ppStmt, 0) == 0) {
        sqlite3_finalize(ppStmt);
        throw runtime_error(u8"count error");
      }
      sqlite3_finalize(ppStmt);
      return std::make_shared<CountCursor>(size_t(cnt));
    }
    auto cursor = std::make_shared<SQLiteCursor>(ppStmt, dbi.getConnection(), dbi.database());
    return cursor;
  } catch (runtime_error &e) {
    THROW(u8"SQLite query: " << e.what());
  } catch (exception &e) {
    THROW(u8"SQLite query: " << e.what());
  }
}

void
SQLiteDatabaseConnection::retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) {
  auto curs = std::dynamic_pointer_cast<SQLiteCursor>(cursor);
  if (not curs)
    throw runtime_error("SQLiteDatabaseConnection: invalid cursor");

  if (not curs->stmt) {
//    if (mysql_errno(connection))
//      throw sqlite_exception(u8"query row failed", connection);
    throw runtime_error("Cursor eof");
  }
  open();
  SQLSQLiteDescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  obj.clear();
  sd.stmt = curs->stmt;
  gsql.readObject(obj);

  while (not gsql.eof()) {
    SqlGenerator::DetailInfo di;
    string s = gsql.selectStatementArray(di);
    LOG(LM_DEBUG, "SQL " << s);
    sqlite3_stmt *ppStmt = nullptr;
    int rc = sqlite3_prepare_v2(connection, s.c_str(), s.length(), &ppStmt, nullptr);
    if (rc != SQLITE_OK)
      throw sqlite_exception(u8"prepare query detail failed", connection);
    try {
      // Vektor auf leer setzten (wurde wegen Struktur zuvor erweitert)
      di.vecNc->resize(0);
      sd.stmt = ppStmt;
      for (;;) {
        rc = sqlite3_step(ppStmt);
        if (rc != SQLITE_ROW) {
          sqlite3_finalize(ppStmt);
          if (rc != SQLITE_DONE)
            throw sqlite_exception(u8"query detail failed", connection);
          break;
        }
        gsql.readObject(di);
      }
    } catch (runtime_error &e) {
      THROW(u8"SQLite retrieve: " << e.what());
    } catch (exception &e) {
      THROW(u8"SQLite retrieve: " << e.what());
    }
  }

  LOG(LM_DEBUG, "RESULT " << obj.to_string());
}

void SQLiteDatabaseConnection::dropAll(DatabaseInterface &dbi, const ObjectBase &obj) {
  try {
    open();
    SQLSQLiteDescription sd(dbi.database());
    mobs::SqlGenerator gsql(obj, sd);

    for (bool first = true; first or not gsql.eof(); first = false) {
      string s = gsql.dropStatement(first);
      LOG(LM_DEBUG, "SQL " << s);
      doSql(s);
    }
  } catch (std::exception &e) {
    THROW(u8"SQLite dropAll: " << e.what());
  }
}

void SQLiteDatabaseConnection::structure(DatabaseInterface &dbi, const ObjectBase &obj) {
  try {
    open();
    SQLSQLiteDescription sd(dbi.database());
    mobs::SqlGenerator gsql(obj, sd);

    for (bool first = true; first or not gsql.eof(); first = false) {
      string s = gsql.createStatement(first);
      LOG(LM_DEBUG, "SQL " << s);
      doSql(s);
//    if (mysql_real_query(connection, s.c_str(), s.length()))
//      throw sqlite_exception(u8"create failed", connection);
    }
  } catch (std::exception &e) {
    THROW(u8"SQLite structure: " << e.what());
  }
}

sqlite3 *SQLiteDatabaseConnection::getConnection() {
  open();
  return connection;
}

size_t SQLiteDatabaseConnection::doSql(const string &sql)
{
  open();
  sqlite3_stmt *ppStmt = nullptr;
  int rc = sqlite3_prepare_v2(connection, sql.c_str(), sql.length(), &ppStmt, nullptr);
  if (rc != SQLITE_OK)
    throw sqlite_exception(u8"prepare failed", connection);
  rc = sqlite3_step(ppStmt);
  if (rc != SQLITE_DONE)
    LOG(LM_ERROR, "STEP FAILED " << rc);
  rc = sqlite3_finalize(ppStmt);
  if (rc != SQLITE_OK)
    throw sqlite_exception(u8"step failed", connection);
  return sqlite3_changes(connection);
}

void SQLiteDatabaseConnection::startTransaction(DatabaseInterface &dbi, DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) {
  try {
    open();
    if (currentTransaction == nullptr) {
      // SET SESSION idle_transaction_timeout=2;
      // SET SESSION idle_transaction_timeout=2, SESSION idle_readonly_transaction_timeout=10;
      string s = "BEGIN TRANSACTION;";
      LOG(LM_DEBUG, "SQL " << s);
      doSql(s);
      currentTransaction = transaction;
    } else if (currentTransaction != transaction)
      throw std::runtime_error("transaction mismatch"); // hier geht nur eine Transaktion gleichzeitig
  } catch (std::exception &e) {
    THROW(u8"SQLite startTransaction: failed " << e.what());
  }
}

void SQLiteDatabaseConnection::endTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) {
  try {
    if (currentTransaction == nullptr)
      return;
    else if (currentTransaction != transaction)
      throw std::runtime_error("transaction mismatch");
    string s = "COMMIT TRANSACTION;";
    LOG(LM_DEBUG, "SQL " << s);
    doSql(s);
    currentTransaction = nullptr;
  } catch (std::exception &e) {
    currentTransaction = nullptr;
    THROW(u8"SQLite transaction failed: " << e.what());
  }
}

void SQLiteDatabaseConnection::rollbackTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) {
  if (currentTransaction == nullptr)
    return;
  try {
    string s = "ROLLBACK TRANSACTION;";
    LOG(LM_DEBUG, "SQL " << s);
    doSql(s);
    currentTransaction = nullptr;
  } catch (std::exception &e) {
    currentTransaction = nullptr;
    THROW(u8"SQLite transaction failed: " << e.what());
  }
}

size_t SQLiteDatabaseConnection::maxAuditChangesValueSize(const DatabaseInterface &dbi) const {
  return 200;
}


}

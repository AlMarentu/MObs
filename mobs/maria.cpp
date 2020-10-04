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

// TODO dirtyRead, Isolationlevel

#include "maria.h"

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

class mysql_exception : public std::runtime_error {
public:
  mysql_exception(const std::string &e, MYSQL *con) : std::runtime_error(string("mysql ") + e + ": " + mysql_error(con)) {
    LOG(LM_DEBUG, "mysql: Error " << mysql_error(con)); }
//  const char* what() const noexcept override { return error.c_str(); }
//private:
//  std::string error;
};


class SQLMariaDBdescription : public mobs::SQLDBdescription {
public:
  explicit SQLMariaDBdescription(const string &dbName) : dbPrefix(dbName + ".") {
    createWith_IfNotExists = true;
  }

  std::string tableName(const std::string &tabnam) override { return dbPrefix + tabnam;  }

  std::string valueStmtIndex(size_t i) override { return std::to_string(i);  }

  std::string valueStmtText(const std::string &tx, bool isNull) override { return isNull ? string("null"):mobs::to_squote(tx); }

  std::string createStmtIndex(std::string name) override { return "INT NOT NULL"; }

  std::string createStmtText(const std::string &name, size_t len) override {
    return string("VARCHAR(") + std::to_string(len) + ") CHARACTER SET utf8";
  }

  std::string createStmt(const MemberBase &mem, bool compact) override {
    std::stringstream res;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    mi.changeCompact(compact);
    if (mi.isTime and mi.granularity >= 86400000000)
      res << "DATE";
    else if (mi.isTime and mi.granularity >= 1000000)
      res << "DATETIME";
    else if (mi.isTime and mi.granularity >= 100000)
      res << "DATETIME(1)";
    else if (mi.isTime and mi.granularity >= 10000)
      res << "DATETIME(2)";
    else if (mi.isTime and mi.granularity >= 1000)
      res << "DATETIME(3)";
    else if (mi.isTime and mi.granularity >= 100)
      res << "DATETIME(4)";
    else if (mi.isTime and mi.granularity >= 10)
      res << "DATETIME(5)";
    else if (mi.isTime)
      res << "DATETIME(6)";
    else if (mi.isUnsigned and mi.max == 1)
      res << "TINYINT";
    else if (mi.isFloat)
      res << "FLOAT";
    else if (mem.is_chartype(mobs::ConvToStrHint(compact))) {
      if (mi.is_specialized and mi.size == 1)
        res << "CHAR(1)";
      else {
        MemVarCfg c = mem.hasFeature(LengthBase);
        size_t n = c ? (c - LengthBase) : 30;
        if (n <= 4)
          res << "CHAR(" << n << ")";
        else
          res << "VARCHAR(" << n << ") CHARACTER SET utf8";
      }
    }
    else if (mi.isSigned and mi.max <= INT16_MAX)
      res << "SMALLINT";
    else if (mi.isSigned and mi.max <= INT32_MAX)
      res << "INT";
    else if (mi.isSigned)
      res << "BIGINT";
    else if (mi.isUnsigned)
      res << "BIGINT UNSIGNED";
    else
      res << "SMALLINT";
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
        return std::to_string(mi.u64 + 1);
      }
      if (mi.isSigned) {
        if (mi.i64 == mi.max)
          throw std::runtime_error("VersionElement overflow");
        return std::to_string(mi.i64 + 1);
      }
      throw std::runtime_error("VersionElement is not int");
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
//    LOG(LM_DEBUG, "Read " << mem.getName(ConvObjToString().exportAltNames()) << " " << string(fields[pos].name, fields[pos].name_length));
//     IS_NUM(fields[pos].type)
    if ((*row)[pos]) {
      string value((*row)[pos], lengths[pos]);
      MobsMemberInfo mi;
      mem.memInfo(mi);
      mi.changeCompact(compact);
      bool ok = true;
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
#if 0
        MTime t;
        ok = string2x(value, t);
        std::istringstream s(value);
        std::tm t = {};
        s >> std::get_time(&t, "%F %T");
        ok = not s.fail();
        if (ok)
          mi.fromLocalTime(t);
        if (ok and mi.granularity < 1000000) {
          unsigned int i;
          char c;
          s.get(c);
          if (not s.fail()) {
            s >> i;
            ok = not s.fail() and c == '.' and i < 1000;
            mi.i64 += i;
          }
        }
        if (ok)
          ok = mem.fromInt64(mi.i64);
#endif
      } else if (mi.isUnsigned and mi.max == 1) {// bool
        mi.u64 = value == "0" ? 0 : 1;
        ok = mem.fromMemInfo(mi);
      } else //if (mem.is_chartype(mobs::ConvToStrHint(compact)))
        ok = mem.fromStr(value, not compact ? ConvFromStrHint::convFromStrHintExplizit : ConvFromStrHint::convFromStrHintDflt);

      if (not ok)
        throw runtime_error(u8"conversion error in " + mem.getElementName() + " Value=" + value);
    } else
      mem.forceNull();
    pos++;
  }

  void readValueText(const std::string &name, std::string &text, bool &null) override {
    if ((*row)[pos]) {
      null = false;
      text = string((*row)[pos], lengths[pos]);
    }
    else
      null = true;
    pos++;
  }

  size_t readIndexValue(const std::string &name) override {
    if ((*row)[pos]) {
      string value((*row)[pos], lengths[pos]);
      pos++;
      return stoull(value, nullptr);
    }
    throw runtime_error("index value is null");
  }

  void startReading() override {
    pos = 0;
    fields = mysql_fetch_fields(result);
    lengths = mysql_fetch_lengths(result);
    if (not fields or not lengths)
      throw runtime_error("Cursor read error");
  }
  void finishReading() override {}

  MYSQL_RES *result = nullptr;
  MYSQL_ROW *row = nullptr;

private:
  std::string dbPrefix;
  MYSQL_FIELD *fields = nullptr;
  unsigned long *lengths = nullptr;
  u_int pos = 0;
};




class CountCursor : public virtual mobs::DbCursor {
  friend class mobs::MariaDatabaseConnection;
public:
  explicit CountCursor(size_t size) { cnt = size; }
  ~CountCursor() override = default;;
  bool eof() override  { return true; }
  bool valid() override { return false; }
  void operator++() override { }
};




class MariaCursor : public virtual mobs::DbCursor {
  friend class mobs::MariaDatabaseConnection;
public:
  explicit MariaCursor(MYSQL_RES *result, unsigned int fldCnt, std::shared_ptr<DatabaseConnection> dbi,
                       std::string dbName, bool keysOnly) :
          result(result), fldCnt(fldCnt), dbCon(std::move(dbi)), databaseName(std::move(dbName)), isKeysOnly(keysOnly)
  {  row = mysql_fetch_row(result); }
  ~MariaCursor() override { if (result) mysql_free_result(result); result = nullptr; }
  bool eof() override  { return not row; }
  bool valid() override { return not eof(); }
  bool keysOnly() const override { return isKeysOnly; }
  void operator++() override {
    if (eof()) return;
    row = mysql_fetch_row(result);
    cnt++;
    if (not row) {
      auto mdb = dynamic_pointer_cast<MariaDatabaseConnection>(dbCon);
      if (mdb and mysql_errno(mdb->getConnection()))
        throw mysql_exception(u8"cursor: query row failed", mdb->getConnection());
      mysql_free_result(result);
      result = nullptr;
    }
  }
private:
  MYSQL_RES *result;
  unsigned int fldCnt;
  std::shared_ptr<DatabaseConnection> dbCon;  // verhindert das Zerstören der Connection
  std::string databaseName;  // unused
  MYSQL_ROW row = nullptr;
  bool isKeysOnly;
};

}

namespace mobs {

std::string MariaDatabaseConnection::tableName(const ObjectBase &obj, const DatabaseInterface &dbi) {
  MemVarCfg c = obj.hasFeature(ColNameBase);
  if (c)
    return dbi.database() + "." + obj.getConf(c);
  return dbi.database() + "." + obj.getObjectName();
}

void MariaDatabaseConnection::open() {
//  LOG(LM_DEBUG, "MariaDb open " << PARAM(connection));
  if (connection)
    return;

//      static char *server_options[] = {
//            "mysql_test", // An unused string
//            "--datadir=/tmp/mysql_embedded_data", // Your data dir
//            NULL };
//    int num_elements = (sizeof(server_options) / sizeof(char *)) - 1;
//    static char *server_groups[] = { "libmysqld_server",
//                                     "libmysqld_client", NULL };


//    mysql_library_init(num_elements, server_options, server_groups);

  size_t pos = m_url.find("//");
  if (pos == string::npos)
    throw runtime_error("mysql: error in url");
  size_t pos2 = m_url.find(':', pos);
  string host;
  unsigned int port = 0;
  if (pos2 == string::npos)
    pos2 = m_url.length();
  else
    try {
      size_t p;
      port = stoul(m_url.substr(pos2 + 1), &p);
      if (p +pos2 +1 != m_url.length())
        throw runtime_error(u8"mysql: invalid port");
    } catch (...) {
      throw runtime_error(u8"mysql: invalid port");
    }

  host = m_url.substr(pos+2, pos2-pos-2);
//    cerr << " PORT " << port << " " << host << endl;
  connection = mysql_init(nullptr);
  if (connection == nullptr)
    throw runtime_error(u8"mysql connection error");
  mysql_options(connection, MYSQL_SET_CHARSET_NAME, "utf8");
  if (mysql_real_connect(connection, host.c_str(), m_user.c_str(), m_password.c_str(),
                         m_database.c_str(), port, nullptr, 0) == nullptr)
    throw mysql_exception(u8"connection failed", connection);
}

MariaDatabaseConnection::~MariaDatabaseConnection() {
  LOG(LM_DEBUG, "MariaDb close");
  if (connection)
    mysql_close(connection);
  connection = nullptr;
}

bool MariaDatabaseConnection::load(DatabaseInterface &dbi, ObjectBase &obj) {
  open();
  SQLMariaDBdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  string s = gsql.selectStatementFirst();
  LOG(LM_DEBUG, "SQL: " << s);
  if (mysql_real_query(connection, s.c_str(), s.length()))
    throw mysql_exception(u8"load failed", connection);
  MYSQL_RES *result = mysql_store_result(connection);
  if (result == nullptr)
    throw mysql_exception(u8"load store failed", connection);
  unsigned int sz = mysql_field_count(connection);
  auto cursor = std::make_shared<MariaCursor>(result, sz, dbi.getConnection(), dbi.database(), false);
  if (cursor->row == nullptr and mysql_errno(connection))
    throw mysql_exception(u8"load row failed", connection);
  if (not cursor->row) {
//    LOG(LM_DEBUG, "NOW ROWS FOUND");
    return false;
  }

  retrieve(dbi, obj, cursor);
  return true;
}

void MariaDatabaseConnection::save(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  SQLMariaDBdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  // Transaktion benutzen zwecks Atomizität
  if (currentTransaction == nullptr) {
    string s = "BEGIN WORK;";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"Transaction failed", connection);
    // Wenn DBI mit Transaktion, dann in Transaktion bleiben
  }
  else if (currentTransaction != dbi.getTransaction())
    throw std::runtime_error("transaction mismatch");
  else {
    string s = "SAVEPOINT MOBS;";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"Transaction failed", connection);
  }
  int64_t version = gsql.getVersion();
  LOG(LM_DEBUG, "VERSION IS " << version);

  bool insertOnly = version == 0;
  try {
    string s;
    if (insertOnly)
      s = gsql.insertStatement(true);
    else if (version > 0)
      s = gsql.updateStatement(true);
    else
      s = gsql.replaceStatement(true);
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"save failed", connection);
    int rows = mysql_affected_rows(connection);
    LOG(LM_DEBUG, "ROWS " << rows);
    // update: wenn sich, obwohl gefunden, nichts geändert hat wird hier auch 0 geliefert - die Version muss sich aber immer ändern
    // replace: 2, wenn wenn zuvor delete nötig
    if (version > 0 and rows != 1)
      throw runtime_error(u8"number of processed rows is " + to_string(mysql_affected_rows(connection)) + " should be 1");
    if (not insertOnly and version < 0 and rows == 1) // wen bei replace 1 geliefert wird war es ein insert
      insertOnly = true;
    while (not gsql.eof()) {
      if (insertOnly) // Bei insert MasterTable reicht auch ein insert auf SubElemente
        s = gsql.insertStatement(false);
      else
        s = gsql.replaceStatement(false);
      LOG(LM_DEBUG, "SQL " << s);
      if (mysql_real_query(connection, s.c_str(), s.length()))
        throw mysql_exception(u8"save failed", connection);
    }
  } catch (runtime_error &e) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"Transaction failed", connection);
    throw e;
  } catch (exception &e) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"Transaction failed", connection);
    throw e;
  }

  string s;
  if (currentTransaction)
    s = "RELEASE SAVEPOINT MOBS;";
  else
    s = "COMMIT WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  if (mysql_real_query(connection, s.c_str(), s.length()))
    throw mysql_exception(u8"Transaction failed", connection);
}



bool MariaDatabaseConnection::destroy(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  SQLMariaDBdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  // Transaktion benutzen zwecks Atomizität
  if (currentTransaction == nullptr) {
    string s = "BEGIN WORK;";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"Transaction failed", connection);
    // Wenn DBI mit Transaktion, dann in Transaktion bleiben
  }
  else if (currentTransaction != dbi.getTransaction())
    throw std::runtime_error("transaction mismatch");
  else {
    string s = "SAVEPOINT MOBS;";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"Transaction failed", connection);
  }

  int64_t version = gsql.getVersion();
  LOG(LM_DEBUG, "VERSION IS " << version);
  if (version == 0)
    throw runtime_error(u8"destroy Object version = 0 cannot destroy");

  bool found = false;
  try {
    for (bool first = true; first or not gsql.eof(); first = false) {
      string s = gsql.deleteStatement(first);
      LOG(LM_DEBUG, "SQL " << s);
      if (mysql_real_query(connection, s.c_str(), s.length()))
        throw mysql_exception(u8"destroy failed", connection);
      if (first) {
        found = (mysql_affected_rows(connection) > 0);
        if (version > 0 and not found)
          throw runtime_error(u8"destroy: Object with appropriate version not found");
      }
    }
  } catch (runtime_error &e) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"Transaction failed", connection);
    THROW(u8"MariaDB destroy: " << e.what());
  } catch (exception &e) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"Transaction failed", connection);
    THROW(u8"MariaDB destroy: " << e.what());
  }

  string s;
  if (currentTransaction)
    s = "RELEASE SAVEPOINT MOBS;";
  else
    s = "COMMIT WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  if (mysql_real_query(connection, s.c_str(), s.length()))
    throw mysql_exception(u8"Transaction failed", connection);

  return found;
}

std::shared_ptr<DbCursor>
MariaDatabaseConnection::query(DatabaseInterface &dbi, ObjectBase &obj, bool qbe, const QueryGenerator *query,
                               const QueryOrder *sort) {
  open();
  SQLMariaDBdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  string sqlLimit;
  if (not dbi.getCountCursor() and (dbi.getQueryLimit() > 0 or dbi.getQuerySkip() > 0))
    sqlLimit = STRSTR(" LIMIT " << dbi.getQuerySkip() << ',' << dbi.getQueryLimit());

  string s;
  SqlGenerator::QueryMode qm = dbi.getKeysOnly() ? SqlGenerator::Keys : SqlGenerator::Normal;
  if (dbi.getCountCursor())
    qm = SqlGenerator::Count;
  if (qbe)
    s = gsql.queryBE(qm, sort, nullptr, sqlLimit);
  else
    s = gsql.query(qm, sort, query, "", sqlLimit);
  // TODO  s += " LOCK IN SHARE MODE WAIT 10 "; / NOWAIT

  LOG(LM_INFO, "SQL: " << s);
  if (mysql_real_query(connection, s.c_str(), s.length()))
    throw mysql_exception(u8"query failed", connection);
  unsigned int sz = mysql_field_count(connection);

  MYSQL_RES *result;
  if (dbi.getCountCursor() or gsql.queryWithJoin())
    result = mysql_store_result(connection);
  else
    result = mysql_use_result(connection);
  if (result == nullptr or sz == 0)
    throw mysql_exception(u8"load failed", connection);

  if (dbi.getCountCursor()) {
     MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr or row[0] == nullptr)
      throw runtime_error("query failed");
    unsigned long *lengths = mysql_fetch_lengths(result);
    string value(row[0], lengths[0]);
    size_t cnt = stoull(value, nullptr);
    mysql_free_result(result);
    return std::make_shared<CountCursor>(cnt);
  }

  auto cursor = std::make_shared<MariaCursor>(result, sz, dbi.getConnection(), dbi.database(), dbi.getKeysOnly());
  if (cursor->row == nullptr and mysql_errno(connection))
    throw mysql_exception(u8"query row failed", connection);
  if (not cursor->row) {
    LOG(LM_DEBUG, "NOW ROWS FOUND");
    mysql_free_result(cursor->result);
    cursor->result = nullptr;
  }
  return cursor;
}

void
MariaDatabaseConnection::retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) {
  auto curs = std::dynamic_pointer_cast<MariaCursor>(cursor);
  if (not curs)
    throw runtime_error("MariaDatabaseConnection: invalid cursor");

  if (not curs->row) {
    if (mysql_errno(connection))
      throw mysql_exception(u8"query row failed", connection);
    throw runtime_error("Cursor eof");
  }
  open();
  SQLMariaDBdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  obj.clear();
  sd.result = curs->result;
  sd.row = &curs->row;
  if (curs->isKeysOnly)
    gsql.readObjectKeys(obj);
  else
    gsql.readObject(obj);

  while (not gsql.eof()) {
    SqlGenerator::DetailInfo di;
    string s = gsql.selectStatementArray(di);
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"query detail failed", connection);

//    unsigned int sz = mysql_field_count(connection);
    sd.result = mysql_store_result(connection);
    try {
      if (sd.result == nullptr)
        throw mysql_exception(u8"load detail failed", connection);
      // Vektor auf leer setzten (wurde wegen Struktur zuvor erweitert)
      di.vecNc->resize(0);
      for (;;) {
        MYSQL_ROW row = mysql_fetch_row(sd.result);
        if (row == nullptr)
          break;
        sd.row = &row;
        gsql.readObject(di);
      }
    }  catch (runtime_error &e) {
      mysql_free_result(sd.result);
      throw e;
    }  catch (exception &e) {
      mysql_free_result(sd.result);
      throw e;
    }
    mysql_free_result(sd.result);
  }

  LOG(LM_DEBUG, "RESULT " << obj.to_string());
}

void MariaDatabaseConnection::dropAll(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  SQLMariaDBdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  for (bool first = true; first or not gsql.eof(); first = false) {
    string s = gsql.dropStatement(first);
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"dropAll failed", connection);
  }
}

void MariaDatabaseConnection::structure(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  SQLMariaDBdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  for (bool first = true; first or not gsql.eof(); first = false) {
    string s = gsql.createStatement(first);
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"create failed", connection);
  }
}

MYSQL *MariaDatabaseConnection::getConnection() {
  open();
  return connection;
}

size_t MariaDatabaseConnection::doSql(const string &sql)
{
  if (mysql_real_query(connection, sql.c_str(), sql.length()))
    throw mysql_exception(u8"SQL failed", connection);
  return mysql_affected_rows(connection);
}

void MariaDatabaseConnection::startTransaction(DatabaseInterface &dbi, DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) {
  open();
  if (currentTransaction == nullptr) {
    // SET SESSION idle_transaction_timeout=2;
    // SET SESSION idle_transaction_timeout=2, SESSION idle_readonly_transaction_timeout=10;
    string s = "BEGIN WORK;";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"Transaction failed", connection);
    currentTransaction = transaction;
  }
  else if (currentTransaction != transaction)
    throw std::runtime_error("transaction mismatch"); // hier geht nur eine Transaktion gleichzeitig
}

void MariaDatabaseConnection::endTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) {
  if (currentTransaction == nullptr)
    return;
  else if (currentTransaction != transaction)
    throw std::runtime_error("transaction mismatch");
  string s = "COMMIT WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  if (mysql_real_query(connection, s.c_str(), s.length()))
    throw mysql_exception(u8"Transaction failed", connection);
  currentTransaction = nullptr;
}

void MariaDatabaseConnection::rollbackTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) {
  if (currentTransaction == nullptr)
    return;
  string s = "ROLLBACK WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  if (mysql_real_query(connection, s.c_str(), s.length()))
    throw mysql_exception(u8"Transaction failed", connection);
  currentTransaction = nullptr;
}

size_t MariaDatabaseConnection::maxAuditChangesValueSize(const DatabaseInterface &dbi) const {
  return 200;
}


}

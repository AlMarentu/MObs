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


#include "maria.h"

#include "dbifc.h"
#include "logging.h"
#include "objgen.h"
#include "unixtime.h"
#include "helper.h"

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
  mysql_exception(const std::string &e, MYSQL *con) : std::runtime_error(string("mysql ") + e + ": " + mysql_error(con)) {  }
//  mysql_exception(const char *e, MYSQL *con) : std::runtime_error(string("mysql ") + e + ": " + mysql_error(con)) {  }
};

class GenSql : public GenerateSql {
public:
  explicit GenSql(GenerateSql::Mode m, ConvObjToString c, string pfx = "") : GenerateSql(m, c.exportAltNames()) { keyPrefix = pfx; };

  std::string createStmt(const MemberBase &mem, bool compact) override {
    std::stringstream res;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    double d;
    if (mi.isTime and mi.granularity >= 86400000)
      res << "DATE";
    else if (mi.isTime)
      res << "DATETIME";
    else if (mi.isUnsigned and mi.max == 1)
      res << "TINYINT";
    else if (mem.toDouble(d))
      res << "FLOAT";
    else if (mem.is_chartype(mobs::ConvToStrHint(compact)))
      res << "VARCHAR(30)";
    else if (mi.isSigned and mi.max <= 32767)
      res << "SMALLINT";
    else if (mi.isSigned)
      res << "INT";
    else if (mi.isUnsigned)
      res << "INT";
    else
      res << "SMALLINT";
    if (not mem.nullAllowed())
      res << " NOT NULL";
    return res.str();
  }

  std::string valueStmt(const MemberBase &mem, bool compact) override {
    if (mem.isNull()) {
      if (mode == Query)
        return u8" is null";
      if (mode != Values)
        return u8"=null";
      return u8"null";
    }
    std::string pf;
    if (mode == Query or mode == Update)
      pf = "=";
    MobsMemberInfo mi;
    mem.memInfo(mi);
    if (mi.isTime and mi.granularity >= 86400000)
      return pf + "DATE";
    else if (mi.isTime)
      return pf + "DATETIME";
    else if (mi.isUnsigned and mi.max == 1) // bool
      return pf + (mi.u64 ? "1" : "0");
    else if (mem.is_chartype(mobs::ConvToStrHint(compact)))
      return pf + mobs::to_squote(mem.toStr(mobs::ConvToStrHint(compact)));

    return pf + mem.toStr(mobs::ConvToStrHint(compact));
  }

};





class ReadSql : public ExtractSql {
public:
  explicit ReadSql(MYSQL_RES *res, MYSQL_ROW &rowref, ConvObjToString c, u_int skip =0) : ExtractSql(Fields, c.exportAltNames()),
          pos(skip), result(res), row(rowref), fields(mysql_fetch_fields(res)), lengths(mysql_fetch_lengths(res))  { };

  void readValue(MemberBase &mem, bool compact) override {
//    LOG(LM_DEBUG, "Read " << mem.getName(cth) << " " << string(fields[pos].name, fields[pos].name_length));
    // IS_NUM(fields[pos].type)
    if (row[pos]) {
      string value(row[pos], lengths[pos]);
      MobsMemberInfo mi;
      mem.memInfo(mi);
      bool ok = true;
      if (mi.isTime and mi.granularity >= 86400000)
        ok = false;
      else if (mi.isTime)
        ok = false;
      else if (mi.isUnsigned and mi.max == 1) // bool
        ok = mem.fromUInt64(value == "0" ? 0 : 1);
      else //if (mem.is_chartype(mobs::ConvToStrHint(compact)))
        ok = mem.fromStr(value, not compact ? ConvFromStrHint::convFromStrHintExplizit : ConvFromStrHint::convFromStrHintDflt);

      if (not ok)
        throw runtime_error("conversion error");
    } else
      mem.forceNull();

    pos++;
  }

private:
  MYSQL_RES *result;
  MYSQL_ROW &row;
  MYSQL_FIELD *fields;
  unsigned long *lengths;
  u_int pos;
//  stringstream res;
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
                       std::string dbName) :
          result(result), fldCnt(fldCnt), dbCon(std::move(dbi)), databaseName(std::move(dbName)) {  row = mysql_fetch_row(result); }
  ~MariaCursor() override { if (result) mysql_free_result(result); result = nullptr; }
  bool eof() override  { return not row; }
  bool valid() override { return not eof(); }
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

};

}

namespace mobs {

std::string MariaDatabaseConnection::tableName(const ObjectBase &obj, const DatabaseInterface &dbi) {
  MemVarCfg c = obj.hasFeature(ColNameBase);
  if (c)
    return dbi.database() + "." + obj.getConf(c);
  return dbi.database() + "." + obj.typeName();
}

static std::string vecTableName(const MemBaseVector &vec, const DatabaseInterface &dbi) {
  MemVarCfg c = vec.hasFeature(ColNameBase);
  if (c and vec.parent())
    return dbi.database() + "." + vec.parent()->getConf(c);
  else
    return dbi.database() + "." + vec.contentObjName();
}

void MariaDatabaseConnection::open() {
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
  if (connection)
    mysql_close(connection);
  connection = nullptr;
}

bool MariaDatabaseConnection::load(DatabaseInterface &dbi, ObjectBase &obj) {
  open();
  GenSql gs(GenerateSql::Fields, ConvObjToString());
  obj.traverse(gs);
  GenSql gq(GenerateSql::Query, ConvObjToString());
  obj.traverseKey(gq);

  string q = "select ";
  q += gs.result();
  q += " from ";
  q += tableName(obj, dbi);
  q += " where ";
  q += gq.result();
  q += ";";
  LOG(LM_INFO, "SQL: " << q);
  if (mysql_real_query(connection, q.c_str(), q.length()))
    throw mysql_exception(u8"load failed", connection);
  bool found = false;
  MYSQL_RES *result = mysql_store_result(connection);
  if (result == nullptr)
    throw mysql_exception(u8"load store failed", connection);
  unsigned int sz = mysql_field_count(connection);
  auto cursor = std::make_shared<MariaCursor>(result, sz, dbi.getConnection(), dbi.database());
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
  std::list<const MemBaseVector *> detailVec;
  GenSql gf(GenerateSql::Fields, ConvObjToString());
  GenSql gv(GenerateSql::Values, ConvObjToString());
//  GenSql gk(GenerateSql::Query, ConvObjToString());
//  obj.traverseKey(gk);
  obj.traverse(gf);
  obj.traverse(gv);
  string s = "replace ";
  s += tableName(obj, dbi);
  s += "(";
  s += gf.result();
  s += ") values (";
  s += gv.result();
  s += ");";
//  s += gk.result();
//  s += ";";
  LOG(LM_DEBUG, "SQL " << s);
  if (mysql_real_query(connection, s.c_str(), s.length()))
    throw mysql_exception(u8"create failed", connection);

  detailVec.splice(detailVec.end(), gf.detailVec, gf.detailVec.begin(), gf.detailVec.end());
  for (auto v:detailVec) {
    GenSql gf(GenerateSql::Fields, ConvObjToString());
    GenSql gk(GenerateSql::Values, ConvObjToString());
    gf.parentMode = true;
    gf.startVec = v;
    v->traverse(gf);
    gk.parentMode = true;
    gk.startVec = v;
    v->traverseKey(gk);
    detailVec.splice(detailVec.end(), gf.detailVec, gf.detailVec.begin(), gf.detailVec.end());
    string dbn = vecTableName(*v, dbi);

    string s = "replace ";
    s += dbn;
    s += "(";
    s += gf.result();
    s += ") values ";
    size_t sz = v->size();
    string delIdx;
    for (size_t i = 0; i < sz; i++) {
      const ObjectBase *vobj = const_cast<MemBaseVector *>(v)->getObjInfo(i);
      if (vobj->isNull()) {
        if (not delIdx.empty())
          delIdx += ",";
        delIdx += to_string(i);
        continue;
      }
      GenSql gv(GenerateSql::Values, ConvObjToString());
      vobj->traverse(gv);
      if (i > 0)
        s += ",";
      s += "(";
      s += gk.result();
      s += ",";
      s += to_string(i);
      s += ",";
      s += gv.result();
      s += ")";
    }
    s += ";";
    if (sz > 0) {
      LOG(LM_DEBUG, "SQL " << s);
      if (mysql_real_query(connection, s.c_str(), s.length()))
        throw mysql_exception(u8"create failed", connection);
    }

    GenSql gq(GenerateSql::Query, ConvObjToString());
    gq.parentMode = true;
    gq.startVec = v;
    v->traverseKey(gq);
    s = "delete from ";
    s += dbn;
    s += " where ";
    s += gq.result();
    s += " and (a_idx >";
    s += to_string(sz-1);
    if (not delIdx.empty())
      s += string(" or a_idx in (") + delIdx + ")";
    s += ");";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"create failed", connection);

  }
}

bool MariaDatabaseConnection::destroy(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  std::list<const MemBaseVector *> detailVec;
  GenSql gs(GenerateSql::Fields, ConvObjToString());
  gs.arrayStructureMode = true;
  obj.traverse(gs);
  GenSql gk(GenerateSql::Query, ConvObjToString());
  obj.traverseKey(gk);

  string q = "delete from ";
  q += tableName(obj, dbi);
  q += " where ";
  q += gk.result();
  q += ";";
  LOG(LM_INFO, "SQL: " << q);
  if (mysql_real_query(connection, q.c_str(), q.length()))
    throw mysql_exception(u8"load failed", connection);

  bool found = (mysql_affected_rows(connection) > 0);

  detailVec.splice(detailVec.end(), gs.detailVec, gs.detailVec.begin(), gs.detailVec.end());
  for (auto v:detailVec) {
    GenSql gf(GenerateSql::Fields, ConvObjToString());
    GenSql gk(GenerateSql::Values, ConvObjToString());
    gf.parentMode = true;
    gf.startVec = v;
    gf.arrayStructureMode = true;
    v->traverse(gf);
    gk.parentMode = true;
    gk.startVec = v;
    v->traverseKey(gk);
    detailVec.splice(detailVec.end(), gf.detailVec, gf.detailVec.begin(), gf.detailVec.end());
    string dbn = vecTableName(*v, dbi);

    GenSql gq(GenerateSql::Query, ConvObjToString());
    gq.parentMode = true;
    gq.startVec = v;
    v->traverseKey(gq);
    string s = "delete from ";
    s += dbn;
    s += " where ";
    s += gq.result();
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"create failed", connection);
  }
  return found;
}

std::shared_ptr<DbCursor>
MariaDatabaseConnection::query(DatabaseInterface &dbi, ObjectBase &obj, const string &query, bool qbe) {
  open();
  GenSql gs(GenerateSql::Fields, ConvObjToString(), "mt.");
  if (dbi.getCountCursor())
    obj.traverseKey(gs);
  else
    obj.traverse(gs);

  string where = query;
  string sJoin;
  if (qbe) {
    where = "";
    std::list<const MemBaseVector *> detailVec;
    GenSql gq(GenerateSql::Query, ConvObjToString().exportModified(), "mt.");
    obj.traverse(gq);
    detailVec.splice(detailVec.end(), gq.detailVec, gq.detailVec.begin(), gq.detailVec.end());
    size_t join = 0;
    for (auto v:detailVec) {
      string pfJoin = string("jt") + to_string(join++);
      GenSql gq(GenerateSql::Query, ConvObjToString().exportModified(), pfJoin + ".");
      gq.startVec = v;
      v->traverse(gq);
      detailVec.splice(detailVec.end(), gq.detailVec, gq.detailVec.begin(), gq.detailVec.end());

      if (gq.result().empty())
        continue;

      // TODO 2. prefix ist immer mt, muss aber nicht sein
      GenSql gj(GenerateSql::Join, ConvObjToString(), pfJoin + ".");
      v->parent()->traverseKey(gj);
      sJoin += " left join  ";
      sJoin += vecTableName(*v, dbi);
      sJoin += " ";
      sJoin += pfJoin;
      sJoin += " on ";
      sJoin += gj.result();

      if (not where.empty())
        where += " and ";
      where += gq.result();
    }
    where += gq.result();
  }
  string q = "select ";
  if (dbi.getCountCursor()) {
    if (not sJoin.empty())
      q += string("count(distinct ") + gs.result() + ")";
    else
      q += "count(*)";
  }
  else {
    if (not sJoin.empty())
      q += "distinct ";
    q += gs.result();
  }
  q += " from ";
  q += tableName(obj, dbi);
  q += " mt ";
  q += sJoin;

  if (not where.empty())
    q += string(" where ") + where;
  q += ";";
  LOG(LM_INFO, "SQL: " << q);
//  throw runtime_error("weg");

  if (mysql_real_query(connection, q.c_str(), q.length()))
    throw mysql_exception(u8"query failed", connection);
  unsigned int sz = mysql_field_count(connection);

  MYSQL_RES *result;
  if (dbi.getCountCursor() or not sJoin.empty())
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
    size_t s = stoull(value, nullptr);
    return std::make_shared<CountCursor>(s);
  }

  auto cursor = std::make_shared<MariaCursor>(result, sz, dbi.getConnection(), dbi.database());
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
  MYSQL_FIELD *fields = mysql_fetch_fields(curs->result);
  if (not fields)
    throw runtime_error("Cursor read error");

  std::list<MemBaseVector *> detailVec;
  ReadSql rs(curs->result, curs->row, ConvObjToString());
  obj.clear();
  obj.traverse(rs);
  detailVec.splice(detailVec.end(), rs.detailVec, rs.detailVec.begin(), rs.detailVec.end());

  for (auto v:detailVec) {
    string dbn = vecTableName(*v, dbi);
    GenSql gs(GenerateSql::Fields, ConvObjToString());
    gs.arrayStructureMode = true;
    gs.startVec = v;
    v->traverse(gs);

    GenSql gq(GenerateSql::Query, ConvObjToString());
    gq.parentMode = true;
    gq.startVec = v;
    v->traverseKey(gq);
    string s = "select ";
    s += gs.result();
    s += " from ";
    s += dbn;
    s += " where ";
    s += gq.result();
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"create failed", connection);

    unsigned int sz = mysql_field_count(connection);
    MYSQL_RES *result;
    result = mysql_store_result(connection);
    if (result == nullptr or sz == 0)
      throw mysql_exception(u8"load detail failed", connection);
    for (;;) {
      MYSQL_ROW row = mysql_fetch_row(result);
      if (row == nullptr)
        break;
      unsigned long *lengths = mysql_fetch_lengths(result);
      string value(row[0], lengths[0]);
      size_t index = stoull(value, nullptr);
      if (index > 2000)
        throw runtime_error("Array with >2000 elements");
      size_t oldSz = v->size();
      if (index >= oldSz) {
        v->resize(index+1);
        for (size_t i = oldSz; i < index; i++)
          v->getObjInfo(index)->forceNull();
      }
      ObjectBase *currObj = v->getObjInfo(index);
      if (not currObj)
        throw runtime_error("invalid array");
      ReadSql rs(result, row, ConvObjToString(), 1);
      currObj->traverse(rs);
      detailVec.splice(detailVec.end(), rs.detailVec, rs.detailVec.begin(), rs.detailVec.end());
    }
  }
  LOG(LM_INFO, "RESULT " << obj.to_string());

}

void MariaDatabaseConnection::dropAll(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  std::list<const MemBaseVector *> detailVec;
  GenSql gs(GenerateSql::Create, ConvObjToString());
  obj.traverse(gs);
  detailVec.splice(detailVec.end(), gs.detailVec, gs.detailVec.begin(), gs.detailVec.end());
  for (auto v:detailVec) {
    GenSql gs(GenerateSql::Create, ConvObjToString());
    gs.parentMode = true;
    gs.arrayStructureMode = true;
    gs.startVec = v;
    v->traverse(gs);
    detailVec.splice(detailVec.end(), gs.detailVec, gs.detailVec.begin(), gs.detailVec.end());
    string s = "drop table if exists ";
    if (v->size() > 0) {
      MemVarCfg c = v->hasFeature(ColNameBase);
      if (c and v->parent())
        s += dbi.database() + "." + v->parent()->getConf(c);
      else
        s += MariaDatabaseConnection::tableName(*const_cast<MemBaseVector *>(v)->getObjInfo(0), dbi);
    } else
      throw runtime_error("TODO");
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"drop failed", connection);
  }

  string s = "drop table if exists ";
  s += tableName(obj, dbi);
  s += ";";
  LOG(LM_DEBUG, "SQL " << s);
  if (mysql_real_query(connection, s.c_str(), s.length()))
    throw mysql_exception(u8"drop failed", connection);
}

void MariaDatabaseConnection::structure(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  std::list<const MemBaseVector *> detailVec;
  GenSql gs(GenerateSql::Create, ConvObjToString());
  GenSql gk(GenerateSql::Keys, ConvObjToString());
  obj.traverseKey(gk);
  obj.traverse(gs);
  string s = "create table ";
  s += tableName(obj, dbi);
  s += "(";
  s += gs.result();
  s += ", unique(";
  s += gk.result();
  s += "));";
  detailVec.splice(detailVec.end(), gs.detailVec, gs.detailVec.begin(), gs.detailVec.end());
  for (auto v:detailVec) {
    GenSql gs(GenerateSql::Create, ConvObjToString());
    GenSql gk(GenerateSql::Keys, ConvObjToString());
    gs.parentMode = true;
    gs.startVec = v;
    gk.parentMode = true;
    gk.startVec = v;
    v->traverseKey(gk);
    v->traverse(gs);
    detailVec.splice(detailVec.end(), gs.detailVec, gs.detailVec.begin(), gs.detailVec.end());
    string s = "create table ";
    if (v->size() > 0) {
      MemVarCfg c = v->hasFeature(ColNameBase);
      if (c and v->parent())
        s += dbi.database() + "." + v->parent()->getConf(c);
      else
        s += MariaDatabaseConnection::tableName(*const_cast<MemBaseVector *>(v)->getObjInfo(0), dbi);
    }
    else
      throw runtime_error("TODO");
    s += "(";
    s += gs.result();
    s += ", unique(";
    s += gk.result();
    s += ",a_idx));";
    LOG(LM_DEBUG, "SQL " << s);
    if (mysql_real_query(connection, s.c_str(), s.length()))
      throw mysql_exception(u8"create failed", connection);
  }
  LOG(LM_DEBUG, "SQL " << s);
  if (mysql_real_query(connection, s.c_str(), s.length()))
    throw mysql_exception(u8"create failed", connection);
}

MYSQL *MariaDatabaseConnection::getConnection() {
  open();
  return connection;
}


}

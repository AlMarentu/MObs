// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2025 Matthias Lautner
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

#include "informix.h"
#include "logging.h"
#include "objgen.h"
#include "mchrono.h"
#include "helper.h"
#include "infxtools.h"

#include <esql/sqlca.h>
#include <esql/sqlhdr.h>
#include <esql/sqltypes.h>

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <utility>
#include <vector>
#include <chrono>

#ifndef SQLINFXBIGINT
#define SQLINFXBIGINT SQLBIGINT
#endif

namespace {
using namespace mobs;
using namespace std;

//const int ISAM_EXCLUSIVE = -106;

string getErrorMsg(int errNum) {
  string e = "SQL error:";
  e += std::to_string(errNum);
  e += ":";
  mint len;
  char buf[1024];
  int32_t e2 = rgetlmsg(int32_t(errNum), buf, sizeof(buf), &len);
  if (not e2) {
    e += string(buf, len);
    size_t pos = e.find("%s");
    if (pos != string::npos)
      e.replace(pos, 2, infx_error_msg2());

    int isam = infx_isam_or_serial();
    if (isam < 0) {
      e += " Isam:";
      e += std::to_string(isam);
      e += ":";
      if (rgetlmsg(int32_t(isam), buf, sizeof(buf), &len) == 0)
        e += string(buf, len);
    }
  }  else
    e += "infx error in getErrorMsg";
  return e;
}

// Der Formatstring für DateTime mit fraction ist Client-SDK anhängig.
// Da sich Informix spart, die Versionsnummern des SDK hochzuzählen, hier empirisch ermitteln
static const char *dtFmt = nullptr;

void setDateTimeFormat() {
  const char *s = "2001-01-01 01:00:00.00001";
  dtime_t dt;
  dt.dt_qual = TU_DTENCODE(TU_YEAR, TU_F5);
  dtFmt = "%Y-%m-%d %H:%M:%S%F5";
  LOG(LM_DEBUG, "TRY FMT " << dtFmt);
  int e = dtcvfmtasc(const_cast<char *>(s), (char *) dtFmt, &dt);
  if (e == 0)
    return;
  dtFmt = "%Y-%m-%d %H:%M:%S.%F5";
  LOG(LM_DEBUG, "TRY FMT " << dtFmt);
  e = dtcvfmtasc(const_cast<char *>(s), (char *) dtFmt, &dt);
  if (e == 0)
    return;
  THROW("can't convert to FRAC");
}

class informix_exception : public std::runtime_error {
public:
  informix_exception(const std::string &e, int errNum) : std::runtime_error(string("informix: ") + e + " " + getErrorMsg(errNum)) {
    LOG(LM_DEBUG, "Informix: " << getErrorMsg(errNum)); }
//  const char* what() const noexcept override { return error.c_str(); }
//private:
//  std::string error;
};

// Informix nutzt DBDATE, GL_DATE, USE_DTENV sowie LOCALE für das Datumsformat. Deswegen spezielle Routine
// z.B.: DBDATE=DMY4.
std::string formatDate(const struct tm &ts) {
  int4 i_date;
  int2 mdy[3]; // = { 12, 10, 2007 };
  mdy[0] = int2(ts.tm_mon + 1);
  mdy[1] = int2(ts.tm_mday);
  mdy[2] = int2(ts.tm_year + 1900);
  int e;
  if ((e = rmdyjul(mdy, &i_date)))
    throw informix_exception("Error formatting date ", e);
  char buf[20];
  if ((e = rdatestr(i_date, buf)))
    throw informix_exception("Error formatting date ", e);
  return buf;
}

class SQLInformixdescription : public mobs::SQLDBdescription {
public:
  explicit SQLInformixdescription(const string &dbName) : dbPrefix(dbName + ":") {
    changeTo_is_IfNull = false;
    createWith_IfNotExists = true;
    orderInSelect = true; // Es müssen alle Elemente des "order by" auch im "select" vorkommen
  }

  std::string tableName(const std::string &tabnam) override { return dbPrefix + tabnam;  }


  std::string createStmtIndex(std::string name) override {
    return "INT NOT NULL";
  }

  std::string createStmtText(const std::string &name, size_t len) override {
    return string(len > 255 ? u8"LVARCHAR(": u8"VARCHAR(") + std::to_string(len) + ")";
  }

  std::string createStmt(const MemberBase &mem, bool compact) override {
    std::stringstream res;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    mi.changeCompact(compact);
    if (mi.isTime and mi.granularity >= 86400000000)
      res << "DATE";
    else if (mi.isTime and mi.granularity >= 1000000)
      res << "DATETIME YEAR TO SECOND";
    else if (mi.isTime and mi.granularity >= 100000)
      res << "DATETIME YEAR TO FRACTION(1)";
    else if (mi.isTime and mi.granularity >= 10000)
      res << "DATETIME YEAR TO FRACTION(2)";
    else if (mi.isTime and mi.granularity >= 1000)
      res << "DATETIME YEAR TO FRACTION(3)";
    else if (mi.isTime and mi.granularity >= 100)
      res << "DATETIME YEAR TO FRACTION(4)";
    else if (mi.isTime)
      res << "DATETIME YEAR TO FRACTION(5)";
    else if (mi.isUnsigned and mi.max == 1)
      res << "BOOLEAN"; //"SMALLINT";
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
        else if (n <= 255)
          res << "VARCHAR(" << n << ")";
        else
          res << "LVARCHAR(" << n << ")";
      }
    }
    else if (mi.isSigned and mi.max <= INT16_MAX)
      res << "SMALLINT";
    else if (mi.isSigned and mi.max <= INT32_MAX)
      res << "INT";
    else if (mi.isSigned or mi.isUnsigned) // uint64_t is not supported
      res << "BIGINT";
    else
      res << "SMALLINT";
    if (not mem.nullAllowed())
      res << " NOT NULL";
    return res.str();
  }

  void startWriting() override
  {
    LOG(LM_DEBUG, "START");
    fldCnt = 0;
    pos = 0;
  }

  void finishWriting() override {
    LOG(LM_DEBUG, "FINISH");
  }

  std::string valueStmtIndex(size_t i) override {
    LOG(LM_DEBUG, "Informix SqlVar index: " << fldCnt << "=" << i);
    ifx_sqlvar_t &sql_var = descriptor->sqlvar[fldCnt++];
    memset(&sql_var, 0, sizeof(ifx_sqlvar_t));
    descriptor->sqld = int2(fldCnt);
    sql_var.sqltype = SQLINT;
    setBuffer(sql_var);
    *(int32_t *)(sql_var.sqldata) = int32_t(i);
    return "?";
  }

  std::string valueStmtText(const std::string &tx, bool isNull) override {
    LOG(LM_DEBUG, "Informix SqlVar DBJSON: " << tx);
    if (not descriptor or not buf)
      return isNull ? string("null"):mobs::to_quote(tx);
    ifx_sqlvar_t &sql_var = descriptor->sqlvar[fldCnt++];
    memset(&sql_var, 0, sizeof(ifx_sqlvar_t));
    descriptor->sqld = int2(fldCnt);
    sql_var.sqltype = SQLVCHAR;
    unsigned int sz = static_cast<unsigned int>(tx.length()) + 1;
    if (tx.empty()) {
      sql_var.sqltype = SQLCHAR;
      sz = 2;
    } else if (tx.length() >= 255) {
      sql_var.sqltype = SQLLVARCHAR;
    }
    setBuffer(sql_var, sz);
    if (isNull)
      rsetnull(sql_var.sqltype, sql_var.sqldata);
    else if (tx.empty())
      stcopy(const_cast<char *>(" "), sql_var.sqldata);
    else
      stcopy(const_cast<char *>(tx.c_str()), sql_var.sqldata);

    return "?";
  }

  void setBuffer(ifx_sqlvar_t &sql_var, unsigned int sz = 0) {
    pos = (short)rtypalign(pos, sql_var.sqltype);
    sql_var.sqldata = buf + pos;
    sql_var.sqllen = sz;
    mint size = rtypmsize(sql_var.sqltype, sql_var.sqllen);
    pos += size;
    if (sql_var.sqllen <= 0) {
      sql_var.sqllen = size;
      if (sql_var.sqllen <= 0)
        throw runtime_error("error in setBuffer");
    }
  }
  std::string memInfoStmt(const MobsMemberInfoDb &mi) override {
    if (mi.isTime and mi.granularity >= 86400000000) { // nur Datum
      std::stringstream s;
      struct tm ts{};
      mi.toLocalTime(ts);
      s << formatDate(ts);
      return mobs::to_squote(s.str());
    }
    else if (mi.isTime) {
      MTime t;
      if (not from_number(mi.t64, t))
        throw std::runtime_error("Time Conversion");
      MTimeFract f = mobs::MSecond;
      if (mi.granularity < 100)
        f = mobs::MF5;
      else if (mi.granularity < 1000)
        f = mobs::MF4;
      else if (mi.granularity < 10000)
        f = mobs::MF3;
      else if (mi.granularity < 100000)
        f = mobs::MF2;
      else if (mi.granularity < 1000000)
        f = mobs::MF1;
      return mobs::to_squote(to_string_ansi(t, f));
    }
    else if (mi.isUnsigned and mi.max == 1) // bool
      return (mi.u64 ? "'t'" : "'f'");

    bool needQuotes;
    std::string r = mi.toString(&needQuotes);
    if (needQuotes)
      return to_squote(r);
    return r;
  }
  std::string valueStmt(const MemberBase &mem, bool compact, bool increment, bool inWhere) override {
    MobsMemberInfo mi;
    mem.memInfo(mi);
    mi.changeCompact(compact);
    if (not descriptor or not buf)
    {
      if (mem.isNull())
        return u8"null";
      if (mi.isTime and mi.granularity >= 86400000000) { // nur Datum
        std::stringstream s;
        struct tm ts{};
        mi.toLocalTime(ts);
        s << formatDate(ts);
        return mobs::to_squote(s.str());
      }
      else if (mi.isTime) {
        MTime t;
        if (not from_number(mi.t64, t))
          throw std::runtime_error("Time Conversion");
        MTimeFract f = mobs::MSecond;
        if (mi.granularity < 100)
          f = mobs::MF5;
        else if (mi.granularity < 1000)
          f = mobs::MF4;
        else if (mi.granularity < 10000)
          f = mobs::MF3;
        else if (mi.granularity < 100000)
          f = mobs::MF2;
        else if (mi.granularity < 1000000)
          f = mobs::MF1;
        return mobs::to_squote(to_string_ansi(t, f));
      }
      else if (mi.isUnsigned and mi.max == 1) // bool
        return (mi.u64 ? "'t'" : "'f'");
      else if (mem.is_chartype(mobs::ConvToStrHint(compact)))
        return mobs::to_squote(mem.toStr(mobs::ConvToStrHint(compact)));

      return mem.toStr(mobs::ConvToStrHint(compact));
    }
    ifx_sqlvar_t &sql_var = descriptor->sqlvar[fldCnt++];
    memset(&sql_var, 0, sizeof(ifx_sqlvar_t));
    descriptor->sqld = int2(fldCnt);
    int e = 0;
    if (mi.isTime and mi.granularity >= 86400000000) { // nur Datum
      struct tm ts{};
      mi.toLocalTime(ts);
      LOG(LM_DEBUG, "Informix SqlVar " << mem.getElementName() << ": " << fldCnt - 1 << "=" << std::put_time(&ts, "%F"));
      sql_var.sqltype = SQLDATE;
      setBuffer(sql_var);
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
      else {
        int2 mdy[3]; //  { 12, 21, 2007 };
        mdy[0] = int2(ts.tm_mon + 1);
        mdy[1] = int2(ts.tm_mday);
        mdy[2] = int2(ts.tm_year + 1900);
        e = rmdyjul(mdy, (int4 *) sql_var.sqldata);
      }
    }
    else if (mi.isTime) {
      MTime t;
      if (not from_number(mi.t64, t))
        throw std::runtime_error("Time Conversion");
      std::string s = to_string_ansi(t, mobs::MF5);
      LOG(LM_DEBUG, "Informix SqlVar " << mem.getElementName() << ": " << fldCnt - 1 << "=" << s);
      sql_var.sqltype = SQLDTIME;
      setBuffer(sql_var);
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
      else {
        auto dtp = (dtime_t *) sql_var.sqldata;
        dtp->dt_qual = TU_DTENCODE(TU_YEAR, TU_F5);
        if (not dtFmt)
          setDateTimeFormat();
        e = dtcvfmtasc(const_cast<char *>(s.c_str()), (char *) dtFmt, dtp);
      }
    } else if (mi.isUnsigned) {
      if (increment) {
        if (mi.u64 == mi.max)
          throw std::runtime_error("VersionElement overflow");
        if (mem.isNull())
          throw std::runtime_error("VersionElement is null");
        mi.u64++;
      }
      LOG(LM_DEBUG, "Informix SqlVar " << mem.getElementName() << ": " << fldCnt - 1 << "=" << mi.u64);
      if (mi.max > INT32_MAX) {
        if (mi.u64 > INT64_MAX)
          throw std::runtime_error("Number to big");
        sql_var.sqltype = SQLINFXBIGINT;
//        sql_var.sqltype = SQLINT8;
        setBuffer(sql_var);
        *(int64_t *) (sql_var.sqldata) = mi.u64;
//        biginttoifx_int8(mi.i64, (ifx_int8*)(sql_var.sqldata));
      } else {
        sql_var.sqltype = SQLINT;
        setBuffer(sql_var);
        *(int *) (sql_var.sqldata) = mi.u64;
      }
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
    } else if (mi.isSigned) {
      if (increment) {
        if (mi.i64 == mi.max)
          throw std::runtime_error("VersionElement overflow");
        if (mem.isNull())
          throw std::runtime_error("VersionElement is null");
        mi.i64++;
      }
      LOG(LM_DEBUG, "Informix SqlVar " << mem.getElementName() << ": " << fldCnt - 1 << "=" << mi.i64);
      if (mi.max > INT32_MAX) {
        sql_var.sqltype = SQLINFXBIGINT;
        setBuffer(sql_var);
        *(int64_t *) (sql_var.sqldata) = mi.i64;
      } else {
        sql_var.sqltype = SQLINT;
        setBuffer(sql_var);
        *(int *) (sql_var.sqldata) = mi.i64;
      }
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);

    } else if (mi.isFloat) {
      LOG(LM_DEBUG, "Informix SqlVar " << mem.getElementName() << ": " << fldCnt - 1 << "=" << mi.d);
      sql_var.sqltype = SQLFLOAT;
      setBuffer(sql_var);
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
      else
        *(double *) (sql_var.sqldata) = mi.d;
//    } else if (mi.isBlob) {
//      LOG(LM_DEBUG, "Informix SqlVar " << mem.name() << ": " << fldCnt - 1 << " BLOB");
//      sql_var.sqltype = SQLBYTES;
//      setBuffer(sql_var);
//      LOG(LM_INFO, "SIZE " << sql_var.sqllen << " " << sizeof(loc_t));
//      loc_t &blob = *(loc_t *) (sql_var.sqldata);
//      memset(&blob, 0, sizeof(loc_t));
//      blob.loc_loctype = LOCFNAME;      /* blob is named file */
//      blob.loc_fname = "/tmp/otto";        /* here is its name */
//      blob.loc_oflags = LOC_RONLY;      /* contents are to be read by engine */
//      blob.loc_size = -1;               /* read to end of file */
//      blob.loc_indicator = 0;   /* not a null blob */

//      ifx_lo_t &blob = *(ifx_lo_t *) (sql_var.sqldata);
//      ifx_lo_create_spec_t *lo_spec;
//      e = ifx_lo_def_create_spec(&lo_spec);
//      // TODO ifx_lo_spec_free
//      if (e)
//        throw informix_exception("Conversion error LO_Spec", e);
//      mint lo_fd = ifx_lo_create(lo_spec, LO_WRONLY, &blob, &e);
//      if (e)
//        throw informix_exception("Conversion error LO_Spec", e);
//      mint res = ifx_lo_write(lo_fd, (char *)reinterpret_cast<const u_char *>(&v[0]), v.size(), &e);
//      LOG(LM_INFO, "RES = " << res);

    } else {
      std::string s = mem.toStr(mobs::ConvToStrHint(compact));
      LOG(LM_DEBUG, "Informix SqlVar " << mem.getElementName() << ": " << fldCnt - 1 << "=" << s);
      if (increment)
        throw std::runtime_error("VersionElement is not int");
      // leerstring entspricht NULL daher SQLCHAR mit 1 SPC als leer mit not null
      sql_var.sqltype = SQLVCHAR;
      if (s.empty()) {
        sql_var.sqltype = SQLCHAR;
        s = " ";
      } else if (s.length() >= 2) {
        sql_var.sqltype = SQLLVARCHAR;
      }
      // Längentest
      mobs::MemVarCfg c = mem.hasFeature(mobs::LengthBase);
      if (c and s.length() > (c - mobs::LengthBase))
        throw runtime_error(u8"SQL: content to big für column " +
        mem.getName(mobs::ConvObjToString().exportPrefix().exportAltNames()) + " need " + std::to_string(s.length()));
      setBuffer(sql_var, s.length() + 1);
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
      else
        stcopy(const_cast<char *>(s.c_str()), sql_var.sqldata);
    }
    if (e)
      throw informix_exception("Conversion error Date", e);

    return "?";
  }

  void readValue(MemberBase &mem, bool compact) override {
    if (pos >= fldCnt)
      throw runtime_error(u8"Result not found " + mem.getElementName());
    auto &col = descriptor->sqlvar[pos++];
    LOG(LM_DEBUG, "Read " << mem.getElementName() << " " << col.sqlname << " " << col.sqllen << " " << rtypname(col.sqltype));

    if (risnull(col.sqltype, col.sqldata)) {
      mem.forceNull();
      return;
    }
    bool ok = true;
    int e = 0;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    mi.changeCompact(compact);
    switch (col.sqltype) {
      case SQLCHAR:
      case SQLNCHAR:
        // strip trailing blanks
        for (char *cp = col.sqldata + col.sqllen - 2; *cp == ' ' and cp >= col.sqldata; --cp)
          *cp = '\0';
        __attribute__ ((fallthrough));
      case SQLLVARCHAR:
      case SQLNVCHAR:
      case SQLVCHAR:
        ok = mem.fromStr(string(col.sqldata),
                         not compact ? ConvFromStrHint::convFromStrHintExplizit : ConvFromStrHint::convFromStrHintDflt);
        if (not ok)
          throw runtime_error(u8"conversion error in " + mem.getElementName() + " Value=" + string(col.sqldata));
        return;
      case SQLDATE: {
        short mdy[3]; //  { 12, 21, 2007 };
        e = rjulmdy(*(int4 *) col.sqldata, mdy);
        if (e)
          throw informix_exception(u8"Date Conversion", e);
        std::tm ts = {};
        ts.tm_mon = mdy[0] - 1;
        ts.tm_mday = mdy[1];
        ts.tm_year = mdy[2] - 1900;
        LOG(LM_INFO, "DATE " << mdy[1] << '.' << mdy[0] << '.' << mdy[2]);
        if (mi.isTime)
          mi.fromLocalTime(ts);
        else
          ok = false;
        break;
      }
      case SQLDTIME: {
        if (not dtFmt)
          setDateTimeFormat();
        char timebuf[32];
        e = dttofmtasc((dtime_t *) col.sqldata, timebuf, sizeof(timebuf), (char *) dtFmt);
        if (e)
          throw informix_exception(u8"DateTime Conversion", e);
        LOG(LM_INFO, "DATETIME " << timebuf);
        MTime t;
        ok = string2x(string(timebuf), t) and mi.isTime;
        mi.setTime(t.time_since_epoch().count());
        break;
      }
      case SQLBOOL:
        mi.setInt(*(int8_t *) (col.sqldata));
        break;
      case SQLSMINT:
        mi.setInt(*(int16_t *) (col.sqldata));
        break;
      case SQLINT:
      case SQLSERIAL:
        mi.setInt(*(int32_t *) (col.sqldata));
        break;
      case SQLINFXBIGINT:
      case SQLBIGSERIAL:
        mi.setInt(*(int64_t *) (col.sqldata));
        break;
      case SQLSERIAL8:
      case SQLINT8: {
        bigint i;
        e = bigintcvifx_int8((const ifx_int8_t *) (col.sqldata), &i);
        mi.setInt(i);
        if (e)
          throw informix_exception(u8"INT8 Conversion", e);
        break;
      }
      case SQLFLOAT:
        if (mi.isFloat)
          mi.d = *(double *) (col.sqldata);
        else
          ok = false;
        break;
//      case SQLUDTFIXED:
//        if (mi.isBlob) {
//          ifx_loc_t &lo = *(loc_t *) (col.sqldata);
//          LOG(LM_INFO, "BBBBB " << lo.loc_size << " " << lo.loc_loctype);
//
//
//        }
//        else
//          ok = false;
//        break;
      case SQLBYTES:
      default:
        throw runtime_error(u8"conversion error in " + mem.getElementName() + " Type=" + to_string(col.sqltype));
    }
//    if (mi.isUnsigned and mi.max == 1) // bool
//      ok = mem.fromUInt64(res == 0 ? 0 : 1);
//    else
    if (ok)
      ok = mem.fromMemInfo(mi);

    if (not ok)
      throw runtime_error(u8"conversion error in " + mem.getElementName());
    // TODO konvertierung über string versuchen
  }

  void readValueText(const std::string &name, std::string &text, bool &null) override {
    if (pos >= fldCnt)
      throw runtime_error(u8"Result not found " + name);
    auto &col = descriptor->sqlvar[pos++];
    LOG(LM_DEBUG, "Read " << name << " " << col.sqlname << " " << col.sqllen << " " << rtypname(col.sqltype));

    if (risnull(col.sqltype, col.sqldata)) {
      null = true;
      return;
    }
    null = false;

    switch (col.sqltype) {
      case SQLCHAR:
      case SQLNCHAR:
        // strip trailing blanks
        for (char *cp = col.sqldata + col.sqllen - 2; *cp == ' ' and cp >= col.sqldata; --cp)
          *cp = '\0';
        __attribute__ ((fallthrough));
      case SQLLVARCHAR:
      case SQLNVCHAR:
      case SQLVCHAR:
        text = string(col.sqldata);
        return;
      case SQLTEXT:
      default:
        throw runtime_error(u8"conversion error in " + name + " Type=" + to_string(col.sqltype));
    }
  }

  size_t readIndexValue(const std::string &name) override {
    if (pos >= fldCnt)
      throw runtime_error(u8"Result not found index");
    auto &col = descriptor->sqlvar[pos++];
    LOG(LM_DEBUG, "Read idx " << name << " " << col.sqlname << " " << col.sqllen);

    if (risnull(col.sqltype, col.sqldata))
      throw runtime_error(u8"index value is null");

    switch (col.sqltype) {
      case SQLSMINT:
        return *(int16_t *) (col.sqldata);
      case SQLINT:
        return *(int32_t *) (col.sqldata);
      case SQLINFXBIGINT:
        return *(int64_t *) (col.sqldata);
      case SQLINT8:
      {
        bigint i;
        int e = bigintcvifx_int8((const ifx_int8_t *) (col.sqldata), &i);
        if (e == 0)
          throw informix_exception(u8"INT8 Conversion", e);
      }
    }
    throw runtime_error(u8"index value is not integer");
  }

  void startReading() override {
    pos = 0;
  }
  void finishReading() override {}

  int fldCnt = 0;
  struct sqlda *descriptor = nullptr;
  char * buf = nullptr; //

private:
  std::string dbPrefix;
  int pos = 0;
};




class CountCursor : public virtual mobs::DbCursor {
  friend class mobs::InformixDatabaseConnection;
public:
  explicit CountCursor(size_t size) { cnt = size; }
  ~CountCursor() override = default;;
  bool eof() override  { return true; }
  bool valid() override { return false; }
  void operator++() override { }
};




class InformixCursor : public virtual mobs::DbCursor {
  friend class mobs::InformixDatabaseConnection;
public:
  explicit InformixCursor(int conNr, std::shared_ptr<DatabaseConnection> dbi,
                       std::string dbName, bool keysOnly) :
          dbCon(std::move(dbi)), databaseName(std::move(dbName)), isKeysOnly(keysOnly), m_conNr(conNr) {
    static int n = 0;
    m_cursNr = ++n;
    buf[0] = '\0';
  }
  ~InformixCursor() override {
    if (descPtr)
      close();
  }
  bool eof() override  { return not descPtr; }
  bool valid() override { return not eof(); }
  bool keysOnly() const override { return isKeysOnly; }
  void operator++() override {
    const int NOMOREROWS=100;
    if (eof())
      return;
    string c = "curs";
    c += std::to_string(m_cursNr);
    LOG(LM_DEBUG, "SQL fetch " << c);
    int e = infx_fetch(c.c_str(), descPtr);
    if (e) {
      close();
      if (e == NOMOREROWS)
        return;
      throw informix_exception(u8"cursor: query row failed", e);
    }
    cnt++;
  }
private:
  void open(const string &stmt) {
    const int NOMOREROWS=100;
    string c = "curs";
    c += std::to_string(m_cursNr);
    string p = "prep";
    p += std::to_string(m_cursNr);
    LOG(LM_DEBUG, "SQL declare " << c << " cursor");
    int e = infx_query(stmt.c_str(), c.c_str(), p.c_str(), &descPtr);
    if (e)
      throw informix_exception(u8"cursor: query row failed", e);
    fldCnt = descPtr->sqld;
    LOG(LM_INFO, "Anz Fields " << fldCnt);

    mint cnt = 0;
    mint pos = 0;
    for(struct sqlvar_struct *col_ptr = descPtr->sqlvar; cnt < fldCnt; cnt++, col_ptr++)
    {
      LOG(LM_INFO, "COL " << cnt << col_ptr->sqltype << " " << col_ptr->sqlname);

      /* Allow for the trailing null character in C character arrays */
      switch (col_ptr->sqltype) {
        case SQLCHAR:
        case SQLNCHAR:
        case SQLNVCHAR:
        case SQLVCHAR:
          col_ptr->sqllen += 1;
          break;
      }

      /* Get next word boundary for column data and assign buffer position to sqldata */
      pos = rtypalign(pos, col_ptr->sqltype);
      col_ptr->sqldata = &buf[pos];

      /* Determine size used by column data and increment buffer position */
      mint size = rtypmsize(col_ptr->sqltype, col_ptr->sqllen);
      pos += size;
      if (pos > mint(sizeof(buf)))
        throw runtime_error(u8"informix Buffer overflow");
    }
    LOG(LM_DEBUG, "SQL open " << c);
    e = infx_open_curs(c.c_str());
    if (e)
      throw informix_exception(u8"cursor: open cursor failed", e);
    LOG(LM_DEBUG, "SQL fetch " << c);
    e = infx_fetch(c.c_str(), descPtr);
    if (e) {
      close();
      if (e != NOMOREROWS)
        throw informix_exception(u8"cursor: query row failed", e);
    }
  }
  void close() {
    string c = "curs";
    c += std::to_string(m_cursNr);
    string p = "prep";
    p += std::to_string(m_cursNr);
    LOG(LM_DEBUG, "SQL close " << c);
    infx_remove_curs(c.c_str(), p.c_str());
    free(descPtr);
    descPtr = nullptr;
  }
  std::shared_ptr<DatabaseConnection> dbCon;  // verhindert das Zerstören der Connection
  std::string databaseName;  // unused
  bool isKeysOnly;
  int m_conNr;
  int m_cursNr = 0;
  int fldCnt = 0;
  struct sqlda *descPtr = nullptr;
  char buf[32768];
};

}

namespace mobs {

InformixDatabaseConnection::~InformixDatabaseConnection() {
  if (conNr > 0)
    infx_disconnect(conNr);
}


std::string InformixDatabaseConnection::tableName(const ObjectBase &obj, const DatabaseInterface &dbi) {
  MemVarCfg c = obj.hasFeature(ColNameBase);
  if (c)
    return dbi.database() + "." + obj.getConf(c);
  return dbi.database() + "." + obj.getObjectName();
}

void InformixDatabaseConnection::open() {
  const int DBLOCALEMISMATCH=-23197;
  if (conNr > 0) {
    infx_set_connection(conNr);
    return;
  }
  size_t pos = m_url.find("//");
  if (pos == string::npos)
    throw runtime_error("informix: error in url");
  size_t pos2 = m_url.find(':', pos);
  string host;
  if (pos2 == string::npos)
    pos2 = m_url.length();

  host = m_url.substr(pos+2, pos2-pos-2);
  const char *dblocale[] = {"de_DE.UTF8", "de_DE.8859-1", nullptr};
  string db = m_database;
  db += "@";
  db += host;
  conNr = infx_connect(db.c_str(), m_user.c_str(), m_password.c_str());
  const char **nextLoc = dblocale;
  while  (conNr == DBLOCALEMISMATCH and *nextLoc) {
    LOG(LM_DEBUG, "infx Locale invalid, try " << *nextLoc);
    setenv("DB_LOCALE", *nextLoc++, 1);
    conNr = infx_connect(db.c_str(), m_user.c_str(), m_password.c_str());
  }
  LOG(LM_DEBUG, "Informix connecting to " << db << " NR = " << conNr);
  if (conNr > 0)
    return;
  if (conNr < 0)
    throw informix_exception(u8"open failed", conNr);
  throw runtime_error("informix: error connecting to db");
}

bool InformixDatabaseConnection::load(DatabaseInterface &dbi, ObjectBase &obj) {
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  string s = gsql.selectStatementFirst();
  LOG(LM_DEBUG, "SQL: " << s);
  auto cursor = std::make_shared<InformixCursor>(conNr, dbi.getConnection(), dbi.database(), false);
  cursor->open(s);
  if (cursor->eof()) {
    LOG(LM_DEBUG, "NOW ROWS FOUND");
    return false;
  }

  retrieve(dbi, obj, cursor);
  return true;
}

void InformixDatabaseConnection::save(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  ifx_sqlda_t descriptor{};
  ifx_sqlvar_t sql_var[100];
  char buffer[32768];
  descriptor.sqlvar = sql_var;
  sd.descriptor = &descriptor;
  sd.buf = buffer;

  // Transaktion benutzen zwecks Atomizität
  if (currentTransaction == nullptr) {
    string s = "BEGIN WORK;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_exec_desc(s.c_str(), &descriptor);
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    // Wenn DBI mit Transaktion, dann in Transaktion bleiben
  }
  else if (currentTransaction != dbi.getTransaction())
    throw std::runtime_error("transaction mismatch");
  else {
    string s = "SAVEPOINT MOBS;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_exec_desc(s.c_str(), &descriptor);
    if (e)
      throw informix_exception(u8"Transaction failed", e);
  }
  int64_t version = gsql.getVersion();
  LOG(LM_DEBUG, "VERSION IS " << version);

  bool insertOnly = version == 0;
  bool updateOnly = version > 0;
  try {
    string s;
    string upd;
    if (insertOnly)
      s = gsql.insertStatement(true);
    else if (updateOnly)
      s = gsql.updateStatement(true);
    else {// bei unsicher (-1) immer erst update probieren
      s = gsql.insertUpdStatement(true, upd);
      if (not upd.empty()) s.swap(upd); // failed update is faster then insert
    }
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_exec_desc(s.c_str(), &descriptor);
    int rows = infx_processed_rows();
    if (not e and not insertOnly and not updateOnly and rows == 0 and not upd.empty()) {  // try insert
      LOG(LM_DEBUG, "SQL " << upd);
      e = infx_exec_desc(upd.c_str(), &descriptor);
      insertOnly = true;
    }
    if (e)
      throw informix_exception(u8"save failed", e);
    if (version > 0 and rows != 1)
      throw runtime_error(u8"number of processed rows is " + to_string(infx_processed_rows()) + " should be 1");

    while (not gsql.eof()) {
      if (insertOnly)
        s = gsql.insertStatement(false);
      else {
        s = gsql.insertUpdStatement(false, upd);
        if (not upd.empty()) s.swap(upd);
      }
      LOG(LM_DEBUG, "SQL " << s);
      e = infx_exec_desc(s.c_str(), &descriptor);
      int rows = infx_processed_rows();
      if (not insertOnly and not e and rows == 0 and not upd.empty()) {  // try insert
        LOG(LM_DEBUG, "SQL " << upd);
        e = infx_exec_desc(upd.c_str(), &descriptor);
      }
      if (e)
        throw informix_exception(u8"save failed", e);
    }

  } catch (runtime_error &exc) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    throw exc;
  } catch (exception &exc) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    throw exc;
  }

  string s;
  if (currentTransaction)
    s = "RELEASE SAVEPOINT MOBS;";
  else
    s = "COMMIT WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  int e = infx_execute(s.c_str());
  if (e)
    throw informix_exception(u8"Transaction failed", e);
}


bool InformixDatabaseConnection::destroy(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  ifx_sqlda_t descriptor{};
  ifx_sqlvar_t sql_var[100];
  char buffer[8096];
  descriptor.sqlvar = sql_var;
  sd.descriptor = &descriptor;
  sd.buf = buffer;
  // Transaktion benutzen zwecks Atomizität
  if (currentTransaction == nullptr) {
    string s = "BEGIN WORK;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    // Wenn DBI mit Transaktion, dann in Transaktion bleiben
  }
  else if (currentTransaction != dbi.getTransaction())
    throw std::runtime_error("transaction mismatch");
  else {
    string s = "SAVEPOINT MOBS;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
  }
  int64_t version = gsql.getVersion();
  LOG(LM_DEBUG, "VERSION IS " << version);

  bool found = false;
  try {
    for (bool first = true; first or not gsql.eof(); first = false) {
      string s = gsql.deleteStatement(first);
      LOG(LM_DEBUG, "SQL " << s);
      int e = infx_exec_desc(s.c_str(), &descriptor);
      if (e)
        throw informix_exception(u8"destroy failed", e);
      if (first) {
        found = (infx_processed_rows() > 0);
        if (version > 0 and not found)
          throw runtime_error(u8"destroy: Object with appropriate version not found");
      }
    }
  } catch (runtime_error &exc) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    throw exc;
  } catch (exception &exc) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    throw exc;
  }

  string s;
  if (currentTransaction)
    s = "RELEASE SAVEPOINT MOBS;";
  else
    s = "COMMIT WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  int e = infx_execute(s.c_str());
  if (e)
    throw informix_exception(u8"Transaction failed", e);

  return found;
}

void InformixDatabaseConnection::dropAll(DatabaseInterface &dbi, const ObjectBase &obj) {
  const int EXISTSNOT = -206;
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  for (bool first = true; first or not gsql.eof(); first = false) {
    string s = gsql.dropStatement(first);
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e and e != EXISTSNOT)
      throw informix_exception(u8"dropAll failed", e);
  }
}

void InformixDatabaseConnection::structure(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  for (bool first = true; first or not gsql.eof(); first = false) {
    string s = gsql.createStatement(first);
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"create failed", e);
  }
}

std::shared_ptr<DbCursor>
InformixDatabaseConnection::query(DatabaseInterface &dbi, ObjectBase &obj, bool qbe, const QueryGenerator *query,
                                  const QueryOrder *sort) {
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  string sqlLimit;
  if (not dbi.getCountCursor() and dbi.getQuerySkip() > 0)
    sqlLimit += STRSTR(" SKIP " << dbi.getQuerySkip());
  if (not dbi.getCountCursor() and dbi.getQueryLimit() > 0)
    sqlLimit += STRSTR(" LIMIT " << dbi.getQueryLimit());

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
  if (dbi.getCountCursor()) {
    long cnt = 0;
    int e = infx_count(s.c_str(), &cnt);
    if (e)
      throw informix_exception(u8"dropAll failed", e);
    return std::make_shared<CountCursor>(cnt);
  }

  auto cursor = std::make_shared<InformixCursor>(conNr, dbi.getConnection(), dbi.database(), dbi.getKeysOnly());
  cursor->open(s);
  if (cursor->eof()) {
    LOG(LM_DEBUG, "NOW ROWS FOUND");
  }
  return cursor;
}

void
InformixDatabaseConnection::retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) {
  auto curs = std::dynamic_pointer_cast<InformixCursor>(cursor);
  if (not curs)
    throw runtime_error("InformixDatabaseConnection: invalid cursor");

  if (not curs->descPtr) {
    throw runtime_error("Cursor eof");
  }
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  obj.clear();
  sd.descriptor = curs->descPtr;
  sd.fldCnt = curs->fldCnt;
  if (curs->isKeysOnly)
    gsql.readObjectKeys(obj);
  else
    gsql.readObject(obj);

  while (not gsql.eof()) {
    SqlGenerator::DetailInfo di;
    string s = gsql.selectStatementArray(di);
    LOG(LM_DEBUG, "SQL " << s);
    auto curs2 = std::make_shared<InformixCursor>(conNr, dbi.getConnection(), dbi.database(), false);
    curs2->open(s);
    sd.descriptor = curs2->descPtr;
    sd.fldCnt = curs2->fldCnt;
    // Vektor auf leer setzten (wurde wegen Struktur zuvor erweitert)
    di.vecNc->resize(0);
    while (not curs2->eof()) {
      gsql.readObject(di);
      curs2->next();
    }
  }

  LOG(LM_DEBUG, "RESULT " << obj.to_string());
}

void InformixDatabaseConnection::startTransaction(DatabaseInterface &dbi, DbTransaction *transaction,
                                                  shared_ptr<TransactionDbInfo> &tdb) {
  open();
  if (currentTransaction == nullptr) {
    string s = "BEGIN WORK;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    currentTransaction = transaction;
  }
  else if (currentTransaction != transaction)
    throw std::runtime_error("transaction mismatch"); // hier geht nur eine Transaktion gleichzeitig
}

void InformixDatabaseConnection::endTransaction(DbTransaction *transaction, shared_ptr<TransactionDbInfo> &tdb) {
  if (currentTransaction == nullptr)
    return;
  else if (currentTransaction != transaction)
    throw std::runtime_error("transaction mismatch");
  string s = "COMMIT WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  int e = infx_execute(s.c_str());
  if (e)
    throw informix_exception(u8"Transaction failed", e);
  currentTransaction = nullptr;
}

void InformixDatabaseConnection::rollbackTransaction(DbTransaction *transaction, shared_ptr<TransactionDbInfo> &tdb) {
  if (currentTransaction == nullptr)
    return;
  string s = "ROLLBACK WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  int e = infx_execute(s.c_str());
  if (e)
    throw informix_exception(u8"Transaction failed", e);
  currentTransaction = nullptr;
}

size_t InformixDatabaseConnection::doSql(const string &sql) {
  LOG(LM_DEBUG, "SQL " << sql);
  open();
  int e = infx_execute(sql.c_str());
  if (e)
    throw informix_exception(u8"doSql " + sql + ": ", e);
  return infx_processed_rows();
}

size_t InformixDatabaseConnection::maxAuditChangesValueSize(const DatabaseInterface &dbi) const {
  return 200;
}


}

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

// TODO dirtyRead, TransaktionsLevel
// TODO Transaktionen (benötigt sharded Installation)
// TODO Erkennung ob Datanbank shardet (also über mongos)
// TODO Passwort und User aus ConnectionObjekt übernehmen

#include "mongo.h"

#include "dbifc.h"
#include "logging.h"
#include "objgen.h"
#include "unixtime.h"
#include "helper.h"
#include "querygenerator.h"
#include "mchrono.h"

#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>
#include <chrono>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>

using namespace bsoncxx;

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::sub_document;
using bsoncxx::builder::basic::sub_array;
using bsoncxx::builder::basic::make_document;

namespace {
using namespace mobs;
using namespace std;

class BsonElements : virtual public mobs::ElementNames {
public:
  explicit BsonElements(mobs::ConvObjToString c) : mobs::ElementNames(c.exportAltNames()) { };

  void orderStmt(const std::string &name, int direction) override {
    doc.append(kvp(name, direction));
  }

  void valueStmt(const std::string &name, const mobs::MemberBase &mem, bool compact) override {
    if (index)
    {
      doc.append(kvp(name, 1));
      return;
    }

    MobsMemberInfo mi;
    mem.memInfo(mi);
    mi.changeCompact(compact);
    int32_t i32;
    int64_t i64 = mi.i64;
    bsoncxx::decimal128 i128;
    std::chrono::system_clock::time_point tp{};
    types::b_binary bin{};
    bin.size = 0;
    bin.bytes = nullptr;
    bool useChar = mem.is_chartype(mobs::ConvToStrHint(compact));
    bool useBool = false;
    bool use32 = false;
    bool use64 = false;
    bool use128 = false;
    if (mi.isSigned)
    {
      if (mi.max <= std::numeric_limits<int32_t>::max() and mi.min >= std::numeric_limits<int32_t>::min())
      {
        i32 = (int32_t)i64;
        use32 = true;
      }
      else
        use64 = true;
    }
    else if (mi.isUnsigned)
    {
      if (mi.max == 1) {
        i64 = (int64_t)mi.u64;
        useBool = true;
      }
      else if (mi.max <= std::numeric_limits<int32_t>::max()) {
        i32 = (int32_t)mi.u64;
        use32 = true;
      }
      else if (mi.max <= std::numeric_limits<int64_t>::max() or mi.i64 <= std::numeric_limits<int64_t>::max()) {
        i64 = (int64_t)mi.u64;
        use64 = true;
      }
      else // Typ 128 nur nehmen, wenn Zahl nicht in int64 passt
      {
        i128 = bsoncxx::decimal128(0, mi.u64);
        use128 = true;
      }
    }
    else if (mi.isTime)
    {
      tp += std::chrono::microseconds(mi.t64);
    }
    else if (mi.isBlob)
    {
      bin.size = int32_t(mi.u64);
      bin.bytes = (uint8_t *)mi.blob;
      useChar = false;
    }

    if (mem.isNull())
      doc.append(kvp(name, types::b_null()));
    else if (mi.isTime)
      doc.append(kvp(name, types::b_date(tp)));
    else if (useChar)
      doc.append(kvp(name, types::b_utf8(mem.toStr(mobs::ConvToStrHint(compact)))));
    else if (use32)
      doc.append(kvp(name, i32));
    else if (use64)
      doc.append(kvp(name, i64));
    else if (useBool)
      doc.append(kvp(name, (i64 != 0)));
    else if (use128)
      doc.append(kvp(name, i128));
    else if (bin.bytes)
      doc.append(kvp(name, types::b_binary(bin)));
    else if (mi.isFloat)
      doc.append(kvp(name, mi.d));
    else
      doc.append(kvp(name, mem.toStr(mobs::ConvToStrHint(compact))));
  }

  std::string result() {
    return bsoncxx::to_json(doc.view());
  }
  bsoncxx::document::value value() {
    return doc.extract();
  }

  bool index = false;

private:
  bsoncxx::builder::basic::document doc;
};

class BsonOut : virtual public ObjTravConst {
public:
  class Level {
  public:
    explicit Level(bool k) : isKey(k) { };
    bool isKey;
    bsoncxx::builder::basic::document doc;
    bsoncxx::builder::basic::array arr;
  };

  explicit BsonOut(mobs::ConvObjToString c) : cth(std::move(c.exportAltNames())) { };

  bool doObjBeg(const ObjectBase &obj) override
  {
    if (obj.isNull() and cth.omitNull())
      return false;
    if (not obj.isModified() and cth.modOnly())
      return false;
    if (inArray() and noArrays)
      return arrayIndex() == 0;
    level.push(Level(level.empty() or (obj.keyElement() and level.top().isKey)));
    return true;
  };
  void doObjEnd(const ObjectBase &obj) override
  {
    if (level.size() == 1)
      return;
//      LOG(LM_DEBUG, "OE " << bsoncxx::to_json(level.top().doc.view()) << " " << boolalpha << inArray());
    if (inArray() and noArrays)
      return;
    bsoncxx::document::value val = level.top().doc.extract();
    level.pop();
    if (level.empty())
      THROW("underflow");
    if (inArray())
    {
      if (obj.isNull())
        level.top().arr.append(types::b_null());
      else
        level.top().arr.append(val);
    }
    else if (not obj.getElementName().empty())
    {
      std::string name = obj.getName(cth);
      if (obj.isNull())
        level.top().doc.append(kvp(name, types::b_null()));
      else
        level.top().doc.append(kvp(name, val));
    }
  };
  bool doArrayBeg(const MemBaseVector &vec) override
  {
    if (vec.isNull() and cth.omitNull())
      return false;
    if (not vec.isModified() and cth.modOnly())
      return false;
    if (index) // für Projektion
    {
      std::string name = vec.getName(cth);
      level.top().doc.append(kvp(name, 1));
      return false;
    }
    level.push(Level(false));
    return true;
  };
  void doArrayEnd(const MemBaseVector &vec) override
  {
    if (noArrays)
    {
      bsoncxx::document::value val = level.top().doc.extract();
      level.pop();
      if (level.empty())
        throw std::runtime_error("underflow");

      std::string name = vec.getName(cth);
      if (not vec.getElementName().empty())
      {
        if (vec.isNull())
          level.top().doc.append(kvp(name, types::b_null()));
        else
          level.top().doc.append(kvp(name, val));
      }
      return;
    }
//      LOG(LM_DEBUG, "AE " << bsoncxx::to_json(level.top().arr.view()));

    bsoncxx::array::value val = level.top().arr.extract();
    level.pop();
    if (level.empty())
      throw std::runtime_error("underflow");
    if (inArray())
    {
      if (vec.isNull())
        level.top().arr.append(types::b_null());
      else
        level.top().arr.append(val);
    }
    else if (not vec.getElementName().empty())
    {
      std::string name = vec.getName(cth);
      if (vec.isNull())
        level.top().doc.append(kvp(name, types::b_null()));
      else
        level.top().doc.append(kvp(name, val));
    }
  };
  void doMem(const MemberBase &mem) override
  {
    if (noKeys and mem.keyElement() > 0 and level.top().isKey)
      return;
    if (mem.isNull() and cth.omitNull())
      return;
    if (not mem.isModified() and cth.modOnly())
      return;
    if (inArray() and noArrays and arrayIndex() != 0)
      return;

    std::string name = mem.getName(cth);
    if (index)
    {
      level.top().doc.append(kvp(name, 1));
      return;
    }
    bool compact = cth.compact();
    if (mem.is_chartype(cth) and mem.hasFeature(mobs::DbCompact))
      compact = true;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    mi.changeCompact(compact);
    double d;
    int32_t i32;
    int64_t i64 = mi.i64;
    bsoncxx::decimal128 i128;
    std::chrono::system_clock::time_point tp{};
    types::b_binary bin{};
    bin.size = 0;
    bin.bytes = nullptr;
    bool useChar = mem.is_chartype(mobs::ConvToStrHint(compact));
    bool useBool = false;
    bool use32 = false;
    bool use64 = false;
    bool use128 = false;
    if (mi.isSigned)
    {
      if (inKeyMode() and mem.isVersionField()) {
        if (version == -1)
          version = i64;
        else
          THROW("VersionInfo duplicate");
      }
      if (increment and mem.isVersionField()) {
        if (i64 == mi.max)
          throw std::runtime_error("VersionElement overflow");
        i64++;
      }
      if (mi.max <= INT32_MAX and mi.min >= INT32_MIN)
      {
        i32 = (int32_t)i64;
        use32 = true;
      }
      else
        use64 = true;
    }
    else if (mi.isUnsigned)
    {
      if (inKeyMode() and mem.isVersionField()) {
        LOG(LM_DEBUG, "FOUND Version " << mi.u64);
        if (mi.u64 >= INT64_MAX)
          THROW("VersionInfo overflow");
        else if (version == -1)
          version = mi.u64;
        else
          THROW("VersionInfo duplicate");
      }
      if (increment and mem.isVersionField()) {
        if (mi.u64 == mi.max or mi.max == 1)
          throw std::runtime_error("VersionElement overflow");
        mi.u64++;
      }
      if (mi.max == 1) {
        i64 = (int64_t)mi.u64;
        useBool = true;
      }
      else if (mi.max <= std::numeric_limits<int32_t>::max()) {
        i32 = (int32_t)mi.u64;
        use32 = true;
      }
      else if (mi.max <= INT64_MAX or mi.i64 <= INT64_MAX) {
        i64 = (int64_t)mi.u64;
        use64 = true;
      }
      else // Typ 128 nur nehmen, wenn Zahl nicht in int64 passt
      {
        i128 = bsoncxx::decimal128(0, mi.u64);
        use128 = true;
      }
    }
    else if (increment and mem.isVersionField())
      throw std::runtime_error("VersionElement is not int");
    else if (mi.isTime)
    {
      tp += std::chrono::microseconds(mi.t64);
    }
    else if (mi.isBlob)
    {
      bin.size = int32_t(mi.u64);
      bin.bytes = (uint8_t *)mi.blob;
      useChar = false;
    }

    if (inArray() and not noArrays)
    {
      if (mem.isNull())
        level.top().arr.append(types::b_null());
      else if (mi.isTime)
        level.top().arr.append(types::b_date(tp));
      else if (useChar)
        level.top().arr.append(types::b_utf8(mem.toStr(cth)));
      else if (use32)
        level.top().arr.append(i32);
      else if (use64)
        level.top().arr.append(i64);
      else if (useBool)
        level.top().arr.append(i64 != 0);
      else if (use128)
        level.top().arr.append(i128);
      else if (bin.bytes)
        level.top().arr.append(types::b_binary(bin));
      else if (mi.isFloat)
        level.top().arr.append(mi.d);
      else
        level.top().arr.append(mem.toStr(cth));
    }
    else
    {
      if (mem.isNull())
        level.top().doc.append(kvp(name, types::b_null()));
      else if (mi.isTime)
        level.top().doc.append(kvp(name, types::b_date(tp)));
      else if (useChar)
        level.top().doc.append(kvp(name, types::b_utf8(mem.toStr(cth))));
      else if (use32)
        level.top().doc.append(kvp(name, i32));
      else if (use64)
        level.top().doc.append(kvp(name, i64));
      else if (useBool)
        level.top().doc.append(kvp(name, (i64 != 0)));
      else if (use128)
        level.top().doc.append(kvp(name, i128));
      else if (bin.bytes)
        level.top().doc.append(kvp(name, types::b_binary(bin)));
      else if (mi.isFloat)
        level.top().doc.append(kvp(name, mi.d));
      else
        level.top().doc.append(kvp(name, mem.toStr(cth)));
    }
  }
  std::string result() {
    if (level.empty())
      throw std::runtime_error("underflow");
    return bsoncxx::to_json(level.top().doc.view());
  }
  bsoncxx::document::value value() {
    return level.top().doc.extract();
  }
  bsoncxx::document::value setValue() {
    bsoncxx::document::value val = level.top().doc.extract();
    bsoncxx::builder::basic::document doc;
    doc.append(kvp("$set", val));
    return doc.extract();
  }
  bool noKeys = false;
  bool noArrays = false;
  bool index = false; // erzeuge den Primär-Schlüssel (alle Member = 1)
  bool increment = false; // VersionsVariable um 1 erhöhen
  int64_t version = -1;
private:
  stack<Level> level;
  ConvObjToString cth;
};



//////////////////////////////////////////////////////////////////////////////////////////////////////

class MongoQuery {
public:
  class Level {
  public:
    explicit Level(QueryGenerator::Operator o, bool negate) : op(o), neg(negate) { };
    QueryGenerator::Operator op;
    bool neg = false;
//    bsoncxx::builder::basic::document doc;
    bsoncxx::builder::basic::array arr;
    bool invert() { return neg and op == QueryGenerator::AndBegin; }
  };

  MongoQuery(const QueryGenerator *query) : query(query) {
    if (query)
      query->createLookup(lookUp);
  }
#if 0
  void showValue(QueryItem &i, std::stringstream &res) {
    switch(i.op) {
      case QueryGenerator::ConstStr:
        res << to_quote(i.str);
        break;
      case QueryGenerator::ConstInt:
        res << i.int64;
        break;
      case QueryGenerator::ConstUInt:
        res << i.uint64;
        break;
      case QueryGenerator::ConstDbl:
        res << i.dbl;
        break;
      case QueryGenerator::ConstTrue:
        res << " true ";
        break;
      case QueryGenerator::ConstFalse:
        res << " false ";
        break;
      default:
        THROW("Value missing");
    }
  }
#endif

  void reduceRevel() {
    std::string op = level.top().op == QueryGenerator::AndBegin ? "$and" : "$or";
    if (level.top().neg)
      op = level.top().op == QueryGenerator::AndBegin ? "$or" : "$nor"; // invert() beachten

    bsoncxx::array::value val = level.top().arr.extract();
    level.pop();
    if (level.empty())
      doc.append(kvp(op, val));
    else
      level.top().arr.append(make_document(kvp(op, val)));
  }

  void genValue(bsoncxx::builder::basic::document &doc, const std::string name, const QueryGenerator::QueryItem &i) {
    if (i.op != QueryGenerator::Const)
      THROW("no constant");
//    LOG(LM_INFO, " OPER " << name << " " << i.toString());

    if (i.isTime)
    {
      std::chrono::system_clock::time_point tp{};
      tp += std::chrono::microseconds(i.t64);
      doc.append(kvp(name, types::b_date(tp)));
    } else if (i.isSigned)
      doc.append(kvp(name, i.i64));
    else if (i.isUnsigned and i.max == 1)
      doc.append(kvp(name, (i.u64 ? true:false)));
    else if (i.isUnsigned and i.u64 <= INT64_MAX)
      doc.append(kvp(name, int64_t(i.u64)));
    else if (i.isUnsigned) {
      bsoncxx::decimal128 i128 = bsoncxx::decimal128(0, i.u64);
      doc.append(kvp(name, types::b_decimal128(i128)));
    } else
      doc.append(kvp(name, types::b_utf8(i.text)));
  }

  void genValue(bsoncxx::builder::basic::array &arr, const QueryGenerator::QueryItem &i) {
    if (i.op != QueryGenerator::Const)
      THROW("no constant");
//    LOG(LM_INFO, "ARRAY " << i.toString());
    if (i.isTime)
    {
      std::chrono::system_clock::time_point tp{};
      tp += std::chrono::microseconds(i.t64);
      arr.append(types::b_date(tp));
    } else if (i.isSigned)
      arr.append(i.i64);
    else if (i.isUnsigned and i.max == 1)
      arr.append((i.u64 ? true:false));
    else if (i.isUnsigned and i.u64 <= INT64_MAX)
      arr.append(int64_t(i.u64));
    else if (i.isUnsigned) {
      bsoncxx::decimal128 i128 = bsoncxx::decimal128(0, i.u64);
      arr.append(types::b_decimal128(i128));
    } else
      arr.append(types::b_utf8(i.text));
  }

  void generate() {
    std::stringstream res;
    int params = 0;
    int vars = 0;

    // Equal, Less, LessEqual, Grater, GraterEqual, NotEqual, Like
    std::vector<const char *> binOp = {"$eq", "$lt", "$lte", "$gt", "$gte", "$ne", "$regex"};
    std::vector<const char *> txtOp = {"=",   "<",   "<=",   ">",   ">=",   "!=",  " LIKE "};
//    std::stack<std::string> lastDelim;
//    std::string aktDelim = " AND ";
//    std::string valDelim;
//    std::string delim;
//    std::string d;

    std::string script;
    bool literal = false;
    bool inScript = false;
    bool simpleScript = false;  // Ohne ExpBegin
    bool negate = false;
    for (auto it = query->query.begin(); it != query->query.end(); it++) {
      auto i = *it;
      std::string variable;
      bool scriptFinish = false;
      switch (i.op) {
        case QueryGenerator::Not:
          if (inScript or literal)
            THROW("'not' not allowed in literal");
          negate = not negate;
          break;
//        case QueryGenerator::ExpBegin:
//          if (inScript or literal)
//            THROW("already in expression mode");
//          inScript = true;
//          break;
//        case QueryGenerator::ExpEnd:
//          if (not inScript)
//            THROW("not in in expression mode");
//          scriptFinish = true;
//          break;
        case QueryGenerator::Variable: {
          auto lu = lookUp.find(i.mem);
          if (lu != lookUp.end())
            variable = lu->second;
          else
            THROW("variable missing");
          if (inScript or literal) {
            if (inScript)
              script += "this.";
            script += variable;
            variable = "";
            if (simpleScript)
              scriptFinish = true;
            break;
          }
          auto it2 = it;
          if (++it2 == query->query.end())
            THROW("syntax");
          switch (it2->op) {
            case QueryGenerator::Equal ... QueryGenerator::Like: {
              bool like = it2->op == QueryGenerator::Like;
              std::string op = binOp[int(it2->op) - int(QueryGenerator::Equal)];
              if (++it2 == query->query.end())
                THROW("syntax");
              bsoncxx::builder::basic::document d;
              if (like) {
                if (it2->op != QueryGenerator::Const or it2->isNumber())
                  THROW("'like' with no string constant");
                QueryGenerator::QueryItem qi(QueryGenerator::Const);
                qi.text = mobs::convLikeToRegexp(it2->text);
                genValue(d, op, qi);
              } else if (it2->op == QueryGenerator::Variable) {
#if 0
                inScript = true;
                simpleScript = true;
                script = "this.";
                script += variable;
                continue;
#else
                bsoncxx::builder::basic::array arr;
                // $expr:{$gt:["$Grade1", "$Grade2"]}
                arr.append(variable);
                auto lu = lookUp.find(it2->mem);
                if (lu != lookUp.end())
                  arr.append(lu->second);
                else
                  THROW("variable missing");
                bsoncxx::array::value val = arr.extract();
                d.append(kvp(op, val));
                variable = "$expr";
                LOG(LM_WARNING, "$expr statement is experimental");
#endif
              } else
                genValue(d, op, *it2);
              bsoncxx::document::value val = d.extract();
              if (negate)
                val = make_document(kvp("$not", val));
              negate = not level.empty() and level.top().invert();
              if (not level.empty())
                level.top().arr.append(make_document(kvp(variable, val)));
              else
                doc.append(kvp(variable, val));
              it = it2;
              break;
            }
            case QueryGenerator::IsNotNull:
            case QueryGenerator::IsNull: {
              if (it2->op == QueryGenerator::IsNotNull)
                negate = not negate;
              std::string op = negate ? "$ne" : "$eq";
              negate = not level.empty() and level.top().invert();
              bsoncxx::document::value val = make_document(kvp(op, types::b_null()));
              if (not level.empty())
                level.top().arr.append(make_document(kvp(variable, val)));
              else
                doc.append(kvp(variable, val));
              it = it2;
              break;
            }
            case QueryGenerator::Between: {
              if (++it2 == query->query.end())
                THROW("syntax");
              bsoncxx::builder::basic::document d;
              genValue(d, "$gte", *it2);
              if (++it2 == query->query.end())
                THROW("syntax");
              genValue(d, "$lte", *it2);
              bsoncxx::document::value val = d.extract();
              if (negate)
                val = make_document(kvp("$not", val));
              negate = not level.empty() and level.top().invert();
              if (not level.empty())
                level.top().arr.append(make_document(kvp(variable, val)));
              else
                doc.append(kvp(variable, val));
              it = it2;
              break;
            }
            case QueryGenerator::InBegin: {
              bsoncxx::builder::basic::array arr;
              for (;;) {
                if (++it2 == query->query.end())
                  THROW("syntax");
                if (it2->op == QueryGenerator::InEnd)
                  break;
                genValue(arr, *it2);
              }
              std::string op = negate ? "$nin" : "$in";
              bsoncxx::document::value val = make_document(kvp(op, arr.extract()));
              negate = not level.empty() and level.top().invert();
              if (not level.empty())
                level.top().arr.append(make_document(kvp(variable, val)));
              else
                doc.append(kvp(variable, val));
              it = it2;
              break;
            }
            default:
              THROW("syntax");
          }
          break;
        }
        case QueryGenerator::AndBegin:
        case QueryGenerator::OrBegin:
          level.emplace(i.op, negate);
          LOG(LM_INFO, "LEVEL " << level.size());
          negate = level.top().invert();
          break;
        case QueryGenerator::AndEnd:
        case QueryGenerator::OrEnd: {
          if (level.empty())
            THROW("parentheses error");
          if (level.top().op != (i.op == QueryGenerator::AndEnd ? QueryGenerator::AndBegin : QueryGenerator::OrBegin))
            THROW("and/or mismatch");
          reduceRevel();
          break;
        }
        case QueryGenerator::Equal ... QueryGenerator::NotEqual: {
          if (not inScript)
            THROW("syntax");
          const char *op = txtOp[int(i.op) - int(QueryGenerator::Equal)];
          script += op;
          break;
        }
        case QueryGenerator::Const: {
          if (not inScript and not literal)
            THROW("syntax");
          bool quote;
          std::string t = i.toString(&quote);
          if (quote and not literal)
            script += mobs::to_quote(t);
          else
            script += t;
          break;
        }
        case QueryGenerator::literalBegin:
          if (inScript or literal)
            THROW("already in literal mode");
          literal = true;
          if (level.empty()) {
            level.emplace(QueryGenerator::AndBegin, negate);
            LOG(LM_INFO, "LEVEL " << level.size());
            negate = not level.empty() and level.top().invert();
          }
          break;
        case QueryGenerator::literalEnd: {
          LOG(LM_DEBUG, "literal: " << script);
          auto it2 = it;
          auto val = bsoncxx::from_json(script);
          if (not level.empty()) {
            bsoncxx::builder::basic::document d;
            if (negate) {
              d.append(kvp("$not", val));
              val = d.extract();
            }
            level.top().arr.append(val);
            literal = false;
            script = "";
          } else
            THROW("literal needs AND or OR");
          negate = not level.empty() and level.top().invert();
          break;
        }
        default:
          THROW("unknown element");

      }
      if (scriptFinish) {
        script = std::string("function() { return ") + script + " }";
        if (not level.empty()) {
          bsoncxx::builder::basic::document d;
          if (negate)
            d.append(kvp("$not", make_document(kvp("$where", types::b_code(script)))));
          else
            d.append(kvp("$where", types::b_code(script)));
          bsoncxx::document::value val = d.extract();
          level.top().arr.append(val);

        } else {
          if (negate)
            doc.append(kvp("$not", make_document(kvp("$where", types::b_code(script)))));
          else
            doc.append(kvp("$where", types::b_code(script)));
        }
        script = "";
        inScript = false;
        simpleScript = false;
      }
    }
    while (not level.empty())
      reduceRevel();
  }

  std::string result() {
    return bsoncxx::to_json(doc.view());
  }
  bsoncxx::document::value value() {
    return doc.extract();
  }


  const QueryGenerator *query;
  std::map<const MemberBase *, std::string> lookUp;
  bsoncxx::builder::basic::document doc;
  stack<Level> level;



};



//////////////////////////////////////////////////////////////////////////////////////////////////////





class MongoRead : public mobs::ObjectNavigator  {
public:
  MongoRead(ConvObjFromStr c) : mobs::ObjectNavigator(c) { }

  void parsival(const bsoncxx::document::view &v, const std::string& array = "") {
    try {
      for (auto const &e:v) {
        bool skip = false;
        std::string key = e.key().to_string();
        //      cerr << "K " << key << " - " << array << endl;
        if (not array.empty())
          enter(array);
        switch (e.type()) {
          case bsoncxx::type::k_oid:
            oid_time = UxTime(e.get_oid().value.get_time_t());
            oid = e.get_oid().value.to_string();
            skip = true;
            break;
          case bsoncxx::type::k_array: {
            const types::b_array a = e.get_array();
            //          cerr << "ARRAY BEG" << endl;
            parsival(a.value, key);
            //          cerr << "ARRAY END" << endl;
            skip = true;
            break;
          }
          case bsoncxx::type::k_null:
            if (array.empty())
              enter(key);
            setNull();
            if (array.empty())
              leave();
            skip = true;
            break;
          case bsoncxx::type::k_document: {
            types::b_document d = e.get_document();
            if (array.empty())
              enter(key);
            //          cerr << "OBJECT BEG" << endl;
            parsival(d.value);
            //          cerr << "OBJECT END" << endl;
            if (array.empty())
              leave(key);
            skip = true;
            break;
          }
          default:
            break;
        }
        if (not skip) {
          if (array.empty())
            enter(key);
          if (member()) {
            ConvToStrHint cts(not cfs.acceptExtended());
            bool compact = cts.compact();
            if (member()->is_chartype(cts) and member()->hasFeature(mobs::DbCompact))
              compact = true;
            MobsMemberInfo mi;
            member()->memInfo(mi);
            mi.changeCompact(compact);
            switch (e.type()) {
              case bsoncxx::type::k_oid:
                break; // skipped
              case bsoncxx::type::k_utf8:
                if (not member()->fromStr(e.get_utf8().value.to_string(), cfs))
                  throw runtime_error(u8"invalid type, can't assign");
                break;
              case bsoncxx::type::k_bool:
                mi.setBool(e.get_bool().value != 0);
                if (not mi.isUnsigned or mi.max != 1 or not member()->fromMemInfo(mi))
                  throw runtime_error(u8"invalid type, can't assign");
                break;
              case bsoncxx::type::k_int32:
                mi.setInt(e.get_int32().value);
                if (not member()->fromMemInfo(mi))
                  throw runtime_error(u8"invalid type, can't assign");
                break;
              case bsoncxx::type::k_int64:
                mi.setInt(e.get_int64().value);
                if (not member()->fromMemInfo(mi))
                  throw runtime_error(u8"invalid type, can't assign");
                break;
              case bsoncxx::type::k_decimal128:
                mi.setUInt(e.get_decimal128().value.low());
                if (e.get_decimal128().value.high() != 0 or not member()->fromMemInfo(mi))
                  throw runtime_error(u8"invalid type, can't assign");
                break;
              case bsoncxx::type::k_date:
                mi.setTime(e.get_date().to_int64() * 1000);
                if (not mi.isTime or not member()->fromMemInfo(mi))
                  throw runtime_error(u8"invalid type, can't assign");
                break;
              case bsoncxx::type::k_timestamp:
                cerr << "timestamp" << endl;
                throw runtime_error(u8"invalid type, can't assign");
                break;
              case bsoncxx::type::k_double:
                mi.d = e.get_double().value;
                if (not mi.isFloat or not member()->fromMemInfo(mi))
                  throw runtime_error(u8"invalid type, can't assign");
                break;
              case bsoncxx::type::k_binary: {
                mi.blob = e.get_binary().bytes;
                mi.u64 = e.get_binary().size;
                if (not mi.isBlob or not member()->fromMemInfo(mi))
                  throw runtime_error(u8"invalid type, can't assign blob");
                break;
              }
              default:
                cerr << "sonst " << int(e.type()) << endl;
                throw runtime_error(u8"invalid type, can't assign");
            }
          } else if (cfs.exceptionIfUnknown())
            throw runtime_error(u8"no variable, can't assign");
          else
            LOG(LM_DEBUG, u8"mongodb element " << showName() << " is not in object");
          if (array.empty())
            leave();
        }
        if (not array.empty())
          leave(array);
      }
    } catch (std::exception &e) {
      THROW(u8"mongodb element " << showName() << ": " << e.what());
    }
  }
  UxTime oid_time{};
  std::string oid;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////

class Cursor : public virtual mobs::DbCursor {
  friend class mobs::MongoDatabaseConnection;
public:
  explicit Cursor(mongocxx::cursor &&c, std::shared_ptr<DatabaseConnection> dbi, std::string dbName, bool keysOnly) :
          cursor(std::move(c)), it(cursor.begin()), dbCon(std::move(dbi)), databaseName(std::move(dbName)), isKeysOnly(keysOnly) { }
  ~Cursor() override = default;;
  bool eof() override  { return it == cursor.end(); }
  bool valid() override { return not eof(); }
  bool keysOnly() const override { return isKeysOnly; }
  void operator++() override { if (eof()) return; it.operator++(); cnt++; }
private:
  mongocxx::cursor cursor;
  mongocxx::cursor::iterator it;
  std::shared_ptr<DatabaseConnection> dbCon;  // verhindert das Zerstören der Connection
  std::string databaseName;  // unused
  bool isKeysOnly;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
class CountCursor : public virtual mobs::DbCursor {
  friend class mobs::MongoDatabaseConnection;
public:
  explicit CountCursor(size_t size) { cnt = size; }
  ~CountCursor() override = default;;
  bool eof() override  { return true; }
  bool valid() override { return false; }
  void operator++() override { }
};

using mongocxx::v_noabi::client_session;

class MongoTransactionDbInfo : public TransactionDbInfo {
public:
  explicit MongoTransactionDbInfo(client_session &&cs) : session(std::move(cs)) {}
  client_session session;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////

}



namespace mobs {

std::string MongoDatabaseConnection::collectionName(const ObjectBase &obj) {
  MemVarCfg c = obj.hasFeature(ColNameBase);
  if (c)
    return obj.getConf(c);
  return obj.getObjectName();
}

std::map<std::string, mongocxx::pool> MongoDatabaseConnection::pools;

void MongoDatabaseConnection::close() {
  entry.release();
}

void MongoDatabaseConnection::open() {
  if (not entry) {
    // "mongodb://my_user:password@localhost:27017/my_database?ssl=true"
    // "mongodb://db1.example.net:27017,db2.example.net:2500/?replicaSet=test&connectTimeoutMS=300000"
    auto it = pools.find(m_url);
    if (it == pools.end()) {
      mongocxx::uri u(m_url);
      it = pools.emplace(m_url, u).first;
      if (it == pools.end())
        THROW("connection failed");
    }

    entry = std::unique_ptr<Entry>(new Entry(it->second));
  }
}

bool MongoDatabaseConnection::load(DatabaseInterface &dbi, ObjectBase &obj) {
  open();
  BsonOut bo(mobs::ConvObjToString().exportExtended());
  obj.traverseKey(bo);
  LOG(LM_DEBUG, "LOAD " << dbi.database() << "." << collectionName(obj) << " " << bo.result());

  mongocxx::database db = entry->client()[dbi.database()];
  auto val = db[collectionName(obj)].find_one(bo.value());
  if (not val)
    return false;

//      std::cout << bsoncxx::to_json(*val) << "\n";
  MongoRead mr(ConvObjFromStr().useAlternativeNames());
  obj.clear();
  mr.pushObject(obj);
  mr.parsival(val->view());
  return true;
}

void MongoDatabaseConnection::create(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  BsonOut bo(mobs::ConvObjToString().exportExtended().exportExtended());
  bo.increment = true;
  obj.traverse(bo);
  LOG(LM_DEBUG, "CREATE " << dbi.database() << "." << collectionName(obj) << " " << bo.result());

  //bsoncxx::stdx::optional<mongocxx::result::insert_one> result =
  mongocxx::database db = entry->client()[dbi.database()];
  auto result = db[collectionName(obj)].insert_one(bo.value());

  if (not result)
    THROW(u8"create failed");
  auto oid = result->inserted_id();
  LOG(LM_DEBUG, "OID " << oid.get_oid().value.to_string());
}

void MongoDatabaseConnection::save(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  auto mtdb = static_cast<MongoTransactionDbInfo *>(dbi.transactionDbInfo());

  // TODO create overwrite update
  BsonOut bk(mobs::ConvObjToString().exportExtended());
  bk.withVersionField = true;
  obj.traverseKey(bk);
  LOG(LM_DEBUG, "VERSION IS " << bk.version);
  BsonOut bo(mobs::ConvObjToString().exportWoNull().exportExtended());
  bo.increment = true;
//    bo.noKeys = true;
  obj.traverse(bo);
  LOG(LM_DEBUG, "UPDATE " << dbi.database() << "." << collectionName(obj) << " " << bk.result() <<  " TO "
                         << bo.result());
  mongocxx::database db = entry->client()[dbi.database()];

//    auto result = db[colName(obj)].update_one(bk.value(), bo.setValue());
//    mongocxx::options::update u;
//    u.upsert(create);
  if (bk.version == 0) { // initiale version
    auto result = db[collectionName(obj)].insert_one(bo.value());
    if (not result)
      THROW(u8"save failed");
    auto oid = result->inserted_id();
    LOG(LM_DEBUG, "INSERTED " << oid.get_oid().value.to_string());
  } else { // ohne Versionsfeld (-1) auch upsert erlauben
    auto r_opt = mongocxx::options::replace().upsert(bk.version < 0);
    auto result = db[collectionName(obj)].replace_one(bk.value(), bo.value(), r_opt);
    if (not result)
      THROW(u8"save failed");
    LOG(LM_DEBUG, "MATCHED " << result->matched_count());
    auto oid = result->upserted_id();
    if (oid)
      LOG(LM_DEBUG, "UPSERTED " << oid->get_oid().value.to_string());
  }
}

bool MongoDatabaseConnection::destroy(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
//  DbTransaction *tdb = dbi.getTransaction();
//  if (tdb) {
//    auto mtdb = static_cast<MongoTransactionDbInfo *>(tdb->transactionDbInfo(dbi));
  auto mtdb = static_cast<MongoTransactionDbInfo *>(dbi.transactionDbInfo());

  BsonOut bo(mobs::ConvObjToString().exportExtended());
  bo.withVersionField = true;
  obj.traverseKey(bo);
  LOG(LM_INFO, "VERSION IS " << bo.version);
  if (bo.version == 0)
    THROW(u8"destroy Object version = 0 cannot destroy");
  LOG(LM_DEBUG, "DESTROY " << dbi.database() << "." << collectionName(obj) << " " <<  bo.result());

  bool found;
  mongocxx::database db = entry->client()[dbi.database()];
  if (mtdb) {
    LOG(LM_DEBUG, "drop with session");
    auto result = db[collectionName(obj)].delete_one(mtdb->session, bo.value());
    if (not result)
      THROW(u8"destroy returns with error");
    found = result->deleted_count() != 0;
  } else {
    auto result = db[collectionName(obj)].delete_one(bo.value());
    if (not result)
      THROW(u8"destroy returns with error");
    found = result->deleted_count() != 0;
  }
  if (bo.version > 0 and not found)
    THROW(u8"destroy: Object with appropriate version not found");

  return found;
}

void MongoDatabaseConnection::dropAll(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  LOG(LM_DEBUG, "DROP COLLECTOION " << dbi.database() << "." << collectionName(obj));

  mongocxx::database db = entry->client()[dbi.database()];
  db[collectionName(obj)].drop();
}

void MongoDatabaseConnection::structure(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  mongocxx::database db = entry->client()[dbi.database()];
  // db.test.createIndex({ id: 1 }, { unique: true })
  BsonElements bo((mobs::ConvObjToString()));
  bo.index = true;
  obj.traverseKey(bo);
  LOG(LM_DEBUG, "CREATE PRiMARY " << dbi.database() << "." << collectionName(obj) << " " <<  bo.result());
  auto idx = bsoncxx::builder::basic::document{};
  idx.append(kvp("unique", true));
  db[collectionName(obj)].create_index(bo.value(), idx.extract());
}

std::shared_ptr<DbCursor>
MongoDatabaseConnection::query(DatabaseInterface &dbi, ObjectBase &obj, bool qbe, const QueryGenerator *query,
                               const QueryOrder *sort) {
  open();
  mongocxx::database db = entry->client()[dbi.database()];
  mongocxx::collection col = db[collectionName(obj)];

  auto mtdb = static_cast<MongoTransactionDbInfo *>(dbi.transactionDbInfo());
  if (mtdb) {
    mongocxx::read_concern rc = col.read_concern();
    if (changedReadConcern(rc, dbi))
      col.read_concern(rc);
  }

  mongocxx::options::find f_opt = mongocxx::options::find().skip(dbi.getQuerySkip()).limit(dbi.getQueryLimit());
  mongocxx::options::count c_opt = mongocxx::options::count().skip(dbi.getQuerySkip());
  if (dbi.getQueryLimit())
    c_opt = c_opt.limit(dbi.getQueryLimit());
  if (not dbi.getCountCursor() and dbi.getKeysOnly()) {
    // Projektion auf Key-Elemente
    BsonElements bo(mobs::ConvObjToString().exportWoNull());
    bo.index = true;
    ObjectBase *o2 = obj.createNew();
    o2->traverseKey(bo);
    delete o2;
    LOG(LM_DEBUG, "Projection " << collectionName(obj) << " " << bo.result());
    f_opt = f_opt.projection(bo.value());
  }
  // Sortierung
  std::string sortLog;
  if (sort and not dbi.getCountCursor()) {
    BsonElements bo((mobs::ConvObjToString()));
    bo.startOrder(*sort);
    obj.traverse(bo);
    bo.finishOrder();
    sortLog = " sort (";
    sortLog += bo.result();
    sortLog += ")";
    f_opt = f_opt.sort(bo.value());
  }
  if (dbi.getTimeout() > std::chrono::milliseconds(0)) {
    c_opt = c_opt.max_time(dbi.getTimeout());
    f_opt = f_opt.max_time(dbi.getTimeout());
  }

  if (qbe) {
    BsonElements bq(mobs::ConvObjToString().exportModified());
    obj.setModified(true);  // äußere Klammer muss sein
    obj.traverse(bq);
    LOG(LM_DEBUG, "QUERY " << dbi.database() << "." << collectionName(obj) << " " << bq.result() << sortLog);

    if (dbi.getCountCursor())
      return std::make_shared<CountCursor>(col.count_documents(bq.value(), c_opt));
    return std::make_shared<Cursor>(col.find(bq.value(), f_opt), dbi.getConnection(), dbi.database(), dbi.getKeysOnly());
  } else {
    MongoQuery qgen(query);
    if (not qgen.lookUp.empty()) {
      BsonElements bo((mobs::ConvObjToString()));
      bo.startLookup(qgen.lookUp);
      obj.traverse(bo);
    }
    qgen.generate();

    LOG(LM_DEBUG, "QUERY " << dbi.database() << "." << collectionName(obj) << " " << qgen.result() << "  " << sortLog);
//    auto doc = bsoncxx::from_json(q); doc.view()
    if (dbi.getCountCursor())
      return std::make_shared<CountCursor>(col.count_documents(qgen.value(), c_opt));
    return std::make_shared<Cursor>(col.find(qgen.value(), f_opt), dbi.getConnection(), dbi.database(), dbi.getKeysOnly());
  }
}

void
MongoDatabaseConnection::retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) {
  auto curs = std::dynamic_pointer_cast<Cursor>(cursor);
  if (not curs)
    THROW("invalid cursor");

  LOG(LM_DEBUG, "ANSWER " << bsoncxx::to_json(*(curs->it)));
  MongoRead mr(ConvObjFromStr().useAlternativeNames());
  obj.clear();
  mr.pushObject(obj);
  mr.parsival(*(curs->it));
}


mongocxx::database MongoDatabaseConnection::getDb(DatabaseInterface &dbi) {
  open();
  return entry->client()[dbi.database()];
}

bool MongoDatabaseConnection::changedReadConcern(mongocxx::read_concern &rc, const DatabaseInterface &dbi) {
  auto trans = dbi.getTransaction();
  if (not trans)
    return false;
  auto lv = mongocxx::v_noabi::read_concern::level::k_unknown;
  switch (trans->getIsolation()) {
    case DbTransaction::ReadUncommitted:
      lv = mongocxx::v_noabi::read_concern::level::k_local; break;
    case DbTransaction::ReadCommitted:
      lv = mongocxx::v_noabi::read_concern::level::k_majority; break;
    case DbTransaction::RepeatableRead:
      lv = mongocxx::v_noabi::read_concern::level::k_linearizable; break;
    case DbTransaction::CursorStability:
    case DbTransaction::Serializable:
      lv = mongocxx::v_noabi::read_concern::level::k_snapshot; break;
  }

  if (rc.acknowledge_level() == lv)
    return false;
  rc.acknowledge_level(lv);
  LOG(LM_DEBUG, "changing isolation level " << int(trans->getIsolation()));
  return true;
}


void MongoDatabaseConnection::startTransaction(DatabaseInterface &dbi, DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) {
  open();
//  if (not tdb) {
//    LOG(LM_DEBUG, "MongoDB startTransaction");
//    auto mtdb = make_shared<MongoTransactionDbInfo>(entry->client().start_session());
//    tdb = mtdb;
//    mtdb->session.start_transaction();
//  }
}

void MongoDatabaseConnection::endTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) {
  if ((tdb)) {
    LOG(LM_DEBUG, "MongoDB endTransaction");
    auto mtdb = static_pointer_cast<MongoTransactionDbInfo>(tdb);
    if ((mtdb)) {
      mtdb->session.commit_transaction();
      return;
    }
  }
//  throw std::runtime_error("endTransaction missing TransactinInfo");
}

void MongoDatabaseConnection::rollbackTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) {
  if ((tdb)) {
    LOG(LM_DEBUG, "MongoDB rollbackTransaction");
    auto mtdb = static_pointer_cast<MongoTransactionDbInfo>(tdb);
    if ((mtdb)) {
      mtdb->session.abort_transaction();
      return;
    }
  }
//  throw std::runtime_error("rollbackTransaction missing TransactinInfo");
}

size_t MongoDatabaseConnection::maxAuditChangesValueSize(const DatabaseInterface &dbi) const {
  return 0;   // do not split AuditChanges values
}

void MongoDatabaseConnection::uploadFile(DatabaseInterface &dbi, const std::string &id, std::istream &source) {
  open();
  LOG(LM_DEBUG, "UPLOAD FILE " << dbi.database() << "." << id);

  mongocxx::database db = entry->client()[dbi.database()];
  auto gridfs_bucket = db.gridfs_bucket();
  auto doc = make_document(kvp("v", oid(id)));
  bsoncxx::document::element el = doc.view()["v"];
  gridfs_bucket.upload_from_stream_with_id(el.get_value(),	"", &source);
}

std::string MongoDatabaseConnection::uploadFile(DatabaseInterface &dbi, std::istream &source) {
  open();
  LOG(LM_DEBUG, "UPLOAD FILE " << dbi.database());

  mongocxx::database db = entry->client()[dbi.database()];
  auto gridfs_bucket = db.gridfs_bucket();
  auto result = gridfs_bucket.upload_from_stream(	"", &source);
  return result.id().get_oid().value.to_string();
}

void MongoDatabaseConnection::downloadFile(DatabaseInterface &dbi, const std::string &id, std::ostream &dest) {
  open();
  LOG(LM_DEBUG, "DOWNLOAD FILE " << dbi.database() << "." << id);

  mongocxx::database db = entry->client()[dbi.database()];
  auto gridfs_bucket = db.gridfs_bucket();
  auto doc = make_document(kvp("v", oid(id)));
  bsoncxx::document::element el = doc.view()["v"];
  gridfs_bucket.download_to_stream(el.get_value(), &dest);
}

void MongoDatabaseConnection::deleteFile(DatabaseInterface &dbi, const std::string &id) {
  open();
  LOG(LM_DEBUG, "DELETE FILE " << dbi.database() << "." << id);

  mongocxx::database db = entry->client()[dbi.database()];
  auto gridfs_bucket = db.gridfs_bucket();
  auto doc = make_document(kvp("v", oid(id)));
  bsoncxx::document::element el = doc.view()["v"];
  gridfs_bucket.delete_file(el.get_value());
}

}
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



#include "querygenerator.h"
#include "logging.h"
#include "converter.h"
#include "helper.h"
#include <list>
#include <vector>
#include <stack>
#include <sstream>
#include <cstring>

namespace mobs {

QueryGenerator &operator<<(QueryGenerator &g, QueryGenerator::Operator o) { g.add(o); return g; }

QueryGenerator &operator<<(QueryGenerator &g, const std::string &s) { g.add(s); return g; }

QueryGenerator &operator<<(QueryGenerator &g, const char *s) { g.add(std::string(s)); return g; }

QueryGenerator &operator<<(QueryGenerator &g, int64_t i) { g.add(i); return g; }

QueryGenerator &operator<<(QueryGenerator &g, uint64_t i) { g.add(i); return g; }

QueryGenerator &operator<<(QueryGenerator &g, int32_t i) { g.add(int64_t(i)); return g; }

QueryGenerator &operator<<(QueryGenerator &g, bool b) { g.add(b); return g; }

QueryGenerator &operator<<(QueryGenerator &g, QueryInfo q) {
  g.add(*q.mem);
  g.addOp(q.op);
  for (auto &i:q.content)
    g.add(i);
  if (std::string("IN") == q.op)
    g.add(QueryGenerator::InEnd);
  return g;
}

class QueryGeneratorData {
public:


};



QueryGenerator::QueryGenerator() {
  data = new QueryGeneratorData;
}

QueryGenerator::~QueryGenerator() {
  delete data;
}

void QueryGenerator::add(const mobs::MemberBase &mem) {
  QueryGenerator::QueryItem i;
  i.mem = &mem;
  query.push_back(i);
}

void QueryGenerator::add(const std::string &s) {
  QueryGenerator::QueryItem i;
  i.text = s;
  i.op = QueryGenerator::Const;
  query.emplace_back(i);
}

void QueryGenerator::add(QueryGenerator::Operator op) {
  query.emplace_back(op);
}

void QueryGenerator::add(int64_t i) {
  QueryGenerator::QueryItem it;
  it.i64 = i;
  it.max = INT64_MAX;
  it.min = INT64_MIN;
  it.i64 = i;
  it.isSigned = true;
  it.op = QueryGenerator::Const;
  query.push_back(it);
}

void QueryGenerator::add(uint64_t i) {
  QueryGenerator::QueryItem it;
  it.u64 = i;
  it.max = UINT64_MAX;
  it.isUnsigned = true;
  it.op = QueryGenerator::Const;
  query.push_back(it);
}

void QueryGenerator::add(bool i) {
  QueryGenerator::QueryItem it;
  it.u64 = i;
  it.isUnsigned = true;
  it.max = 1;
  it.op = QueryGenerator::Const;
  query.push_back(it);
}

void QueryGenerator::add(const MobsMemberInfoDb &mi) {
  query.emplace_back(mi);
}


std::string QueryGenerator::show(const std::map<const MemberBase *, std::string> &lookUp, SQLDBdescription *sqd) const {
  std::stringstream res;
  int params = 0;
  int vars = 0;

  // Equal, Less, LessEqual, Grater, GraterEqual, NotEqual, Like
  std::vector<const char *> binOp = { "=", "<", "<=", ">", ">=", "<>", " LIKE " };
  std::stack<std::string>lastDelim;
  std::string aktDelim = " AND ";
  std::string valDelim;
  std::string delim;
  std::string d;
  bool literal = false;
  int needValues = 0;
  for (auto &i:query) {
    if (needValues) { // alle nötigen Parameter einer Operation ausgeben
      delim = aktDelim;
      switch (i.op) {
        case InEnd:
          needValues = 0;
          res << ')';
          break;
        case Const: {
          needValues--;
          res << d;
          if (sqd)
            res << sqd->memInfoStmt(i);
          else {
            bool quote;
            std::string r = i.toString(&quote);
            if (quote)
              r = mobs::to_quote(r);
            res << r;
          }
          d = valDelim;
          break;
        }
        case Variable: {
          needValues--;
          auto it = lookUp.find(i.mem);
          if (it == lookUp.end())
            THROW("no lookup");
          if (it->second.empty())
            res << "VAR";
          else
            res << it->second;
          break;
        }
        default:
          THROW("is not a constant");
      }
      continue;
    }
    d = "";
    switch (i.op) {
      case AndEnd:
      case OrEnd:
        if (lastDelim.empty())
          THROW("syntax");
        aktDelim = lastDelim.top();
        lastDelim.pop();
        res << ")";
        delim = "";
        break;
      default:;
    }
    res << delim;
    delim = "";
    switch (i.op) {
      case Variable: {
        auto it = lookUp.find(i.mem);
        if (it == lookUp.end())
          THROW("no lookup");
        if (it->second.empty())
          res << "VAR";
        else
          res << it->second;
        vars++;
        break;
      }
      case Const: {
        if (sqd)
          res << sqd->memInfoStmt(i);
        else {
          bool quote;
          std::string r = i.toString(&quote);
          if (quote and not literal)
            r = mobs::to_quote(r);
          res << r;
        }
        params++;
        break;
      }
      case Equal ... Like:
        res << binOp[int(i.op) - int(Equal)];
        if (literal)
          break;
        if (vars != 1)
          THROW("operation must begin with mobs::MemVar");
        if (params != 0)
          LOG(LM_ERROR, "Binop with " << params +1 << " params");
        needValues = 1;
        params = vars = 0;
        break;
      case Between:
        if (vars != 1 and params != 0)
          THROW("operation must begin with mobs::MemVar");
        res << " BETWEEN ";
        needValues = 2;
        valDelim = " AND ";
        params = vars = 0;
        break;
      case Not:
        delim = "NOT ";
        if (params + vars != 0)
          LOG(LM_ERROR, "Not with " << (params + vars) << " params");
        break;
      case IsNull:
        if (vars != 1 and params != 0)
          THROW("operation must begin with mobs::MemVar");
        res << " IS NULL ";
        params = vars = 0;
        delim = aktDelim;
        break;
      case IsNotNull:
        if (vars != 1 and params != 0)
          LOG(LM_ERROR, "operation must begin with mobs::MemVar");
        res << " IS NOT NULL ";
        params = vars = 0;
        delim = aktDelim;
        break;
      case AndBegin:
        res << '(';
        lastDelim.push(aktDelim);
        aktDelim = " AND ";
        break;
      case OrBegin:
        res << '(';
        lastDelim.push(aktDelim);
        aktDelim = " OR ";
        break;
      case InBegin:
        if (vars != 1 and params != 0)
          THROW("in operation must begin with mobs::MemVar");
        res << " IN (";
        valDelim = ",";
        needValues = INT_MAX;
        params = vars = 0;
        delim = aktDelim;
        break;
      case AndEnd:
      case OrEnd:
      case InEnd:
        delim = aktDelim;
        break;
      case literalBegin:
        literal = true;
        break;
      case literalEnd:
        literal = false;
        break;
      default:
        res << "OP." << int(i.op);
    }
  }
  return res.str();
}

void QueryGenerator::createLookup(std::map<const MemberBase *, std::string> &lookUp) const {
  lookUp.clear();
  for (auto &i:query) {
    if (i.op == Variable) {
      lookUp[i.mem] = "";
    }
  }

}

void QueryGenerator::addOp(const char *op) {
  Operator o = Variable;
  if (strlen(op) <= 2) {
    switch (op[0]) {
      case '<':
        switch (op[1]) {
          case '\0': o = Less; break;
          case '=': o = LessEqual; break;
          case '>': o = NotEqual; break;
          default: ;
        }
        break;
      case '>':
        switch (op[1]) {
          case '\0': o = Grater; break;
          case '=': o = GraterEqual; break;
          case '<': o = NotEqual; break;
          default: ;
        }
        break;
      case '=':
        switch (op[1]) {
          case '\0':
          case '=': o = Equal; break;
          case '>': o = GraterEqual; break;
          default: ;
        }
        break;
      case '!':
        switch (op[1]) {
          case '\0': o = Not; break;
          case '=': o = NotEqual; break;
          default: ;
        }
        break;
      case 'I':
      case 'i':
        switch (op[1]) {
          case 'N':
          case 'n': o = InBegin; break;
          case 'B':
          case 'b': o = Between; break;
          default: ;
        }
        break;
      case 'N':
      case 'n':
        switch (op[1]) {
          case 'N':
          case 'n': o = IsNotNull; break;
          case 'U':
          case 'u': o = IsNull; break;
          default: ;
        }
        break;
      default: ;
    }
  }
  else {
    std::string c = mobs::toUpper(op);
    if (c == "LIKE")
      o = Like;
    else if (c == "BETWEEN")
      o = Between;
    else if (c == "ISNULL")
      o = IsNull;
    else if (c == "ISNOTNULL")
      o = IsNotNull;
  }

  if (o == Variable)
    THROW("Invalid Operator " << op);
  add(o);
}




}
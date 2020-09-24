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

/** \file querygenerator.h
 \brief Datenbank-Interface: Generator für Bedingung einer Datenbankabfrage
 */


#ifndef MOBS_QUERYGENERATOR_H
#define MOBS_QUERYGENERATOR_H

#include <string>

#include "objgen.h"

namespace mobs {

class QueryGeneratorData;

/** \brief Klasse zum Erzeugen eines Filters für Datenbankabfragen

 Die Filterbedngung wird schrittweise über den operator<<() erzeugt. Dabei stehen die Qi*-Methoden der Membervariablen
 zur Verfügung. Es können aber auch die Einzelnen Schritte einzeln zusammengestellt werden.

 Mit Hilfe des literal-operators kann auch ein freier Text für eine Bedingung generiert werden. Dieser kann auch Teil
 einer And/Or-Kette sein. Solche Filter sind jedoch Datenbank-Spezifisch und nicht portabel.

 \code
 using Q = mobs::QueryGenerator;    // Erleichtert die Tipp-Arbeit
 Q filter;
 Q << Q::AndBegin << kunde.Nr.Qi("!=", 7)
   << Q::OrBegin << Q::Not << kunde.Ort.Qi("LIKE", "%Nor_heim") << kunde.Ort.Qi("=", "Nordheim") << Q::OrEnd
   << kunde.eintritt.QiBetween("2020-01-01", "2020-06-30") << Q::Not << kunde.status.QiIn({7,8,3}) << Q::AndEnd;
 \endcode

 Ein einfacher Konstruktor ist ein leerer Filter und sucht alle Elemente der Datenbank
 */
class QueryGenerator {
public:
  /// Operator eines QueryItems
  enum Operator { Variable,     ///< Item ist eine Membervariable
                  Const,        ///< Item ist eine Konstante
                  Equal,        ///< Operator ==
                  Less,         ///< Operator <
                  LessEqual,    ///< Operator <=
                  Grater,       ///< Operator >
                  GraterEqual,  ///< Operator >=
                  NotEqual,     ///< Operator !=
                  Like,         ///< Operator SQL-Like
                  Between,      ///< Operator SQL-Between
                  InBegin,      ///< Operator ist Element von (Listen Begin)
                  InEnd,        ///< Operator ist Element von (Listen Ende)
                  IsNull,       ///< Operator ist null
                  IsNotNull,    ///< Operator ist nicht null
                  Not,          ///< Operator unäres Nicht
                  AndBegin,     ///< Beginn einer Und-verknüpften Bedingungsliste
                  AndEnd,       ///< Ende einer Und-verknüpften Bedingungsliste
                  OrBegin,      ///< Beginn einer Oder-verknüpften Bedingungsliste
                  OrEnd,        ///< Ende einer Oder-verknüpften Bedingungsliste
                  literalBegin, ///< Beginn eines literalen Kommandos, Variablen und Konstanten werden als Text übernommen
                  literalEnd,   ///< Ende eines literalen Kommandos
                  };

  template<typename T, class C>
  friend QueryGenerator &operator<<(QueryGenerator &g, Member<T, C> &m);
  friend QueryGenerator &operator<<(QueryGenerator &g, QueryInfo q);
  friend QueryGenerator &operator<<(QueryGenerator &g, bool b);
  friend QueryGenerator &operator<<(QueryGenerator &g, Operator o);
  friend QueryGenerator &operator<<(QueryGenerator &g, const std::string& s);
  friend QueryGenerator &operator<<(QueryGenerator &g, const char *s);
  friend QueryGenerator &operator<<(QueryGenerator &g, int64_t i);
  friend QueryGenerator &operator<<(QueryGenerator &g, uint64_t i);
  friend QueryGenerator &operator<<(QueryGenerator &g, int32_t i);
  friend QueryGenerator &operator<<(QueryGenerator &g, bool b);

  /** \brief  Einzelner Schritt eines Abfrage-Filters
   *
   */
  class QueryItem : public MobsMemberInfoDb {
  public:
    QueryItem() = default;
    /// Konstruktor für einen Operator
    explicit QueryItem(QueryGenerator::Operator op) : MobsMemberInfoDb(), op(op) {}
    /// Konstruktor für eine Konstante
    explicit QueryItem(const MobsMemberInfoDb &mi) : MobsMemberInfoDb(mi), op(Const) {}
    /// \private
    const mobs::MemberBase *mem = nullptr;
    /// \private
    QueryGenerator::Operator op = QueryGenerator::Variable;
  };

  /// Konstruktor
  QueryGenerator();
  ~QueryGenerator();

  /// \private
  void createLookup(std::map<const MemberBase *, std::string> &lookUp) const;

  /// \private
  std::string show(const std::map<const MemberBase *, std::string> &lookUp) const;






  /// Liste der einzelnen Schritte
  std::list<QueryItem> query;

private:
  void addOp(const char *op);
  void add(bool i);
  void add(uint64_t i);
  void add(const mobs::MemberBase &mem);
  void add(const mobs::MobsMemberInfoDb &mi);
  void add(Operator op);
  void add(const std::string &s);
  void add(int64_t i);
  QueryGeneratorData *data = nullptr;
};



template<typename T, class C>
/// Füge eine Membervariable zu einem Abfragefilter hinzu
QueryGenerator &operator<<(QueryGenerator &g, Member<T, C> &m) { g.add(m); return g; }

/// Füge einen Qi*-Bedingung einer Membervariable zu einem Abfragefilter hinzu
QueryGenerator &operator<<(QueryGenerator &g, QueryInfo q);

/// Füge einen Operator zu einem Abfragefilter hinzu
QueryGenerator &operator<<(QueryGenerator &g, QueryGenerator::Operator o);

/// Füge eine Konstante zu einem Abfragefilter hinzu
QueryGenerator &operator<<(QueryGenerator &g, const std::string& s);

/// Füge eine Konstante zu einem Abfragefilter hinzu
QueryGenerator &operator<<(QueryGenerator &g, const char *s);

/// Füge eine Konstante zu einem Abfragefilter hinzu
QueryGenerator &operator<<(QueryGenerator &g, int64_t i);

/// Füge eine Konstante zu einem Abfragefilter hinzu
QueryGenerator &operator<<(QueryGenerator &g, uint64_t i);

/// Füge eine Konstante zu einem Abfragefilter hinzu
QueryGenerator &operator<<(QueryGenerator &g, int32_t i);

/// Füge eine Konstante zu einem Abfragefilter hinzu
QueryGenerator &operator<<(QueryGenerator &g, bool b);




template<typename T, class C>
QueryInfo Member<T, C>::Qi(const char *oper, const T &value) const {
QueryInfo t(this, oper);
MobsMemberInfo mi;
memInfo(mi, value);
if (mi.isNumber())
  t.content.emplace_back(mi);
else {
  ConvToStrHint cth(hasFeature(mobs::DbCompact));
  t.content.emplace_back(C::c_to_string(value, cth));
}
return t;
}

template<typename T, class C>
QueryInfo Member<T, C>::Qi(const char *oper, const char *val) const {
T t;
if (not this->c_string2x(val, t, ConvFromStrHint::convFromStrHintExplizit))
  throw std::runtime_error("fromStrExplizit input error (Qi)");
return Qi(oper, t);
}

template<typename T, class C>
QueryInfo Member<T, C>::QiBetween(const T &value1, const T &value2) const {
ConvToStrHint cth(hasFeature(mobs::DbCompact));
QueryInfo t(this, "IB");
MobsMemberInfo mi1;
memInfo(mi1, value1);
if (mi1.isNumber())
  t.content.emplace_back(mi1);
else
  t.content.emplace_back(C::c_to_string(value1, cth));
MobsMemberInfo mi2;
memInfo(mi2, value2);
if (mi2.isNumber())
  t.content.emplace_back(mi2);
else
  t.content.emplace_back(C::c_to_string(value2, cth));
return t;
}

template<typename T, class C>
QueryInfo Member<T, C>::QiBetween(const char *value1, const char *value2) const {
T t1, t2;
if (not this->c_string2x(value1, t1, ConvFromStrHint::convFromStrHintExplizit) or
    not this->c_string2x(value2, t2, ConvFromStrHint::convFromStrHintExplizit))
  throw std::runtime_error("fromStrExplizit input error (Qi)");
return QiBetween(t1, t2);
}

template<typename T, class C>
QueryInfo Member<T, C>::QiIn(const std::list<T> &value) const {
ConvToStrHint cth(hasFeature(mobs::DbCompact));
QueryInfo t(this, "IN");
for (auto it:value) {
  MobsMemberInfo mi;
  memInfo(mi, it);
  if (mi.isNumber())
    t.content.emplace_back(mi);
  else
    t.content.emplace_back(C::c_to_string(it, cth));
}
return t;
}

template<typename T, class C>
QueryInfo Member<T, C>::QiIn(const std::vector<const char *>& value) const {
ConvToStrHint cth(hasFeature(mobs::DbCompact));
QueryInfo t(this, "IN");
for (auto &it:value) {
  MobsMemberInfo mi;
  T v;
  if (not this->c_string2x(it, v, ConvFromStrHint::convFromStrHintExplizit))
    throw std::runtime_error("fromStrExplizit input error (Qi)");
  memInfo(mi, v);
  if (mi.isNumber())
    t.content.emplace_back(mi);
  else
    t.content.emplace_back(C::c_to_string(v, cth));
}
return t;
}


}

#endif //MOBS_QUERYGENERATOR_H

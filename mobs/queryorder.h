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

/** \file queryorder.h
 \brief Datenbank-Interface: Generator für Sortier-Statement
 */

#ifndef MOBS_QUERYORDER_H
#define MOBS_QUERYORDER_H

#include "objgen.h"

namespace mobs {


class QuerySortData;

/** \brief Klasse zur Definition einer Sortierung für Datenbankabfragen

 Die standard Sortierreihenfole ist aufsteigend.

 Beispiel:
\code
 class Fahrzeug : public ObjectBase
 {
   public:
     ObjInit(Fahrzeug);

     MemVar(std::string, fahrzeugTyp);
     MemVar(int,         anzahlRaeder);
     MemVar(float,       leistung);
 };
 ...
 Fahrzeug f;
 QueryOrder sort;
 sort << f.leistung << QueryOrder::descending << f.anzahlRaeder;
 auto cursor = dbi.qbe(f2, sort);
\endcode

 */
class QueryOrder {
public:
  /// \private
  class SortSwitch {
  public:
    SortSwitch() = default;
  };

  /// \private
  QueryOrder();
  ~QueryOrder();

  /// \private
  void add(const mobs::MemberBase &mem);

  /// \private
  bool sortInfo(const mobs::MemberBase &mem, u_int &pos, int &dir) const;

  /// \private
  void directionAsc(bool);

  /// Sortierfolge aufsteigend
  static SortSwitch ascending;
  /// Sortierfolge absteigend
  static SortSwitch descending;

private:
  QuerySortData *data = nullptr;
};



template<typename T, class C>
/// Klasse zur Beschreibung von Membervariablen als Key-Element für einen Datenbank
QueryOrder &operator<<(QueryOrder &k, mobs::Member<T, C> &m) { k.add(m); return k; }

/// füge eine Sortier-Richtung hinzu
QueryOrder &operator<<(QueryOrder &k, QueryOrder::SortSwitch &s);


}
#endif //MOBS_QUERYORDER_H

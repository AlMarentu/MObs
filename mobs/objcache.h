// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2024 Matthias Lautner
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

#ifndef MOBS_OBJCACHE_H
#define MOBS_OBJCACHE_H


#include "objgen.h"

#include <iostream>
#include <string>

namespace mobs {

class ObjCacheData;
/** \brief Klasse zum cachen von Objekten die von mobs::ObjectBase abgeleitet sind
 *
 * In den Objekten muss mindestens ein KEYELEMENT defeiniert sein.
 * Wird über einem Object-Ident gesucht müssen die einzelnen Key-Elemente mittels escapeKey() verwendet werden, falls
 * diese Doppelpunkt oder Backslash enthalten können.
 */
class ObjCache
{
public:
  ObjCache();
  ~ObjCache();
  ObjCache(const ObjCache &) = delete;
  ObjCache &operator=(const ObjCache &) = delete;
  /** \brief lädt ein Objekt vom BasisTyp mobs::objectBase anhand vorausgefüllter Schlüsselfelder aus dem Cache
   *
   * @param obj Object mit gefüllten Key-Elementen zur Suche, wird mit Cache-Inhalt gefüllt
   * @return true, wenn vorhanden
   */
  bool load(ObjectBase &obj) const;
  /** \brief sucht ein Objekt vom BasisTyp mobs::objectBase anhand vorausgefüllter Schlüsselfelder aus dem Cache
   *
   * Es wird lediglich die Key-Information geprüft, nicht der gesamte Objhektinhalt
   * @param obj  Object mit gefüllten Key-Elementen zur Suche
   * @return true, wenn im Cache vorhanden
   */
  bool exists(const ObjectBase &obj) const;
  /** \brief sucht ein Objekt vom Typ mobs::objectBase anhand vorausgefüllter Schlüsselfelder aus dem Cache
   *
   * @param objIdent  Ident bestehend aus ObjectType + Key-Elementen analog objNameKeyStr()
   * @return shared_ptr mit gefundenem Objekt oder nullptr wenn nicht gefunden
   */
   std::shared_ptr<const ObjectBase> searchObj(const std::string &objIdent) const;
  /** \brief sucht ein Objekt vom BasisTyp mobs::objectBase anhand vorausgefüllter Schlüsselfelder aus dem Cache
   *
   * @param objIdent  Ident bestehend aus Key-Elementen analog objNameKeyStr(), der ObjectType wird automatisch vorangestellt
   * @return shared_ptr mit gefundenem Objekt oder nullptr wenn nicht gefunden
   */
   template <class T>
   std::shared_ptr<const T> search(const std::string &objIdent) const {
     return std::dynamic_pointer_cast<const T>(searchObj(STRSTR(T::objName() << ':' << objIdent)));
   }

  /** \brief speichert eine Kopie des Objektes im Cache
   *
   * wenn der Datensatz bereits existiert wird es ersetzt
   * @param obj Objekt das im Cache abgelegt wird
   */
  void save(const ObjectBase &obj);
  /** \brief speichert ein (const) Objekte im Cache
   *
   * Dabei wird ein neuer shared_ptr erzeugt. shared_ptr, die auf das vormalige Objekt verweisen bleiben unberührt.
   *
   * @param obj uniq Zeiger auf das Objekt; obj wird im Anschluß mit nullptr gefüllt (move)
   * @return shared Zeiger auf das gespeicherte Objekt
   */
  template <class T>
  std::shared_ptr<const T> save(std::unique_ptr<const T> &obj) {
    auto p = std::shared_ptr<const ObjectBase>(obj.release());
    save(p);
    return std::dynamic_pointer_cast<const T>(p);
  }
  /** \brief speichert ein Objekte im Cache
    *
    * Dabei wird ein neuer shared_ptr erzeugt. shared_ptr, die auf das vormalige Objekt verweisen bleiben unberührt.
    *
    * @param obj uniq Zeiger auf das Objekt; obj wird im Anschluß mit nullptr gefüllt (move)
    * @return shared Zeiger auf das gespeicherte Objekt
    */
  template <class T>
  std::shared_ptr<const T> save(std::unique_ptr<T> &obj) {
    auto p = std::shared_ptr<const ObjectBase>(obj.release());
    save(p);
    return std::dynamic_pointer_cast<const T>(p);
  }

  /** \brief speichert ein Objekte im Cache
   *
   * Dabei wird der neue shared_ptr gespeichert. shared_ptr, die auf das vormalige Objekt verweisen bleiben unberührt.
   *
   * @param obj uniq Zeiger auf das Objekt; obj wird im Anschluß mit nullptr gefüllt (move)
   * \throw runtime_error im Fehlerfall
   */
  void save(std::shared_ptr<const ObjectBase> &obj);
   /** \brief reduziere Chache auf Größe n
    *
    * die Verdrängung erfolgt nach last recent used
    * @param n
    * @return tatsächliche Größe des Caches
    */
  size_t reduce(size_t n);
  /** \brief escape-Function, die ':' und '\\' für die Suche escaped
   *
   * @param key Key-Value
   * @return  Key mit der alle nötigen Zeichen escaped
   */
  static std::string escapeKey(const std::string &key); // implementiert in objgen.cc

protected:
ObjCacheData *data{};
};

}

#endif // MOBS_OBJCACHE_H
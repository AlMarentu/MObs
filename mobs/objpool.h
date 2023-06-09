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


/** \file objpool.h
\brief Klassen für Object-Pool - benamte Objekte minimale in memory Datenbank oder Objekt-Cache  */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"
#ifndef MOBS_OBJPOOL_H
#define MOBS_OBJPOOL_H

#include <string>
#include <list>
#include <memory>
#include <utility>
#include <stdexcept>

namespace mobs {

/// Basisklasse um  beliebige Objekte als "named object" zu verwalten
class NamedObject
{
  public:
//    virtual std::string objName() const = 0;
    virtual ~NamedObject() = default;;
    /// \private
    bool nOdestroyed() const { return not valid; };
    /// \private
    void setNOdestroyed() {  valid = false; };
  private:
    bool valid = true;
};

class NOPData;
/// Klasse für Einen Pool um Objekte zu speichern, die von NamedObject und ObjectBase abgeleitet sind
class NamedObjPool
{
public:
  NamedObjPool();
  ~NamedObjPool();
  NamedObjPool(const NamedObjPool &) = delete;
  NamedObjPool &operator=(const NamedObjPool &) = delete;

  /// \private
  void assign(const std::string &objName, std::shared_ptr<NamedObject> obj);
  /// \private
  bool lookup(const std::string &objName, std::weak_ptr<NamedObject> &ptr);
  /// \private
  void search(const std::string &searchName, std::list<std::pair<std::string, std::weak_ptr<NamedObject>>> &result);
  /// Entfernt  Objekte die nicht in Verwendung sind \c shared_ptr<T>
  void clearUnlocked();
private:
  std::unique_ptr<NOPData> data;
};

template <class T>
/** \brief Objektreferenz die in einem Datenpool über einen Namen (ID) verwaltet wird
 \tparam T Klasse für die die Referenz ist
 
 Verwendung;
 
 \code
 
 class Fahrzeug : virtual public NamedObject, virtual public ObjectBase
 {
   public:
     ObjInit(Fahrzeug);
     MemVar(int, id);
     MemVar(string, typ);
 };
 ObjRegister(Fahrzeug);

 ...
 // Einen Pool erzeugen
 shared_ptr<NamedObjPool> pool = make_shared<NamedObjPool>();

 // pointer auf ein Objekt erzeugen (pool, ID)
 NamedObjRef<Fahrzeug> f1(pool, "1");
 // Objekt erzeugen
 f1.create();
 // Objekt verwenden
 f1->id(1);
 ...
 NamedObjRef<Fahrzeug> f2(pool, "1");
 // f1 und f2 referenzieren das selbe Objekt
 
 \endcode
 
 \todo Suchen einer liste<NameObjRef> aus dem Pool, über eine regexp
 \todo Anpassungen um die Zugriffe thread safe zu gestalten

 */
class NamedObjRef
{
public:
  /// Konstruktor für eine Named Object
  /// @param nOPool Zeiger auf den zugehörigen Daten-Pool
  /// @param objName Name des Objektes unter dem es abgelegt werden soll
    NamedObjRef(std::shared_ptr<NamedObjPool> nOPool, std::string objName) : pool(std::move(nOPool)), name(std::move(objName)) {
      pool->lookup(name, ptr);
    };
  /// \private
    NamedObjRef(std::shared_ptr<NamedObjPool> nOPool, std::string objName, std::weak_ptr<NamedObject> &p) : pool(std::move(nOPool)), name(std::move(objName)), ptr(p) {};
    ~NamedObjRef() = default;;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-unconventional-assign-operator"
  /// Zuweisung eines Objektes per C-Pointer
      T *operator=(T *t) {
      auto tmp = std::shared_ptr<T>(t);
      pool->assign(name, tmp);
      ptr = tmp;
      return t;
    };
#pragma clang diagnostic pop
  /// Erzeuge ein neues Objekt, überschreibt, falls bereits vorhanden
    std::shared_ptr<T> create() {
      auto tmp = std::make_shared<T>();
      pool->assign(name, tmp);
      return tmp;
    };
  /// liefert einen \c std::shared_ptr auf das Objekt
    std::shared_ptr<T> lock() const {
      if ((ptr.expired() or ptr.lock()->nOdestroyed()) and not pool->lookup(name, ptr))
        return nullptr;
      return std::dynamic_pointer_cast<T>(ptr.lock());
    };
  /// prüft ob ein Objekt existiert
    bool exists() const { return bool(lock()); };
    /// Zugriffsoperator *
    T &operator*() {
      auto t = lock();
      if (t == nullptr)
        throw std::runtime_error(std::string("NamedObject ") + name + " nullptr access");
      return *t;
    };
  /// Zugriffsoperator ->
    T *operator->() {
      auto t = lock();
      if (t == nullptr)
        throw std::runtime_error(std::string("NamedObject ") + name + " nullptr access");
      T *p = t.get();  

      return p;
    };
    /// const Zugriffsoperator ->
    const T *operator->() const {
      auto t = lock();
      if (t == nullptr)
        throw std::runtime_error(std::string("NamedObject ") + name + " nullptr access");
      T *p = t.get();

      return p;
    };
    /// const Zugriffsoperator *
    const T &operator*() const {
      auto t = lock();
      if (t == nullptr)
        throw std::runtime_error(std::string("NamedObject ") + name + " nullptr access");
      return *t;
    };
  /// Pool abfragen
  std::shared_ptr<NamedObjPool> getPool() const { return pool; }
  /// Objektname abfragen
  std::string getName() const { return name; }
  
  private:
    std::shared_ptr<NamedObjPool> pool;
    std::string name;
    mutable std::weak_ptr<NamedObject> ptr;

};

template <class T>
/// Liste von \c NamedObjRef
class NamedObjList : public std::list<NamedObjRef<T>> {
public:
  NamedObjList() : std::list<NamedObjRef<T>>() {}
  
  /// Sucht nach Objekten, die einen gleichen Teil zu Beginn des Objektnamens haben
  /// @param pool Zeiger auf Pool, aus dem gesucht werden soll
  /// @param searchName Suchmuster
  /// \throws runtime_error falls Methode nicht implementiert (unordered pool)
  void searchBeginsWith(std::shared_ptr<NamedObjPool> pool, const std::string &searchName) {
    std::list<std::pair<std::string, std::weak_ptr<NamedObject>>> result;
    pool->search(searchName, result);
    this->clear();
    for (auto &i:result)
      this->emplace(this->end(), pool, i.first, i.second);
  }
};

}

#endif

#pragma clang diagnostic pop
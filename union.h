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

/** \file union.h
 \brief Deklaration um variable Objektyben  analog Union zu generieren */


#ifndef MOBS_UNION_H
#define MOBS_UNION_H

#include <string>

#include "logging.h"
#include "objtypes.h"
#include "objgen.h"

namespace mobs {


template <class T>
/** \brief Klasse um verschiedene Objekte einer Basisklasse in einem Mobs-Vektor zu speichern
 
 \see MemVar
 \code
 
 // Deklaration einer Basisklasse
class BaseObj : virtual public mobs::ObjectBase {
public:
  ObjInit(BaseObj);
  virtual ~BaseObj() {};
 
 virtual Obj0 &toObj0() { throw runtime_error("invalid cast"); };
 virtual Obj1 &toObj1() { throw runtime_error("invalid cast"); };
 virtual Obj2 &toObj2() { throw runtime_error("invalid cast"); };
};

 // Deklaration Variante 0
class Obj0 : virtual public BaseObj, virtual public mobs::ObjectBase {
public:
  ObjInit(Obj0);

  MemVar(int, aa);
  MemVar(int, bb);
  virtual Obj0 &toObj0() { return *this; };
};
 // Objekt muss registriert werden
ObjRegister(Obj0);

 // Deklaration Variante 0
class Obj1 : virtual public BaseObj, virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj1);
  MemVar(int, i1,);
  MemVar(std::string, xx);
  MemObj(Obj0, oo, USENULL KEYELEMENT2);
  virtual Obj1 &toObj1() { return *this; };
};
ObjRegister(Obj1);

class Obj2 : virtual BaseObj, virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj2);
  MemVar(int, id);
  MemVar(int, xx);
  virtual Obj2 &toObj2() { return *this; };
};
ObjRegister(Obj2);

// Objekt, in dem die Basisklasse verwendet wird
class Master : virtual public mobs::ObjectBase {
  public:
 
  ObjInit(Master);
  MemVar(int, id, KEYELEMENT1);
  MemVar(int, abcd);
 
 //------ Basisklasse muss als Vektor deklariert werden
  MemVector(mobs::MobsUnion<BaseObj>, elements, USENULL);
 
 
 Master m
 ...
 // Zugriff:
 if (m.elements[1]);
   m.elements[1]->toObj1().i1(12));
 cout << m.elements[1]().toObj1().i1();
};
\endcode
*/
class MobsUnion : virtual public mobs::ObjectBase {
public:
  /// \private
  ObjInit(MobsUnion);
  ~MobsUnion() { if (obj) delete obj; obj = nullptr; }
  /// liefert den Objekttyp des Unions oder \e leer  wenn nicht gesetzt
  std::string type() const { if (obj) return obj->typName(); return ""; }
  /// setzt den Objekttyp des Unions, keine Aktion benn der Typ bereits passt
  /// @param t Objekttyp (muss mit \c ObjRegister) registriert sein; ist t leer, so wird das Objekt gelöscht
  /// \throw runtime_erroe wenn der Objekttyp nicht von der Basisklasse \c T abgeleitet ist
  void setType(std::string t);
  /// Übernehme eine Kopie des angegebenen Objektes
  void operator() (const T &t) { setType(t.typName()); obj->doCopy(t); activate(); }
  /// const Zugriffsmethode auf die Basisklasse
  const T *operator-> () const { return dynamic_cast<T *>(obj); }
  /// Prüfen, ob  Objekt gesetzt
  operator bool () const { return obj; }
  /// nicht const Zugriffsmethode auf die Basisklasse
  T *operator-> () { return dynamic_cast<T *>(obj); }
  /// nicht const Zugriffsmethode auf die Basisklasse mit exception
  T &operator() () { T *t = dynamic_cast<T *>(obj); if (not t) throw std::runtime_error("invalid cast"); return *t; }
  /// const Zugriffsmethode auf die Basisklasse mit exception
  const T &operator() () const  { T *t = dynamic_cast<T *>(obj); if (not t) throw std::runtime_error("invalid cast"); return *t; }
  /// \private
  virtual ObjectBase *getObjInfo(const std::string &name) { setType(name); return obj; }
  /// \private
  virtual void cleared() { if (obj) delete obj; obj = nullptr; }
  /// Starte Traversierung nicht const
  void traverse(ObjTrav &trav);
  /// Starte Traversierung  const
  void traverse(ObjTravConst &trav) const;
  
private:
  ObjectBase *obj = 0;
};




template <class T>
void MobsUnion<T>::setType(std::string t) {
  if (t.empty()) {
    clear();
    return;
  }
  if (obj == nullptr or obj->typName() != t) {
    if (obj) delete obj;
    obj = ObjectBase::createObj(t, this);
    if (not dynamic_cast<T *>(obj)) {
      cleared();
      throw std::runtime_error(u8"MobsUnion invalid object " + t);
    }
    activate();
  }
}


template <class T>
void MobsUnion<T>::traverse(ObjTrav &trav) {
  size_t arrayIndex = trav.arrayIndex;
  if (trav.doObjBeg(*this)) {
    if (obj) {
      trav.arrayIndex = SIZE_MAX;
      obj->traverse(trav);
    }
    trav.arrayIndex = arrayIndex;
    trav.doObjEnd(*this);
  }
}

template <class T>
void MobsUnion<T>::traverse(ObjTravConst &trav) const {
  bool inNull = trav.inNull;
  size_t arrayIndex = trav.arrayIndex;
  if (trav.doObjBeg(*this)) {
    if (obj) {
      trav.inNull = inNull or isNull();
      trav.arrayIndex = SIZE_MAX;
      obj->traverse(trav);
    }
    trav.inNull = inNull;
    trav.arrayIndex = arrayIndex;
    trav.doObjEnd(*this);
  }
}


}

#endif

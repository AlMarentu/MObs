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
 \brief Deklaration um variable Objekttypen  analog Union zu generieren */


#ifndef MOBS_UNION_H
#define MOBS_UNION_H

#include <string>

#include "objtypes.h"
#include "objgen.h"

namespace mobs {


template <class T>
/** \brief Klasse um verschiedene Objekte einer Basisklasse in einem Mobs-Vektor zu speichern
 
 \see MemVector
 
 Beispiel:
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

 // Deklaration Variante 1
class Obj0 : virtual public BaseObj, virtual public mobs::ObjectBase {
public:
  ObjInit(Obj0);

  MemVar(int, aa);
  MemVar(int, bb);
  Obj0 &toObj0() override { return *this; };
};
 // Objekt muss registriert werden
ObjRegister(Obj0);

 // Deklaration Variante 2
class Obj1 : virtual public BaseObj, virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj1);
  MemVar(int, i1,);
  MemVar(std::string, xx);
  MemObj(Obj0, oo, USENULL KEYELEMENT2);
  Obj1 &toObj1() override{ return *this; };
};
ObjRegister(Obj1);

// Deklaration Variante 3
class Obj2 : virtual BaseObj, virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj2);
  MemVar(int, id);
  MemVar(int, xx);
  Obj2 &toObj2() override { return *this; };
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
  ObjInit1(MobsUnion);
  /// \private
  MobsUnion(const MobsUnion &that) : ObjectBase() { TRACE(""); MobsUnion::doCopy(that); }
  ~MobsUnion() override { if (m_obj) delete m_obj; }
  /// liefert den Objekttyp des Unions oder \e leer  wenn nicht gesetzt
  std::string type() const { if (m_obj) return m_obj->typeName(); return ""; }
  /// setzt den Objekttyp des Unions, keine Aktion benn der Typ bereits passt
  /// @param t Objekttyp (muss mit \c ObjRegister) registriert sein; ist t leer, so wird das Objekt gelöscht
  /// \throw runtime_error wenn der Objekttyp nicht von der Basisklasse \c T abgeleitet ist
  void setType(const std::string& t);
  /// Übernehme eine Kopie des angegebenen Objektes
  void operator() (const T &t) { setType(t.typeName()); m_obj->doCopy(t); activate(); }
  /// const Zugriffsmethode auf die Basisklasse
  const T *operator-> () const { return m_obj; }
  /// Prüfen, ob  Objekt gesetzt
  explicit operator bool () const { return m_obj; }
  /// nicht const Zugriffsmethode auf die Basisklasse
  T *operator-> () { return m_obj; }
  /// nicht const Zugriffsmethode auf die Basisklasse mit exception
  T &operator() () { if (not m_obj) throw std::runtime_error("MobsUnion is empty"); return *m_obj; }
  /// const Zugriffsmethode auf die Basisklasse mit exception
  const T &operator() () const  { if (not m_obj) throw std::runtime_error("MobsUnion is empty"); return *m_obj; }
  /// \brief Überladenen Methode, die das gewünschte MobsUnion-Objekt \c name zuerst zu erzeugen versucht
  ObjectBase *getObjInfo(const std::string &name) override { setType(name); return m_obj; }
  /// \private
  void doCopy(const ObjectBase &other) override;
  /// \private
  void cleared() final { if (m_obj) {delete m_obj; regObj(nullptr); } }
  
private:
  T *m_obj = nullptr;
};




template <class T>
void MobsUnion<T>::setType(const std::string& t) {
  // hier wird als einziges ein neues Objekt erzeugt oder entfernt
  // es muss immer m_obj mit ObjectBase.mlist synchron gehalten werden
  if (t.empty()) {
    if (not m_obj)
      return;
    delete m_obj;
    regObj(nullptr); // ObjectBase.mlist clear
    m_obj = nullptr; // immer mit ObjectBase.mlist synchron halten
    clear(); // wegen null-handling, Achtung ruft hinterher cleared() auf
    return;
  }
  if (m_obj == nullptr or m_obj->typeName() != t) {
    if (m_obj) delete m_obj;
    regObj(nullptr); // ObjectBase.mlist clear
    // wird über Constructor in Obj-Liste von ObjectBase eingefügt
    ObjectBase *o = ObjectBase::createObj(t, this);
    m_obj = dynamic_cast<T *>(o);
    if (not m_obj) {
      regObj(nullptr); // ObjectBase.mlist clear
      if (o) LOG(LM_WARNING, "MobsUnion::setType " << t << " is not a valid base class");
      delete o;
      clear(); // wegen null-handling, Achtung ruft hinterher cleared() auf
    }
    else
      activate();
  }
}

template <class T>
void MobsUnion<T>::doCopy(const ObjectBase &other) {
  auto that = dynamic_cast<const MobsUnion<T> *>(&other);
  if (not that)
    throw std::runtime_error(u8"ObjectBase::doCopy: invalid Element (expected MobsUnion)");
  setType(that->type());
  ObjectBase::doCopy(other);
}


}

#endif

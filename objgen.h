// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs
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


#ifndef MOBS_OBJGEN_H
#define MOBS_OBJGEN_H

#include <list>
#include <map>
#include <vector>
#include <stack>
#include <limits>

#include "logging.h"
#include "objtypes.h"

#define MemVar(typ, name) Member<typ> name = Member<typ>(#name, this)
#define MemVector(typ, name) MemberVector<Member<typ> > name = MemberVector<Member<typ> >(#name, this)
#define ObjVector(typ, name) MemberVector<typ> name = MemberVector<typ>(#name, this)
#define ObjVar(typ, name) typ name = typ(#name, this)
#define ObjInit(objname) \
  objname() { TRACE(""); keylist.reset(); init(); }; \
  objname(std::string name, ObjectBase *t) { TRACE(PARAM(name) << PARAM(this)); if (t) t->regObj(this); m_varNam = name; }; \
  objname &operator=(const objname &other) { TRACE(""); \
    if (this != &other) { \
      doCopy(other); \
    } \
    return *this; \
  }; \
  static ObjectBase *createMe() { return new objname; } ; \
  virtual std::string typName() const { return #objname; };

#define ObjRegister(name) \
namespace  { \
class RegMe##name { \
public: \
  RegMe##name() { ObjectBase::regObject(#name, name::createMe); }; \
  static RegMe##name regme; \
}; \
RegMe##name RegMe##name::regme; \
}


class ObjTrav;
class ObjTravConst;

class KeyList {
  public:
    KeyList() {};
    int add() { return ++cnt; };
    void reset() { cnt = 0; };
  private:
    int cnt = 0;
};

class NullValue {
  public:
    bool isNull() const { return m_null and m_nullAllowed; };
    void setNull(bool n) { m_null = n; } ;
    void nullAllowed(bool on) { m_nullAllowed = on; };
    bool nullAllowed() const { return m_nullAllowed; };
  private:
    bool m_null = false;
    bool m_nullAllowed = false;
};

class MemberBase : public NullValue {
public:
  MemberBase(std::string n) : m_name(n) {};
  virtual ~MemberBase() {};
  std::string name() const { return m_name; };
  virtual void strOut(std::ostream &str) const  = 0;
  virtual std::string toStr() const  = 0;
  virtual bool is_specialized() const  = 0;
  virtual bool is_chartype() const = 0;
  virtual bool fromStr(const std::string &s) = 0;
  void traverse(ObjTrav &trav);
  void traverse(ObjTravConst &trav) const;
  void key(int k) { m_key = k; };
  int key() const { return m_key; };

protected:
  std::string m_name;
  int m_key = 0;
};




class ObjectBase;

class MemBaseVector : public NullValue {
  public:
    MemBaseVector(std::string n) : m_name(n) { TRACE(PARAM(n)); };
    virtual ~MemBaseVector() { TRACE(""); };
    virtual void traverse(ObjTrav &trav) = 0;
    virtual void traverse(ObjTravConst &trav) const = 0;
    virtual size_t size() const = 0;
    virtual void resize(size_t s) = 0;
    std::string name() const { return m_name; };
    virtual void doCopy(const MemBaseVector &other) = 0;
    virtual MemberBase *getMemInfo(size_t i) = 0;
    virtual ObjectBase *getObjInfo(size_t i) = 0;
  protected:
    std::string m_name;
};

class ObjectBase : public NullValue {
  public:
  ObjectBase() {};
  virtual ~ObjectBase() {};
  void regMem(MemberBase *mem);
  void regObj(ObjectBase *obj);
  void regArray(MemBaseVector *vec);
  //void regMemList(std::list<MemberBase> *m, std::string n);
  void traverse(ObjTrav &trav);
  void traverse(ObjTravConst &trav) const;
  virtual std::string typName() const { return ""; };
  /// wird einmalig im Constructor aufgerufen
  virtual void init() {};
  std::string name() const { return m_varNam; };
  MemberBase &get(std::string name);
  MemberBase *getMemInfo(const std::string &name);
  ObjectBase *getObjInfo(const std::string &name);
  MemBaseVector *getVecInfo(const std::string &name);
  
  static void regObject(std::string n, ObjectBase *fun());
  static ObjectBase *createObj(std::string n);
  void doCopy(const ObjectBase &other);

  protected:
    std::string m_varNam;
    KeyList keylist;
  private:
    class MlistInfo {
      public:
        MlistInfo(MemberBase *m, ObjectBase *o, MemBaseVector *v) : mem(m), obj(o), vec(v) {};
        MemberBase *mem = 0;
        ObjectBase *obj = 0;
        MemBaseVector *vec = 0;
    };
    std::list<MlistInfo> mlist;
    static std::map<std::string, ObjectBase *(*)()> createMap;

};

// ist typename ein Character-Typ
template <typename T>
inline bool mobschar(T) { return not std::numeric_limits<T>::is_specialized; };
template <> inline bool mobschar(char) { return true; };
template <> inline bool mobschar(char16_t) { return true; };
template <> inline bool mobschar(char32_t) { return true; };
template <> inline bool mobschar(wchar_t) { return true; };
template <> inline bool mobschar(unsigned char) { return true; };
template <> inline bool mobschar(signed char) { return true; };


//template <typename T>
//inline T mobsempty(T&) { return T(); };
//template <> inline char mobsempty(char&) { return ' '; };
//template <> inline signed char mobsempty(signed char&) { return ' '; };
//template <> inline unsigned char mobsempty(unsigned char&) { return ' '; };


template<typename T>
class Member : virtual public MemberBase {
public:
  Member() : MemberBase(""), wert(T()) { TRACE(""); };  // Konstruktor für Array
  Member(std::string n, ObjectBase *o) : MemberBase(n), wert(T()) { TRACE(PARAM(n) << PARAM(this)); if (o) o->regMem(this); }; // Konst. f. Objekt
  Member &operator=(const Member &other) = delete;
  ~Member() { TRACE(PARAM(m_name)); };
  inline T operator() () const { return wert; };
  inline void operator() (const T &t) { TRACE(PARAM(this)); wert = t; };
  virtual void strOut(std::ostream &str) const { str << mobs::to_string(wert); };
//  virtual std::string toStr() const { std::stringstream s; s << wert; return s.str(); };
  virtual std::string toStr() const { return mobs::to_string(wert); };
//  virtual std::wstring toWStr2() const { return mobs::to_wstring(wert); };
  virtual bool is_specialized() const { return std::numeric_limits<T>::is_specialized; };
  virtual bool is_chartype() const { return mobschar(wert); };
  virtual bool fromStr(const std::string &sin) { return mobs::string2x(sin, wert); };
  void doCopy(const Member<T> &other) { operator()(other()); };
private:
  T wert;
};

template<typename T>
KeyList & operator<<(KeyList &k, Member<T> &m)
{ 
  TRACE(PARAM(m.name()) << PARAM(m.key()));
  m.key(k.add());
  return k;
};

template<class T>
class MemberVector : virtual public MemBaseVector {
  public:
    MemberVector(std::string n, ObjectBase *o) : MemBaseVector(n) { TRACE(PARAM(n) << PARAM(this)); o->regArray(this); };
    ~MemberVector() { TRACE(PARAM(m_name)); resize(0); };   // heap Aufräumen
    T &operator[] (size_t t) { if (t >= size()) resize(t+1); return *werte[t]; };
    virtual size_t size() const { return werte.size(); };
    virtual void resize(size_t s);
    virtual void traverse(ObjTrav &trav);
    virtual void traverse(ObjTravConst &trav) const;
    virtual MemberBase *getMemInfo(size_t i) { if (i >= size()) return 0; return dynamic_cast<MemberBase *>(werte[i]); };
    virtual ObjectBase *getObjInfo(size_t i) { if (i >= size()) return 0; return dynamic_cast<ObjectBase *>(werte[i]); };
  protected:
    void doCopy(const MemberVector<T> &other);
    void doCopy(const  MemBaseVector &other);
  private:
    // Vector von Heap-Elementen verwenden, da sonst Probleme beim Reorg
    std::vector<T *> werte;
};

class ObjTrav {
  public:
    virtual void doObjBeg(ObjTrav &ot, ObjectBase &obj) = 0;
    virtual void doObjEnd(ObjTrav &ot, ObjectBase &obj) = 0;
    virtual void doArrayBeg(ObjTrav &ot, MemBaseVector &vec) = 0;
    virtual void doArrayEnd(ObjTrav &ot, MemBaseVector &vec) = 0;
    virtual void doMem(ObjTrav &ot, MemberBase &mem) = 0;
};

class ObjTravConst {
  public:
    virtual void doObjBeg(ObjTravConst &ot, const ObjectBase &obj) = 0;
    virtual void doObjEnd(ObjTravConst &ot, const ObjectBase &obj) = 0;
    virtual void doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec) = 0;
    virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec) = 0;
    virtual void doMem(ObjTravConst &ot, const MemberBase &mem) = 0;
};

class ObjectInserter  {
public:
  /// Zeiger auf die aktuelle MemberVariable oder nullptr, falls es keine Variable ist
  inline MemberBase *member() const { return memBase; };
  /// Name der aktuellen MemberVariablen, oder leer, falls es keine Variable ist
  inline const std::string &showName() const { return memName; };
  /// Name des aktuell referenzierten Objektes/Variable, leer, falls der das Objekt abgearbeitet ist
  inline const std::string current() const { return path.empty() ? "" : path.top(); };
  /// Push eines Objektes auf den Objektstack, zum weiteren Abarbeiten
  /// @param obj Objekt-Referenz
  void pushObject(ObjectBase &obj, const std::string &name = "<obj>");
  /// gehe eine Ebene tiefer in die Objektstruktur, Rückgabe true, wenn das Element existiert
  /// @param element Name des Elementes
  bool enter(const std::string &element);
  /// Verlassen einer Ebene in der Objektstruktur; Exception, bei Struktur-Fehler
  /// @param element Name des Elementes oder leer, wenn der Name der Struktur nicht geprüft werden soll
void leave(const std::string &element = "");
  
private:
  class Objekt {
  public:
    Objekt(ObjectBase *o, const std::string &name) : obj(o), objName(name) { };
    ObjectBase *obj = 0;
    std::string objName;
  };
  std::stack<Objekt> objekte;
  std::stack<std::string> path;
  
  std::string memName;
  MemberBase *memBase = 0;
};

template<class T>
void MemberVector<T>::resize(size_t s)
{
  TRACE(PARAM(s));
  size_t old = size();
  if (old > s)
  {
    for (size_t i = s; i < old; i++)
      delete werte[i];
    werte.resize(s);
  }
  else if (old < s)
  {
    werte.resize(s);
    for (size_t i = old; i < s; i++)
      werte[i] = new T;
  }
}

template<class T>
void MemberVector<T>::traverse(ObjTrav &trav)
{
  trav.doArrayBeg(trav, *this);
  for (auto w:werte)
    w->traverse(trav);
  trav.doArrayEnd(trav, *this);
};

template<class T>
void MemberVector<T>::traverse(ObjTravConst &trav) const
{
  trav.doArrayBeg(trav, *this);
  for (auto w:werte)
    w->traverse(trav);
  trav.doArrayEnd(trav, *this);
};

template<class T>
void MemberVector<T>::doCopy(const MemberVector<T> &other)
{
  resize(other.size());
  size_t i = 0;
  for (auto const w:other.werte)
    operator[](i++).doCopy(*w);
};

template<class T>
void MemberVector<T>::doCopy(const MemBaseVector &other)
{
  const MemberVector<T> *t = dynamic_cast<const MemberVector<T> *>(&other);
  if (not t)
    throw std::runtime_error("MemberVector::doCopy invalid");
  doCopy(*t);
};

namespace mobs {
  std::string to_string(const ObjectBase &obj);
  //std::wstring to_wstring(const ObjectBase &obj);
}

#endif

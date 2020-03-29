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

/** \file objgen.h
 \brief Deklaration um die Zielklassen zu generieren */

/** \mainpage MObs C++ Bibliothek
 
 Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
 
 Lizenztyp: LGPL
 
 Die Grundbausteine:
 
 Das Grundelement ist ein Objekt bestehend aus einer Klasse die von \c ObjectBase abgeleitet ist.
 Dieses Objekt kann wiederum verschiedene Objekte oder Variablen von Basistypen enthalten.
 Außderdem können auch Vektoren dieser beiden Typen existieren.
 \code
 class Kontakt : virtual public mobs::ObjectBase {
 public:
   ObjInit(Kontakt); // Makro, das alle nötigen Konstruktoren liefert
   enum device { fax, sms, mobil, privat, arbeit };
   
   /// Art des Kontaktes Fax/Mobil/SMS
   MemVar(int, art);
   /// Nummer
   MemVar(string, number);
 };

 class Adresse : virtual public mobs::ObjectBase {
 public:
   ObjInit(Adresse); // Makro, das alle nötigen Konstruktoren liefert
   
   MemVar(std::string, strasse);
   MemVar(std::string, plz);
   MemVar(std::string, ort);
 };

 class Person : virtual public mobs::ObjectBase {
   public:
   ObjInit(Person); // Makro, das alle nötigen Konstruktoren liefert
   
   MemVar(int,            kundennr);  // int kundennr;
   MemVar(bool,           firma);     // bool firma;
   MemVar(std::string,    name);      // std::string name
   MemVar(std::string,    vorname);
   ObjVar(Adresse,        adresse);   // Adresse adresse;
   ObjVector(Kontakt,     kontakte);  // vector<Kontakt> kontakte;
   MemVector(std::string, hobbies);   // vector<std::string> hobbies
 };
 \endcode
 
 Der Zugriff auf die Elemente dieser Klassen erfolgt über get- und set-Methoden:
 \code
 Person p;
 p.name("Peter");
 p.adresse.ort("Tokyo");
 p.hobbies[2]("Tauchen");
 p.kontakt[3].number("+49 0000 00000");
 std::cout << p.adresse.ort() << " " << p.hobbies[2]();
 \endcode
 DIe Vectoren werden bei Zugriffen automatisch vergrößert, so dass eine Überprüfung entfallen kann.
 
 Zum Verarbeiten solcher Objekte gibt es verschiedene Methoden:
 \arg Rekursiver Durchlauf mit Hilfe einer Traversier-Klasse
 \arg Füllen über eine sequentiellen Pfad
 \see ObjTravConst
 \see ObjTrav
 \see ObjectInserter
 
  Es existieren Klassen, um solche Objekt von und nach XML oder JSON zu wandeln
 
 \todo Anpassungen um die Zugriffe threadsafe zu gestalten
 
 */



#ifndef MOBS_OBJGEN_H
#define MOBS_OBJGEN_H

#include <list>
#include <map>
#include <vector>
#include <stack>
#include <limits>

#include "logging.h"
#include "objtypes.h"

/*! \brief Deklarations-Makro für eine  Membervarieable
 @param typ Basistyp
 @param name Name
 */
#define MemVar(typ, name) mobs::Member<typ> name = mobs::Member<typ>(#name, this)
/*! \brief Deklarations-Makro für eines Vector von Membervarieablen
 @param typ Basistyp
 @param name Name
 */
#define MemVector(typ, name) mobs::MemberVector<mobs::Member<typ> > name = mobs::MemberVector<mobs::Member<typ> >(#name, this)
/*! \brief Deklarations-Makro für eines Vector von Objektvaliebalen
 @param typ Objekttyp
 @param name Name
 */
#define ObjVector(typ, name) mobs::MemberVector<typ> name = mobs::MemberVector<typ>(#name, this)
/*! \brief Deklarations-Makro für eine Objekt als Membervariable
 @param typ Objekttyp
 @param name Name
 */
#define ObjVar(typ, name) typ name = typ(#name, this)

/*! \brief Makro für Definitionen im Objekt das von ObjecBase ist
 @param objname Name der Klasse (muss von ObjecBase abgeleitet sein)
 */
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

/*! \brief Makro um eine Objektklasse am Ojekt-Generator anzumelden
 @param name Name des Objektes
 \see ObjectBase::createObj
 */
#define ObjRegister(name) \
namespace  { \
class RegMe##name { \
public: \
RegMe##name() { mobs::ObjectBase::regObject(#name, name::createMe); }; \
static RegMe##name regme; \
}; \
RegMe##name RegMe##name::regme; \
}

namespace mobs {

class ObjTrav;
class ObjTravConst;

/// \brief Interne Klasse zur Verwaltung der Schlüsselelenten bei Datenbankobjekten
/// \see KeyList
class KeyList {
public:
  KeyList() {};
  int add() { return ++cnt; };
  void reset() { cnt = 0; };
private:
  int cnt = 0;
};

/// Interne Klasse zur Behandlung von \c NULL \c -Werten
class NullValue {
public:
  /// Abfrage, ob eine Variable den Wet \c NULL hat (nur möglich, wenn \c nullAllowed gesetzt ist.
  bool isNull() const { return m_null and m_nullAllowed; };
  /// Setzmethode für Membevasriable ist \c NULL
  void setNull(bool n) { m_null = n; } ;
  /// Abfrage ob \c NULL -Werte erlaubt sind
  void nullAllowed(bool on) { m_nullAllowed = on; };
  /// Abfrage ob für diese Variable Null-Werte erlaubt sind
  bool nullAllowed() const { return m_nullAllowed; };
private:
  bool m_null = false;
  bool m_nullAllowed = false;
};

/// Basisklasse für Membervariablen
class MemberBase : public NullValue {
protected:
  /// \private
  MemberBase(std::string n) : m_name(n) {};
public:
  virtual ~MemberBase() {};
  /// Abfrage des Namen der Membervariablen
  std::string name() const { return m_name; };
  virtual void strOut(std::ostream &str) const  = 0;
  /// Setze Inhalt auf leer
  virtual void clear() = 0;
  /// Ausgabe des Ihnhalts als \c std::string in UTF-8
  virtual std::string toStr() const  = 0;
  /// Abfrage ob der Basistyp vom typ \c  std::is_specialized  ist; \see \<limits>
  virtual bool is_specialized() const  = 0;
  /// Abfrage, ob der Inhalt textbasiert ist (zb. in JSON in Hochkommata gestzt wird)
  virtual bool is_chartype() const = 0;
  /// Einlesen der Variable aus einnem \c std::string im Format UTF-8
  virtual bool fromStr(const std::string &s) = 0;
  /// Starte Traversierung nicht const
  void traverse(ObjTrav &trav);
  /// Starte Traversierung  const
  void traverse(ObjTravConst &trav) const;
  /// private
  void key(int k) { m_key = k; };
  /// \brief Abfrage ob Memvervariable ein Key-Element ist
  /// @return Position im Schlüssel oder \c 0 wenn kein Schlüsselelement
  int key() const { return m_key; };
  
protected:
  /// \private
  int m_key = 0;
private:
  std::string m_name;
};




class ObjectBase;

/// \brief Basisklasse für Vectoren auf Membervariablen oder Objekten innerhalb von von \c ObjectBase angeleiteten Klasse; Bitte als Makro MemVector oder ObjVector verwenden
/// \see MemVector
/// \see ObjVector
class MemBaseVector : public NullValue {
public:
  /// \private
  MemBaseVector(std::string n) : m_name(n) { TRACE(PARAM(n)); };
  virtual ~MemBaseVector() { TRACE(""); };
  /// Starte Traversierung nicht const
  virtual void traverse(ObjTrav &trav) = 0;
  /// Starte Traversierung  const
  virtual void traverse(ObjTravConst &trav) const = 0;
  /// liefert die Anzahl der Elemente
  virtual size_t size() const = 0;
  /// Vergrößert/verkleinert die Elemtzahl des Vektors
  virtual void resize(size_t s) = 0;
  /// liefert den Namen der Vektor variablen
  std::string name() const { return m_name; };
  /// \private
  virtual void doCopy(const MemBaseVector &other) = 0;
  /// liefert einen Zeiger auf das entsprechende Element, falls es eine \c MemVar ist
  virtual MemberBase *getMemInfo(size_t i) = 0;
  /// liefert einen Zeiger auf das entsprechende Element, falls es eine \c MemObj ist
  virtual ObjectBase *getObjInfo(size_t i) = 0;
  /// Setze Inhalt auf leer; äquivalent zu \c resize(0)
  void clear() { resize(0); };
protected:
  /// \private
  std::string m_name;
};

/// Basisklasse für Objekte
class ObjectBase : public NullValue {
public:
  ObjectBase() {};
  virtual ~ObjectBase() {};
  /// \private
  void regMem(MemberBase *mem);
  /// \private
  void regObj(ObjectBase *obj);
  /// \private
  void regArray(MemBaseVector *vec);
  //void regMemList(std::list<MemberBase> *m, std::string n);
  /// Starte Traversierung nicht const
  void traverse(ObjTrav &trav);
  /// Starte Traversierung  const
  void traverse(ObjTravConst &trav) const;
  /// liefert den Typnamen des Objektes
  virtual std::string typName() const { return ""; };
  /// Callback-Funktion die einmalig im Constructor aufgerufen wird
  virtual void init() {};
  /// liefert den Namen Membervariablen
  std::string name() const { return m_varNam; };
  /// liefert ???
  MemberBase &get(std::string name);
  /// Suche eine Membervariable
  /// @param name Name der zu suchenden Variable
  /// @return Zeiger auf Objekt \c Memberbase oder \c nullptr
  MemberBase *getMemInfo(const std::string &name);
  /// Suche eine Member-Objekt
  /// @param name Name des zu suchenden Objektes
  /// @return Zeiger auf Objekt \c ObjecBase oder \c nullptr
  ObjectBase *getObjInfo(const std::string &name);
  /// Suche einen Vector von Membervariablen oder Objekten
  /// @param name Name der zu suchenden Variable
  /// @return Zeiger auf Objekt \c MemBaseVector oder \c nullptr
  MemBaseVector *getVecInfo(const std::string &name);
  
  /// \private
  static void regObject(std::string n, ObjectBase *fun());
  /// \brief Erzeuge ein neues Objekt
  /// @param n Typname des Objektes
  /// @return Zeiger auf das erzeugte Objekt oder \c nullptr falls ein solches Objekt nicht refgistriert wurde \see ObjRegister(name)
  static ObjectBase *createObj(std::string n);
  /// Setze Inhalt auf leer, d.h. alle Vektoren sowie Unterobjekte und Varieblen werden gelöscht,
  void clear();
  /// \brief Kopiere ein Objekt aus einem bereits vorhandenen.
  /// @param other zu Kopierendes Objekt
  /// \throw runtime_error Sind die Strukturen nicht identisch, wird eine Exception erzeugt
  void doCopy(const ObjectBase &other);

  
protected:
  std::string m_varNam;
  KeyList keylist; ///< Liste von Key-Elementen die innerhab \c init() gesetzt werden können \see KeyList
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
/// \brief prüft, ob der Typ T aus Text besteht - also zB. in JSON in Hochkommata steht
inline bool mobschar(T) { return not std::numeric_limits<T>::is_specialized; };
/// \private
template <> inline bool mobschar(char) { return true; };
/// \private
template <> inline bool mobschar(char16_t) { return true; };
/// \private
template <> inline bool mobschar(char32_t) { return true; };
/// \private
template <> inline bool mobschar(wchar_t) { return true; };
/// \private
template <> inline bool mobschar(unsigned char) { return true; };
/// \private
template <> inline bool mobschar(signed char) { return true; };


//template <typename T>
//inline T mobsempty(T&) { return T(); };
//template <> inline char mobsempty(char&) { return ' '; };
//template <> inline signed char mobsempty(signed char&) { return ' '; };
//template <> inline unsigned char mobsempty(unsigned char&) { return ' '; };


template<typename T>
/** \brief Klasse für Member-Variable zum angegeben Basistyp
 
 Diese Klasse wird normalerweise innerhalb einen Objektes  das von der Basisklasse  \c ObjectType abgeleitet verwendet.
 
 Zur Deklaration bitte das Makro \c MemVar verwenden
 \see MemVar
 \code
 class Fahrzeug : virtual public ObjectBase
 {
   public:
     ObjInit(Fahrzeug); // Makro zum Initialisieren, Name muss mit Klassennamen übereinstimmen

     MemVar(std::string, fahrzeugTyp);
     MemVar(int,         anzahlRaeder);
}
\endcode
  
   Zulässige Typen sind:
 \arg bool
 \arg char
 \arg char16_t
 \arg char32_t
 \arg wchar_t
 \arg signed char
 \arg short int
 \arg int
 \arg long int
 \arg long long int
 \arg unsigned char
 \arg unsigned short int
 \arg unsigned int
 \arg unsigned long int
 \arg unsigned long long int
 \arg float
 \arg double
 \arg long double
 \arg std::string
 \arg std::wstring
 \arg std::u16string
 \arg std::u32string

 */
class Member : virtual public MemberBase {
public:
  Member() : MemberBase(""), wert(T()) { TRACE(""); };  // Konstruktor für Array
  /// \private
  Member(std::string n, ObjectBase *o) : MemberBase(n), wert(T()) { TRACE(PARAM(n) << PARAM(this)); if (o) o->regMem(this); }; // Konstruktor f. Objekt nur intern
  Member &operator=(const Member &other) = delete;
  ~Member() { TRACE(PARAM(name())); };
  /// Zugriff auf Inhalt
  inline T operator() () const { return wert; };
  /// Zuweisung eines Wertes
  inline void operator() (const T &t) { TRACE(PARAM(this)); wert = t; };
  
  virtual void strOut(std::ostream &str) const { str << mobs::to_string(wert); };
  /// Setze Inhalt auf leer
  virtual void clear()  { wert = T(); };
  //  virtual std::string toStr() const { std::stringstream s; s << wert; return s.str(); };
  virtual std::string toStr() const { return mobs::to_string(wert); };
  //  virtual std::wstring toWStr2() const { return mobs::to_wstring(wert); };
  /// Abfrage, ob der Inhalt textbasiert ist (zb. in JSON in Hochkommata gestzt wird)
  virtual bool is_specialized() const { return std::numeric_limits<T>::is_specialized; };
  /// Abfrage, ob der Inhalt textbasiert ist (zb. in JSON in Hochkommata gestzt wird)
  virtual bool is_chartype() const { return mobschar(wert); };
  /// Einlesen der Variable aus einnem \c std::string im Format UTF-8
  virtual bool fromStr(const std::string &sin) { return mobs::string2x(sin, wert); };
  /// \private
  void doCopy(const Member<T> &other) { operator()(other()); };
private:
  T wert;
};

template<typename T>
/// Klasse zur Beschreibung von Membervariablen als Key-Element für einen Datenbank
KeyList & operator<<(KeyList &k, Member<T> &m)
{ 
  TRACE(PARAM(m.name()) << PARAM(m.key()));
  m.key(k.add());
  return k;
};

template<class T>
/// \brief Klasse für Vectoren auf Membervariablen oder Objekten innerhalb von von \c ObjectBase angeleiteten Klasse; Bitte als Makro MemVector oder ObjVector verwenden
/// \see MemVector
/// \see ObjVector
/// \tparam T entweder eine von \c MemberBase oder eine von \c ObjectBase abgeleitete Klasse
class MemberVector : virtual public MemBaseVector {
public:
  /// \private
  MemberVector(std::string n, ObjectBase *o) : MemBaseVector(n) { TRACE(PARAM(n) << PARAM(this)); o->regArray(this); };
  ~MemberVector() { TRACE(PARAM(m_name)); resize(0); };   // heap Aufräumen
  /// Zugriff auf das entsprechende Vector-Element
  T &operator[] (size_t t) { if (t >= size()) resize(t+1); return *werte[t]; };
  virtual size_t size() const { return werte.size(); };
  virtual void resize(size_t s);
  virtual void traverse(ObjTrav &trav);
  virtual void traverse(ObjTravConst &trav) const;
  virtual MemberBase *getMemInfo(size_t i) { if (i >= size()) return 0; return dynamic_cast<MemberBase *>(werte[i]); };
  virtual ObjectBase *getObjInfo(size_t i) { if (i >= size()) return 0; return dynamic_cast<ObjectBase *>(werte[i]); };
protected:
  /// \private
  void doCopy(const MemberVector<T> &other);
  /// \private
  void doCopy(const MemBaseVector &other);
private:
  // Vector von Heap-Elementen verwenden, da sonst Probleme beim Reorg
  std::vector<T *> werte;
};

/// Basisklasse zum rekursiven Durchlauf über eine Objektstruktur
class ObjTrav {
public:
  /// Callbackfunktion, die bei Betreten eines Objektes aufgerufen wird
  virtual void doObjBeg(ObjTrav &ot, ObjectBase &obj) = 0;
  /// Callbackfunktion, die bei Verlassen eines Objektes aufgerufen wird
  virtual void doObjEnd(ObjTrav &ot, ObjectBase &obj) = 0;
  /// Callbackfunktion, die bei Betreten eines Arrays aufgerufen wird
  virtual void doArrayBeg(ObjTrav &ot, MemBaseVector &vec) = 0;
  /// Callbackfunktion, die bei Verlassen eines Arrays aufgerufen wird
  virtual void doArrayEnd(ObjTrav &ot, MemBaseVector &vec) = 0;
  /// Callbackfunktion, die bei einer Varieblen aufgerufen wird
  virtual void doMem(ObjTrav &ot, MemberBase &mem) = 0;
};

/// Basisklasse zum rekursiven Durchlauf über eine  \c const  Objektstruktur
class ObjTravConst {
public:
  /// Callbackfunktion, die bei Betreten eines Objektes aufgerufen wird
  virtual void doObjBeg(ObjTravConst &ot, const ObjectBase &obj) = 0;
  /// Callbackfunktion, die bei Verlassen eines Objektes aufgerufen wird
  virtual void doObjEnd(ObjTravConst &ot, const ObjectBase &obj) = 0;
  /// Callbackfunktion, die bei Betreten eines Arrays aufgerufen wird
  virtual void doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec) = 0;
  /// Callbackfunktion, die bei Verlassen eines Arrays aufgerufen wird
  virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec) = 0;
  /// Callbackfunktion, die bei einer Varieblen aufgerufen wird
  virtual void doMem(ObjTravConst &ot, const MemberBase &mem) = 0;
};

/// Basisiklasse zum sequentiellen Einfügen von Daten in ein Objekt
class ObjectInserter  {
public:
  /// Zeiger auf die aktuelle MemberVariable oder nullptr, falls es keine Variable ist
  inline MemberBase *member() const { return memBase; };
  /// Name der aktuellen MemberVariablen, oder leer, falls es keine Variable ist
  inline const std::string &showName() const { return memName; };
  /// Name des aktuell referenzierten Objektes/Variable, leer, falls der das Objekt abgearbeitet ist
  inline const std::string current() const { return path.empty() ? "" : path.top(); };
  /// lege ein Objekt auf den Stapel, um die aktuellen Member zu füllen
  /// @param obj Objekt, muss von ObjecBase abgeleitet sein
  /// @param name Name des Objektes, muss nicht angegeben werden
  void pushObject(ObjectBase &obj, const std::string &name = "<obj>");
  /** \brief Suche ein Element in der aktuellen Objektstruktur
   @param element Name des Elementes
   @return liefert false, wenn das Element nicht existiert
  
   Ist das Element  eine Variable, so Kann über \c member darauf zugegriffen werden. Bei Objekten wird in die neue Objektebene gesprungen. Im Falle von Arrays wird ein neues Element angehängt.
      Sind entsprechnde Objekte nicht vorhanden, wird trotzdem die Struktur verfolgt und bei Rückkehr in die entsprechende Eben die Bearbeitung wieder aufgenommen.
   */
  bool enter(const std::string &element);
  /// Verlassen einer Ebene in der Objektstruktur
  /// @param element Name des Elementes oder leer, wenn der Name der Struktur nicht geprüft werden soll
  /// \throw std::runtime_error bei Strukturfehler
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

/// Ausgabe eines Objektes im kompakten JSON-Format, Keys unquoted
std::string to_string(const ObjectBase &obj);
/// Ausgabe eines Objektes im  JSON-Format ohne whitespace
std::string to_json(const ObjectBase &obj);
//std::wstring to_wstring(const ObjectBase &obj);
void string2Obj(const std::string &str, ObjectBase &obj);
}

#endif

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
   MemVector(Kontakt,     kontakte);  // vector<Kontakt> kontakte;
   MemVarVector(std::string, hobbies);   // vector<std::string> hobbies
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
 \see ObjectNavigator
 
  Es existieren Klassen, um solche Objekt von und nach XML oder JSON zu wandeln
 
 \todo Anpassungen um die Zugriffe threadsafe zu gestalten
 
 */



#ifndef MOBS_OBJGEN_H
#define MOBS_OBJGEN_H

#include <list>
#include <map>
#include <vector>
#include <stack>

#include "logging.h"
#include "objtypes.h"

namespace mobs {

/// Makro für Typ-Deklaration zu \c MemVar
#define MemVarType(typ) mobs::Member<typ, mobs::StrConv<typ>>
/*! \brief Deklarations-Makro für eine  Membervarieable
@param typ Basistyp
@param name Name
*/
#define MemVar(typ, name, ...) MemVarType(typ) name = MemVarType(typ) (#name, this __VA_ARGS__)
/// Makro für Typ-Deklaration zu \c MemEnumVar
#define MemEnumVarTyp(typ) mobs::Member<typ, mobs::StrIntConv<typ>>
/*! \brief Deklarations-Makro für eine  Membervarieable des Typs \c enum
@param typ Basistyp
@param name Name
*/
#define MemEnumVar(typ, name, ...) MemEnumVarTyp(typ) name = MemEnumVarTyp(typ) (#name, this)
/// Makro für Typ-Deklaration zu \c MemMobsEnumVar
#define MemMobsEnumVarType(typ) mobs::Member<enum typ, mobs::Str##typ##Conv>
/*! \brief Deklarations-Makro für eine  Membervarieable eines mit \c MOBS_ENUM_DEF erzeugten enums
@param typ Name des enums (ohne Tiken \c enum
@param name Name
*/
#define MemMobsEnumVar(typ, name, ...) MemMobsEnumVarType(typ) name = MemMobsEnumVarType(typ) (#name, this __VA_ARGS__)

/// Makro für Typ-Deklaration zu \c MemMobsVar
#define MemMobsVarType(typ, converter) mobs::Member<typ, converter>
/*! \brief Deklarations-Makro für eine  Membervarieable eines mit \c MOBS_ENUM_DEF erzeugten enums
@param typ Name des enums (ohne Tiken \c enum
@param name Name
@param converter Konvertier-Klasse von und nach \c std::string
\see  ConvToStrHint
*/
#define MemMobsVar(typ, name, converter, ...) MemMobsVarType(typ, converter) name = MemMobsVarType(typ, converter) (#name, this __VA_ARGS__)

/// \private
enum MemVarCfg { Unset, InitialNull, VectorNull, Key1, Key2, Key3, Key4, Key5, AltNameBase = 100, AltNameBaseEnd = 299 };
/// \private
enum mobs::MemVarCfg mobsToken(MemVarCfg base, std::vector<std::string> &confToken, const std::string &s);

#define USENULL ,mobs::InitialNull ///< Element wir mit\c null vorinitialisiert
#define USEVECNULL ,mobs::VectorNull ///< Bei Vectoren wird der Vector selbst mit \c null vorinitialisiert
#define KEYELEMENT1 ,mobs::Key1 ///<  Keyelement der Priorität 1 (erstes Element)
#define KEYELEMENT2 ,mobs::Key2 ///< Keyelement der Priorität 2
#define KEYELEMENT3 ,mobs::Key3 ///< Keyelement der Priorität 3
#define KEYELEMENT4 ,mobs::Key4 ///< Keyelement der Priorität 4
#define KEYELEMENT5 ,mobs::Key5 ///< Keyelement der Priorität 5
#define ALTNAME(name) ,::mobs::mobsToken(::mobs::AltNameBase,m_confToken,#name) ///< Definition eines Alternativen Namens für die Ein-/Ausgsbe




// vector
/*! \brief Deklarations-Makro für eines Vector von Objektvaliebalen
 @param typ Objekttyp
 @param name Name
 */
#define MemVector(typ, name, ...) mobs::MemberVector<typ> name = mobs::MemberVector<typ>(#name, this __VA_ARGS__)
/*! \brief Deklarations-Makro für eines Vector von Membervarieablen
  identisch mit \c MemVector(MeVarType(typ), \c name)
 @param typ Basistyp
 @param name Name
 */
#define MemVarVector(typ, name, ...) MemVector( MemVarType(typ), name, __VA_ARGS__)
//#define MemVarVector(typ, name) mobs::MemberVector< MemVarType(typ) > name = mobs::MemberVector< MemVarType(typ) >(#name, this)


/*! \brief Deklarations-Makro für eine Objekt als Membervariable
 @param typ Objekttyp
 @param name Name
 */
#define ObjVar(typ, name, ...) typ name = typ(#name, this __VA_ARGS__)

/*! \brief Makro für Definitionen im Objekt das von ObjecBase ist
 @param objname Name der Klasse (muss von ObjecBase abgeleitet sein)
 */
#define ObjInit(objname) \
objname() { TRACE(""); doInit(); init(); }; \
objname(const objname &that) { TRACE(""); doCopy(that); }; \
objname(mobs::MemBaseVector *m, mobs::ObjectBase *o, mobs::MemVarCfg c1 = mobs::Unset, mobs::MemVarCfg c2 = mobs::Unset, mobs::MemVarCfg c3 = mobs::Unset) : ObjectBase(m, o, c1, c2, c3) { TRACE(""); doInit(); init(); }; \
objname(std::string name, ObjectBase *t, mobs::MemVarCfg c1 = mobs::Unset, mobs::MemVarCfg c2 = mobs::Unset, mobs::MemVarCfg c3 = mobs::Unset) : ObjectBase(name, t, c1, c2, c3) { TRACE(PARAM(name) << PARAM(this)); if (t) t->regObj(this); doInit(); init(); }; \
objname &operator=(const objname &rhs) { TRACE(""); if (this != &rhs) { doCopy(rhs); } return *this; }; \
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


class ObjTrav;
class ObjTravConst;

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

class ObjectBase;
class MemBaseVector;

// ------------------ MemberBase ------------------

/// Basisklasse für Membervariablen
class MemberBase : public NullValue {
protected:
  /// \private
  MemberBase(std::string n, ObjectBase *obj, MemVarCfg c1 = Unset, MemVarCfg c2 = Unset, MemVarCfg c3 = Unset) : m_name(n), m_parent(obj) { doConfig(c1); doConfig(c2); doConfig(c3); }
  /// \private
  MemberBase(mobs::MemBaseVector *m, mobs::ObjectBase *o, MemVarCfg c1 = Unset, MemVarCfg c2 = Unset, MemVarCfg c3 = Unset) : m_name(""), m_parent(o), m_parVec(m) { doConfig(c1); doConfig(c2); doConfig(c3); }
public:
  virtual ~MemberBase() {}
  /// Abfrage des Namen der Membervariablen
  std::string name() const { return m_name; }
  /// Config-Token alternativer Name oder \c SIZE_T_MAX
  size_t cAltName() const { return m_altName; };
//  virtual void strOut(std::ostream &str) const  = 0;
  /// Setze Inhalt auf leer
  virtual void clear() = 0;
  /// Ausgabe des Ihnhalts als \c std::string in UTF-8
  virtual std::string toStr(const ConvToStrHint &) const  = 0;
  /// Abfrage ob der Basistyp vom typ \c  std::is_specialized  ist; \see \<limits>
  virtual bool is_specialized() const  = 0;
  /// Abfrage, ob der Inhalt textbasiert ist (zb. in JSON in Hochkommata gestzt wird)
  virtual bool is_chartype(const ConvToStrHint &) const = 0;
  /// Einlesen der Variable aus einnem \c std::string im Format UTF-8
  virtual bool fromStr(const std::string &s, const ConvFromStrHint &) = 0;
  /// Starte Traversierung nicht const
  void traverse(ObjTrav &trav);
  /// Starte Traversierung  const
  void traverse(ObjTravConst &trav) const;
  /// private
  void key(int k) { m_key = k; }
  /// \brief Abfrage ob Memvervariable ein Key-Element ist
  /// @return Position im Schlüssel oder \c 0 wenn kein Schlüsselelement
  int key() const { return m_key; }
  /// Zeiger auf Vater-Objekt
  ObjectBase *parent() const { return m_parent; }
  /// Objekt wurde beschrieben
  void activate();

protected:
  /// \private
  int m_key = 0;
  /// \private
  size_t m_altName = SIZE_T_MAX;
  
private:
  void doConfig(MemVarCfg c);
  std::string m_name;
  ObjectBase *m_parent = nullptr;
  MemBaseVector *m_parVec = nullptr;
};

// ------------------ VectorBase ------------------


/// \brief Basisklasse für Vectoren auf Membervariablen oder Objekten innerhalb von von \c ObjectBase angeleiteten Klasse; Bitte als Makro MemVarVector oder MemVector verwenden
/// \see MemVarVector
/// \see MemVector
class MemBaseVector : public NullValue {
public:
  /// \private
  MemBaseVector(std::string n, ObjectBase *obj, MemVarCfg c1 = Unset, MemVarCfg c2 = Unset, MemVarCfg c3 = Unset, MemVarCfg c4 = Unset) : m_name(n), m_parent(obj)
  { TRACE(PARAM(n)); doConfig(c1); doConfig(c2); doConfig(c3); doConfig(c4); if (nullAllowed()) setNull(true); m_c.resize(3, Unset); }
  virtual ~MemBaseVector() { TRACE(""); }
  /// Starte Traversierung nicht const
  virtual void traverse(ObjTrav &trav) = 0;
  /// Starte Traversierung  const
  virtual void traverse(ObjTravConst &trav) const = 0;
  /// liefert die Anzahl der Elemente
  virtual size_t size() const = 0;
  /// Vergrößert/verkleinert die Elemtzahl des Vektors
  virtual void resize(size_t s) = 0;
  /// liefert den Namen der Vektor variablen
  inline std::string name() const { return m_name; }
  /// Config-Token alternativer Name oder \c SIZE_T_MAX
  size_t cAltName() const { return m_altName; };
  /// \private
  virtual void doCopy(const MemBaseVector &other) = 0;
  /// liefert einen Zeiger auf das entsprechende Element, falls es eine \c MemVar ist
  virtual MemberBase *getMemInfo(size_t i) = 0;
  /// liefert einen Zeiger auf das entsprechende Element, falls es eine \c MemObj ist
  virtual ObjectBase *getObjInfo(size_t i) = 0;
  /// Setze Inhalt auf leer; äquivalent zu \c resize(0)
  void clear() { resize(0); }
  /// Objekt wurde beschrieben
  void activate();
  /// Zeiger auf Vater-Objekt
  inline ObjectBase *parent() const { return m_parent; }

protected:
  /// \private
  std::string m_name;
  /// \private
  std::vector<MemVarCfg> m_c;
  /// \private
  size_t m_altName = SIZE_T_MAX;


private:
  void doConfig(MemVarCfg c);
  ObjectBase *m_parent = nullptr;
};

#if 0
/// Hilfsklasse für Json-Ausgabe
class ConvToJsonHint : virtual public ConvToStrHint {
public:
  virtual ~ConvToJsonHint() {}
  /// \private
  virtual bool compact() const { return comp; }
  
  bool comp;
};
#endif

// ------------------ ObjectBase ------------------


/// Basisklasse für Objekte
class ObjectBase : public NullValue {
public:
  ObjectBase() { }
  /// \private
  ObjectBase(std::string n, ObjectBase *obj, MemVarCfg c1 = Unset, MemVarCfg c2 = Unset, MemVarCfg c3 = Unset) : m_varNam(n), m_parent(obj) { doConfig(c1); doConfig(c2); doConfig(c3); } // Konstructor for ObjVar
  /// \private
  ObjectBase(MemBaseVector *m, ObjectBase *o, MemVarCfg c1 = Unset, MemVarCfg c2 = Unset, MemVarCfg c3 = Unset) : m_varNam(""), m_parent(o), m_parVec(m) { doConfig(c1); doConfig(c2); doConfig(c3); } // Konstructor for Vector
  virtual ~ObjectBase() {};
  /// \private
  ObjectBase &operator=(const ObjectBase &rhs) { doCopy(rhs); return *this; }
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
  virtual std::string typName() const { return ""; }
  /// Callback-Funktion die einmalig im Constructor aufgerufen wird
  virtual void init() {};
  /// liefert den Namen Membervariablen
  std::string name() const { return m_varNam; };
  /// Config-Token alternativer Name oder \c SIZE_T_MAX
  size_t cAltName() const { return m_altName; };
  /// Objekt wurde beschrieben
  void activate();
  /// private
  void key(int k) { m_key = k; }
  /// \brief Abfrage ob Unteröbjekt ein Key-Element enthält
  /// @return Position im Schlüssel oder \c 0 wenn kein Schlüsselelement
  int key() const { return m_key; }
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
  /// Setzt eine Variable mit dem angegebene Inhalt
  /// @param path Pfad der Variable z.B.: kontakt[3].number
  /// @param value Inhalt, der geschrieben werden soll
  /// \return true, wenn der Vorgang erfolgreich war
  bool setVariable(const std::string &path, const std::string &value);
  /// liest eine Variable relativ zum angegebenen Pfad
  /// @param path Pfad der Variable z.B.: kontakt[3].number
  /// @param found opt. Zeiger auf bool-Variable, die Anzeigt, ob das Element gefunden wurde
  /// @param compact opt. Angabe, ob ausgabe \e kompakt erfolgen soll (bei enum als \c int statt als Text
  /// \return Inhalt der variable als string in UTF-8, oder leer, wenn nicht gefunden
  std::string getVariable(const std::string &path, bool *found = nullptr, bool compact = false) const;
  
  /// Erzeuge eine Liste der Key-Elementze
  /// @param key Rückgabe der Liste
  /// @param cth Konvertierungs-Hinweis
  void getKey(std::list<std::string> &key, const ConvToStrHint &cth) const;
  /// liefert einen \c std::string aus den Key-Elementen
  std::string keyStr() const;
  /// \brief Kopiere ein Objekt aus einem bereits vorhandenen.
  /// @param other zu Kopierendes Objekt
  /// \throw runtime_error Sind die Strukturen nicht identisch, wird eine Exception erzeugt
  void doCopy(const ObjectBase &other);
  /// Zeiger auf Vater-Objekt
  ObjectBase *parent() const { return m_parent; }
  /// Config-Token lesen
  const std::string &getConf(std::size_t i) const { static std::string x; if (i < m_confToken.size()) return m_confToken[i]; return x; }
  /// Ausgabe als \c std::string (Json)
  std::string to_string(ConvObjToString cft = ConvObjToString()) const;


protected:
  /// \private
  void doInit() { if (nullAllowed()) setNull(true); } // Initialisierung am Ende des Konstruktors
  /// \private
  std::vector<std::string> m_confToken; // Liste der Konfigurationstokens

private:
  std::string m_varNam;
  ObjectBase *m_parent = nullptr;
  MemBaseVector *m_parVec = nullptr;
  int m_key = 0;
  size_t m_altName = SIZE_T_MAX;

  void doConfig(MemVarCfg c);

  class MlistInfo {
  public:
    MlistInfo(MemberBase *m, ObjectBase *o, MemBaseVector *v) : mem(m), obj(o), vec(v) {}
    MemberBase *mem = 0;
    ObjectBase *obj = 0;
    MemBaseVector *vec = 0;
  };
  std::list<MlistInfo> mlist;
  static std::map<std::string, ObjectBase *(*)()> createMap;
  
};


//template <typename T>
//inline T mobsempty(T&) { return T(); };
//template <> inline char mobsempty(char&) { return ' '; };
//template <> inline signed char mobsempty(signed char&) { return ' '; };
//template <> inline unsigned char mobsempty(unsigned char&) { return ' '; };

// ------------------ Member ------------------

template<typename T, class C> 
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
class Member : virtual public MemberBase, public C {
public:
  /// \private
  Member(MemVarCfg c1 = Unset, MemVarCfg c2 = Unset, MemVarCfg c3 = Unset) : MemberBase("", nullptr) { TRACE(""); clear(); }  // Konstruktor für Array oder Solo
  /// \private
  Member(std::string n, ObjectBase *o, MemVarCfg c1 = Unset, MemVarCfg c2 = Unset, MemVarCfg c3 = Unset) : MemberBase(n, o, c1, c2, c3), wert(T()) { TRACE(PARAM(n) << PARAM(this)); if (o) o->regMem(this); clear(); } // Konstruktor innerhalb  Objekt nur intern
  /// \private
  Member(MemBaseVector *m, ObjectBase *o, MemVarCfg c1 = Unset, MemVarCfg c2 = Unset, MemVarCfg c3 = Unset) : MemberBase(m, o, c1, c2, c3) {} // Konstruktor f. Objekt nur intern
  Member &operator=(const Member &other) = delete;
  ~Member() { TRACE(PARAM(name())); }
  /// Zugriff auf Inhalt
  inline T operator() () const { return wert; }
  /// Zuweisung eines Wertes
  inline void operator() (const T &t) { TRACE(PARAM(this)); wert = t; activate(); }
  
//  virtual void strOut(std::ostream &str) const { str << mobs::to_string(wert); }
  /// Setze Inhalt auf leer
  virtual void clear()  { wert = this->c_empty(); if (nullAllowed()) setNull(true); else activate(); };
  //  virtual std::string toStr() const { std::stringstream s; s << wert; return s.str(); }
  virtual std::string toStr(const ConvToStrHint &cth) const { return this->c_to_string(wert, cth); };
  //  virtual std::wstring toWStr2() const { return mobs::to_wstring(wert); }
  /// Abfrage, ob der Inhalt textbasiert ist (zb. in JSON in Hochkommata gestzt wird)
  virtual bool is_specialized() const { return this->c_is_specialized(); }
  /// Abfrage, ob der Inhalt textbasiert ist (zb. in JSON in Hochkommata gestzt wird)
  virtual bool is_chartype(const ConvToStrHint &cth) const { return this->c_is_chartype(cth); }
  /// Einlesen der Variable aus einnem \c std::string im Format UTF-8
  virtual bool fromStr(const std::string &sin, const ConvFromStrHint &cfh) { if (this->c_string2x(sin, wert, cfh)) { activate(); return true; } return false; }
  /// \private
  void doCopy(const Member<T, C> &other) { operator()(other()); }
private:
  T wert;
};
 

// ------------------ Iterator ------------------

template<typename T>
/// \private
class MemberVectorIterator
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;
  
  MemberVectorIterator() { }
  MemberVectorIterator(typename std::vector<T *>::iterator it) { m_iter = it; }
  MemberVectorIterator(const MemberVectorIterator<T>& rawIterator) = default;
  ~MemberVectorIterator() { }
  
  MemberVectorIterator<T>& operator=(const MemberVectorIterator<T>& rawIterator) = default;
  //    MemberVectorIterator<T>&                  operator=(T* ptr){m_ptr = ptr;return (*this);}
  
  operator bool() const { return m_iter; }
  bool operator==(const MemberVectorIterator<T>& rawIterator)const{ return (m_iter == rawIterator.m_iter); }
  bool operator!=(const MemberVectorIterator<T>& rawIterator)const{ return (m_iter != rawIterator.m_iter); }
  MemberVectorIterator<T>& operator+=(const difference_type& movement) { m_iter += movement;return *this; }
  MemberVectorIterator<T>& operator-=(const difference_type& movement) { m_iter -= movement;return *this; }
  MemberVectorIterator<T>& operator++() { ++m_iter; return *this; }
  MemberVectorIterator<T>& operator--() { --m_iter; return *this; }
  MemberVectorIterator<T> operator++(int) { auto temp(*this); ++m_iter; return temp; }
  MemberVectorIterator<T> operator--(int) { auto temp(*this); --m_iter; return temp; }
  MemberVectorIterator<T> operator+(const difference_type& movement) { auto oldIdx = m_iter; m_iter+=movement; auto temp(*this); m_iter = oldIdx; return temp; }
  MemberVectorIterator<T> operator-(const difference_type& movement) { auto oldIdx = m_iter; m_iter-=movement; auto temp(*this); m_iter = oldIdx; return temp; }
  difference_type operator-(const MemberVectorIterator<T>& rawIterator) { return std::distance(rawIterator.m_ite,this->m_iter); }
  
  T &operator*(){return **m_iter;}
  const T &operator*()const{return **m_iter;}
  T *operator->(){return *m_iter;}
protected:
  typename std::vector<T *>::iterator m_iter;
};

// ------------------ MemberVector ------------------

template<class T>
/// \brief Klasse für Vectoren auf Membervariablen oder Objekten innerhalb von von \c ObjectBase angeleiteten Klasse; Bitte als Makro MemVarVector oder MemVector verwenden
/// \see MemVarVector
/// \see MemVector
/// \tparam T entweder eine von \c MemberBase oder eine von \c ObjectBase abgeleitete Klasse
class MemberVector : virtual public MemBaseVector {
public:
  /// \private
  MemberVector(std::string n, ObjectBase *o, MemVarCfg c1 = Unset, MemVarCfg c2 = Unset, MemVarCfg c3 = Unset, MemVarCfg c4 = Unset) : MemBaseVector(n, o, c1, c2, c3, c4) { TRACE(PARAM(n)); o->regArray(this); }
  ~MemberVector() { TRACE(PARAM(m_name)); resize(0); };   // heap Aufräumen
  /// Zugriff auf das entsprechende Vector-Element
  T &operator[] (size_t t) { if (t >= size()) resize(t+1); return *werte[t]; }
  virtual size_t size() const { return werte.size(); }
  virtual void resize(size_t s);
  virtual void traverse(ObjTrav &trav);
  virtual void traverse(ObjTravConst &trav) const;
  virtual MemberBase *getMemInfo(size_t i) { if (i >= size()) return 0; return dynamic_cast<MemberBase *>(werte[i]); }
  virtual ObjectBase *getObjInfo(size_t i) { if (i >= size()) return 0; return dynamic_cast<ObjectBase *>(werte[i]); }
 
  /// Typ-Deklaration iterator auf Vector
  typedef MemberVectorIterator<T> iterator;
  /// Typ-Deklaration iterator auf Vector
  typedef MemberVectorIterator<const T> const_iterator;
  /// Typ-Deklaration iterator auf Vector
  typedef MemberVectorIterator<T> reverse_iterator;
  /// Typ-Deklaration iterator auf Vector
  typedef MemberVectorIterator<const T> const_reverse_iterator;

  /// Start-Iterator
  iterator begin() noexcept { return werte.begin(); }
  /// Ende-Iterator
  iterator end() noexcept { return werte.end(); }
  /// Start-Iterator
  reverse_iterator rbegin() { return werte.rbegin(); }
  /// Ende-Iterator
  reverse_iterator rend() { return werte.rend(); }
  /// Start-Iterator
  const_iterator cbegin() noexcept { return werte.cbegin(); }
  /// Ende-Iterator
  const_iterator cend() noexcept { return werte.cend(); }
  /// Start-Iterator
  const_reverse_iterator crbegin() { return werte.crbegin(); }
  /// Ende-Iterator
  const_reverse_iterator crend() { return werte.crend(); }

protected:
  /// \private
  void doCopy(const MemberVector<T> &other);
  /// \private
  void doCopy(const MemBaseVector &other);
private:
  // Vector von Heap-Elementen verwenden, da sonst Probleme beim Reorg
  std::vector<T *> werte;
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
      werte[i] = new T(this, parent(), m_c[0], m_c[1], m_c[2]);
  }
}


// ------------------ Navigation / Ausgabe ------------------


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
class ObjectNavigator  {
public:
  /// Zeiger auf die aktuelle MemberVariable oder nullptr, falls es keine Variable ist
  inline MemberBase *member() const { return memBase; };
  /// Name der aktuellen MemberVariablen, oder leer, falls es keine Variable ist
  inline const std::string &showName() const { return memName; };
  /// Name des aktuell referenzierten Objektes/Variable, leer, falls der das Objekt abgearbeitet ist
  inline const std::string current() const { return path.empty() ? "" : path.top(); }
  /// lege ein Objekt auf den Stapel, um die aktuellen Member zu füllen
  /// @param obj Objekt, muss von ObjecBase abgeleitet sein
  /// @param name Name des Objektes, muss nicht angegeben werden
  void pushObject(ObjectBase &obj, const std::string &name = "<obj>");
  /** \brief Suche ein Element in der aktuellen Objektstruktur
   @param element Name des Elementes
   @param index optonal: Index bei Vectoren; ansosnten wir der Vector um ein Element erweitert
   @return liefert false, wenn das Element nicht existiert
  
   Ist das Element  eine Variable, so Kann über \c member darauf zugegriffen werden. Bei Objekten wird in die neue Objektebene gesprungen. Im Falle von Arrays wird ein neues Element angehängt.
      Sind entsprechnde Objekte nicht vorhanden, wird trotzdem die Struktur verfolgt und bei Rückkehr in die entsprechende Eben die Bearbeitung wieder aufgenommen.
   */
  bool enter(const std::string &element, std::size_t index = SIZE_MAX);
  /// Verlassen einer Ebene in der Objektstruktur
  /// @param element Name des Elementes oder leer, wenn der Name der Struktur nicht geprüft werden soll
  /// \throw std::runtime_error bei Strukturfehler
  void leave(const std::string &element = "");
  /// Sucht einen direkten Pfad zu einer Elemntvariablen nach C-Syntax
  ///
  /// Sucht nach Variablen irelativ zu dem mit \ pushObj abgelegten Objektes:
  /// z.B.:  "kontakte[2].number"
  ///  find darf nur einmalig aufgerufen werden
  /// @param path Variablenname
  /// \return liefert \c false bei Syntax-Fehler
  bool find(const std::string &path);
  
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
inline std::string to_string(const ObjectBase &obj) { return obj.to_string(); };
//std::wstring to_wstring(const ObjectBase &obj);

void string2Obj(const std::string &str, ObjectBase &obj, const ConvFromStrHint &cfh = ConvFromStrHint::convFromStrHintDflt);
}

#endif

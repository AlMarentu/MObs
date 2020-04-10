#  MObs
Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte

[MObs GitHub](https://github.com/AlMarentu/MObs)
Lizenz: LGPL

## MObs Objektdefinition

 Alle MObs-Objekte sind Klassen die von public mobs::ObjectBase abgeleitet sind.

Die Konstruktoren und nötigen Hilfsfunktionen weden über das Makro ObjInit(<class name>) ; erzeugt.

Die Zugriffshethoden für einfache Variablen erzeugt das Makro MemVar<type>, <variable name>);

~~~~~~~~~~cpp
#include "objgen.h"

class Fahrzeug : virtual public mobs::ObjectBase
 {
   public:
     ObjInit(Fahrzeug); // Makro zum Initialisieren, Name muss mit Klassennamen übereinstimmen

     MemVar(std::string, fahrzeugTyp);
     MemVar(int,         anzahlRaeder);
}
~~~~~~~~~~

Der Zugriff auf die Variablen erfolgd dann über
~~~~~~~~~~cpp
Fahrzeug f;
f.fahrzeugTyp("PKW");
std::cout << f.fahrzeugTyp();
~~~~~~~~~~

Als Basistypen für Variablen sid folgende erlaubt:
* bool
* char
* char16_t
* char32_t
* wchar_t
* signed char
* short int
* int
* long int
* long long int
* unsigned char
* unsigned short int
* unsigned int
* unsigned long int
* unsigned long long int
* float
* double
* long double
* std::string
* std::wstring
* std::u16string
* std::u32string

Zusätzlich können auch wieder MObs-Objekte als Member verwendet werden sowie Vektoren von diesen Elementen.

Dazu stehen die Makros
* MemVar für Basisitypen
* ObjVar für Objekte
* MemVarVector für Vector von Basisitypen
* MemVector für Vectoren von Objekt-Klassen
zur Verfügung
~~~~~~~~~~cpp
class Kontakt : virtual public mobs::ObjectBase {
public:
  ObjInit(Kontakt);
  enum device { fax, sms, mobil, privat, arbeit };
  
  /// Art des Kontaktes Fax/Mobil/SMS
  MemVar(int, art);
  /// Nummer
  MemVar(string, number);
};

class Adresse : virtual public mobs::ObjectBase {
public:
  ObjInit(Adresse);
  
  MemVar(std::string, strasse);
  MemVar(std::string, plz);
  MemVar(std::string, ort);
};

class Person : virtual public mobs::ObjectBase {
  public:
  ObjInit(Person);
  
  MemVar(int, kundennr);
  MemVar(bool, firma);
  MemVar(std::string, name);
  MemVar(std::string, vorname);
  ObjVar(Adresse, adresse);
  MemVector(Kontakt, kontakte);
  MemVarVector(std::string, hobbies);
};
~~~~~~~~~~
Der Zugriff auf die Elemente dieser Klassen erfolgt über get- und set-Methoden:
~~~~~~~~~~cpp
Person p;
p.name("Peter");
p.adresse.ort("Tokyo");
p.hobbies[2]("Tauchen");
p.kontakt[3].number("+49 0000 00000");
std::cout << p.adresse.ort() << " " << p.hobbies[2]();
~~~~~~~~~~
DIe Vectoren werden bei Zugriffen automatisch vergrößert, so dass eine Überprüfung entfallen kann.

## Features
### Serialisierung
Funktionen für Im- und Export in JSON und XML sind vorhanden

### Traversierung
Über einen Operator kann ein Objekt rekursiv durchlaufen werden
Über Callback-Functions könen Aktionen bei
* Betreten eines Objektes
* Verlassen eines Objektes
* Betreten eines Vectors
* Verlassen eines Vectos
* Zugriff auf einen Basistyp
ausgelöste werden.

Über diese Methotik bestehen Funktionen um ein Objekt
* im JSON-Format auszugeben (to_string bzw. to_json)
* Im XML-Format auszugeben (to_xml)

### Dynamische Navigation innerhalb eines Objektes
Über den ObjectNavaigator könne Objekte-Teile gezielt angesprochen werden.

Hier kann auf Elemente auch über textuele Anweisungen zgegriffen werden:
~~~~~~~~~~cpp
obj.setVariable("p.hobbies[2]", "Tauchen");
~~~~~~~~~~


### Null-Values
Alle Elemente haben zusätliche Eigenschaften für NULL.

## Objektverwaltung
Über ein extra Modul könen beliebige Objekte in einen Pool abgelegt werden. Der Zugriff erfogt nur
über den Objekt-Namen.

Darüber lassen sich
* einfache Caches
* Objekt-Referenzen,
* On-Demand Datenbakkzugriffe
* in-Memory Datenbanken
usw. realiesieren

## Installation
Zum Übersetzen wird ein c++11 Compiler inkl. STL benötigt.
Einige optionale Module benötigen rapidjson

Das Makefile ist rudimentär und eine Installationsroutine fehlen noch. 

Dies folgt in einer späteren Version

## Module
* objgen.h  Deklaration um die Zielklassen zu generieren
* jsonparser.h  EInfacher JSON-Parser
* xmlparser.h  EInfacher XML-Parser
* xmlout.h  Optionale Klasse um Objekte in einen XML-String umzuwandeln
* xmlread.h  Optionale Klasse um Objekte aus einen XML-String auszulesen
* objpool.h  Klassen für Named Objects - minimale in memory Datenbank oder Objekt-Cache 





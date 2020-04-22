#  MObs
Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte

[MObs GitHub](https://github.com/AlMarentu/MObs)
Lizenz: LGPL

## Einführung

Werden Programme komplexer, müssen Daten zwischen verschiedenen Schichten ausgetauscht, über Netzwerke übertragen oder in Datenbanken abgelegt werden.
Für solche Serialisierungen von Daten-Stömen/-Objekten hat sich mittlerweile JSON oder XML als Standard etabliert.

In C++-Programmen werden Strukturen jedoch idealerweise in Klassen abgelegt. Dadurch ergeben sich Typ-Sicherheit und Hirarchische Definitionen.

Mobs vereint hier eine Vereinfachung der Deklaration über Basisklassen und Definitionsmakros. So lässt sich sehr einfach eine Struktur abbilden, die dann entsprechend
weiterverarbeitet werden kann. Die Mobs-Objekte lifern automatische Getter und Setter, Methoden zur Traversierung und Navigation sowie textbasierte Zugriffe auf die Elemente.
Die Umwandlung der Objekte von und nach Text (JSON bzw. XML) sind ebenso vorhanden, wie die Definition von Schlüsselelementen alternativer Member-Namen sowie
eine erweiterte behandlung von NULL-Werten.

Als weieres Feature können enum-Werte im Klartext serialisiert weden um designunabhängig zu bleiben. Unterstützt werden alle gängigen C++ und STL Basistypen.

Erweitert werden dies Funktionen durch eine Klasse mit Benamten-Zeigern. Mobs-Objekte können in einen Daten-Pool abgelegt, und über einen Schlüsselwert wieder abgerufen werden.
So kann eine simple In-Memory Datenbank aufgebaut werden oder Objekt-Caches angelegt werden.

Als zukünftige Erweiterung ist ein Datenbank-Interface geplant, um Mobs-Objekte in File-, und (NO-)SQL-Datenbanken zu speichern.


## MObs Objektdefinition

Die Konstruktoren und nötigen Hilfsfunktionen weden über das Makro ObjInit(<class name>) ; erzeugt.

Die Zugriffsmethoden für einfache Variablen erzeugt das Makro MemVar<type>, <variable name>);

~~~~~~~~~~cpp
#include "objgen.h"

class Fahrzeug : virtual public mobs::ObjectBase
 {
   public:
     ObjInit(Fahrzeug);                     // Makro zum Initialisieren, Name muss mit Klassennamen übereinstimmen

     MemVar(std::string, fahrzeugTyp);      // Makro für einen Member-Variable
     MemVar(int,         anzahlRaeder);
}
~~~~~~~~~~

Der Zugriff auf die Variablen erfolgd dann über Getter und Setter-Methoden
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

Die Klasse UxTime steht als Wrapper für den typ time_t zur Verfügung, um Zeitpunkte in den Objekten verwenden zu können

Zusätzlich können auch wieder MObs-Objekte als Member verwendet werden sowie Vektoren von diesen Elementen.

Dazu stehen die Makros
* MemVar für Basisitypen
* MemObj für Objekte
* MemVarVector für Vektor von Basisitypen
* MemVector für Vektoren von Objekt-Klassen
* MemMobsEnumVar, MOBS_ENUM_DEF und MOBS_ENUM_VAL für enum-definitionen
zur Verfügung
~~~~~~~~~~cpp
MOBS_ENUM_DEF(device,  fax,   sms,   mobil,   privat,   arbeit )
MOBS_ENUM_VAL(device, "fax", "sms", "mobil", "privat", "arbeit" )

class Kontakt : virtual public mobs::ObjectBase {
public:
  ObjInit(Kontakt);
  
  /// Art des Kontaktes 
  MemMobsEnumVar(device, art);
  /// Nummer
  MemVar(std::string, number);
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
  MemObj(Adresse, adresse);
  MemVector(Kontakt, kontakte);
  MemVarVector(std::string, hobbies);
};

...
Person p;
p.name("Peter");
p.adresse.ort("Tokyo");
p.hobbies[2]("Tauchen");
p.kontakt[3].number("+49 0000 00000");
std::cout << p.adresse.ort() << " " << p.hobbies[2]();
~~~~~~~~~~
DIe Vektoren werden bei Zugriffen automatisch vergrößert, so dass eine Überprüfung entfallen kann.

Werden alternative Namen Schlüsselelemente oder Behandlung von Null-Werten benötigt, existieren die Erweiterungen
* KEYELEMENT 
* USENULL
* ALTNAME
Diese werdern  (ohne Trenner bei Mehrfachverwendung) als 3. Parameter im Makro angegeben 
~~~~~~~~~~cpp
class Obj1 : virtual public mobs::ObjectBase {
  public:
  ObjInit(Obj1);
  MemVar(int, id, KEYELEMENT1);
  MemVar(int, xx);
  MemVar(int, yy, USENULL KEYELEMENT3 ALTNAME(NewName));
  MemVar(int, zz);

  MemObj(Obj0, oo, USENULL KEYELEMENT2);
};
~~~~~~~~~~

## Features
### Basisklasse
Alle MObs-Objekte sind von  mobs::ObjectBase abgeleitet. Alle Funktionen und Zugriffe auf die Elemente sind auch über die Basisklasse möglich.

~~~~~~~~~~cpp
mobs::ObjectBase *b = ...
b->setVariable("kontakte[3].number", "00-00-00");
cout << b->kontakte[1].number();

cout << b->to_string();  // Ausgabe als vereinfachtes JSON
~~~~~~~~~~

### Serialisierung
Die Umwandlung von Mobs in einen std::string erfolgt über die .to_string() Metode.

Zur Steuerung des Ausgabeformates können entsprechende Parameter angegeben werden, z.B.:
~~~~~~~~~~cpp
b->to_string(mobs::ConvObjToString().exportXml().exportAltNames().doIndent())
~~~~~~~~~~
Zur Verfügung stehen:
* exportXml()  Ausgabe als XML
* exportJson() Ausgabe als JSON
* exportAltNames() Verwende die alternativen Namen bei der Ausgabe
* doIndent() Ausgabe mit Pretty-Printer
* noIndent() Export ohne Einrückung und Whitespace
* exportCompact() Verwende native Ausgabe von enums und Zeiten 
* exportExtendet() Ausgabe im Klartext von enums und Uhrzeit
* exportWoNull() Ausgabe von null-Werten überspringen

Um ein Objekt aus einem std::string einzulesen wird  string2Obj verwendet
~~~~~~~~~~cpp
mobs::string2Obj("{id:12,a:17,b:null,c:33,o:null,d:[null]}", object, mobs::ConvObjFromStr().useDontShrink().useForceNull())
~~~~~~~~~~
Im Fehlerfall wird ein std::runtime_error geworfen.

Konfiguriert werden kann:
* useCompactValues() Werte in Kurzform akzeptieren (zB. Zahl anstatt enum-Text)
* useExtentedValues() Werte in Langform akzeptieren (zB. enum-Text, Datum)
* useAutoValues() Werte in beliebiger Form akzeptieren
* useOriginalNames() Nur Original-Namen akzeptieren
* useAlternativeNames() Nur Alternativ-Namen akzeptieren
* useAutoNames() Original oder Alternativ-Namen akzeptieren
* useDontShrink() Vektoren beim Schreiben entsprechens verkleinern
* exceptionIfUnknown() werfe eine Exception falls ein Element nicht gefunden wird; ansonsten werden zusätzliche Felder ignoriert
* useExceptNull() null-Elemente werden beim Einlesen abhängig von "USENULL"  auf null gesetzt. Im Fehlerfall erfolgt eine Exception
* useOmitNull() null-Elemente werden beim Einlesen überlesen
* useClearNull() null-Elemente werden beim Einlesen abhängig von "USENULL"  auf null gesetzt. Ansonsten nur gelöscht.
* useForceNull() null-Elemente werden beim Einlesen unabhängig von "USENULL"   auf null gesetzt.


### Traversierung
Über einen Operator kann ein Objekt rekursiv durchlaufen werden
Über Callback-Functions könen Aktionen bei
* Betreten eines Objektes
* Verlassen eines Objektes
* Betreten eines Vectors
* Verlassen eines Vectos
* Zugriff auf einen Basistyp
ausgelöste werden.

### Dynamische Navigation innerhalb eines Objektes
Über den ObjectNavaigator könne Objekte-Teile gezielt angesprochen werden.


### Null-Values
Alle Elemente haben zusätliche Eigenschaften für NULL.
Wurde eine Variable mit "USENULL" definiert, so wir sie standartmäßig mit null initialisiert.
Dies kann zur Laufzeit über  nullAllowed(bool) angepasst werden. 
Wird ein Objekt auf null gesetzt, werden alle Unterelemente ebenfalls gelöscht.
Wird Unterelement eines null-Objektes beschrieben, so wird automatisch der null-Status aufgehoben

## Objektverwaltung NamedPool
Über ein extra Modul könen beliebige Objekte in einen Pool abgelegt werden. Der Zugriff erfogt nur
über den Objekt-Namen.

Darüber lassen sich
* einfache Caches
* Objekt-Referenzen,
* On-Demand Datenbakkzugriffe
* in-Memory Datenbanken
usw. realiesieren

## Installation
Zum Übersetzen wird ein c++11 Compiler inkl. STL benötigt. Für die Test-Suite wird googletest benötigt
Die Entwicklung efolgt mit clang version 11.0.3, sollte aber auch mit anderen Compilern funktionieren
Einige optionale Module benötigen rapidjson (dumpjson und readjson) diese dienen nur als Beispiel und können  ohne Funktionseinschränkung weggelassen werden

Das Makefile ist rudimentär und eine Installationsroutine fehlen noch. 

Dies folgt in einer späteren Version

## Module
* objtypes.h Typ-Deklarationen
* objgen.h  Deklaration um die Zielklassen zu generieren
* jsonparser.h  Einfacher JSON-Parser
* xmlparser.h  Einfacher XML-Parser
* xmlout.h  Klasse um  XML zu generieren
* objpool.h  Klassen für Named Objects 
* logging.h Makros und Klassen für Logging und TRACING auf stderr 
* unixtime.h Wrapper Klasse für Datum-Zeit auf basis von Unix-time_t




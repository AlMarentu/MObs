#  MObs
Bibliothek für serialisierbare C++-Objekte zur Übergabe oder zum Speichern in Datenbanken

[MObs GitHub](https://github.com/AlMarentu/MObs)
Lizenz: LGPL

## Einführung

Werden Programme komplexer, müssen Daten zwischen verschiedenen Schichten ausgetauscht, über Netzwerke übertragen oder 
in Datenbanken abgelegt werden.
Für solche Serialisierungen von Daten-Strömen/-Objekten hat sich mittlerweile JSON oder XML als Standard etabliert.
Zum Lesen bzw. Speichern in Datenbanken existieren verschiedene Mechanismen. So wird bei NO-SQL-Datenbanken idR.
auf JSON zurückgegriffen, in SQL-Datenbanken wird eine tabellenartige Struktur verwendet.

In C++-Programmen werden Strukturen jedoch idealerweise in Klassen abgelegt. Dadurch ergeben sich Typ-Sicherheit und 
hierarchische Definitionen.

MObs versucht hier die typische C++-Klassenstruktur in die gewünschten Zielstrukturen abzubilden, indem
- Datei Im- und Exportschnittstellen für XML (in den Zeichensätzen UTF-8, ISO8859-1,9,15, UTF1-6)
- Konvertierroutinen von und nach JSON
- Datenbankoperationen (MongoDb[^1], MariaDb[^1], Informix[^2])
angeboten werden.
In SQL-Datenbanken werden automatisch Master-Detail-Tabellen angelegt, um Arrays von Subobjekten ablegen zu können.

Mobs vereint hier eine Vereinfachung der Deklaration über Basisklassen und Definitionsmakros. So lässt sich sehr einfach 
eine Struktur abbilden, die dann entsprechend
weiterverarbeitet werden kann. Die Mobs-Objekte liefern automatische Getter und Setter, Methoden zur Traversierung und 
Navigation sowie textbasierte Zugriffe auf die Elemente.

Weitere Features sind die
- Behandlung von NULL-values
- Möglichkeit der Definition von primär-Schlüsseln aus mehreren Elementen
- Versionierung von Datensätzen, um Veränderungen zwischen Lesen und Speichern zu erkennen
- Audit Trail Unterstützung, jede Änderung kann protokolliert werden
- Transaktionen bei Datenbankoperationen
- Markierung von geänderten Elementen
- Verwaltung von enum-Tags im Klartext
- Ablage verschiedener Objekte in einer Liste (MobsUnion)
 
Unterstützt werden alle gängigen C++ und STL Basistypen.

Erweitert werden dies Funktionen durch eine Klasse mit benamten-Zeigern. 
Mobs-Objekte können in einen Daten-Pool abgelegt, und über einen Schlüsselwert wieder abgerufen werden.
So kann eine simple In-Memory Datenbank aufgebaut werden oder Objekt-Caches angelegt werden.


## MObs Objektdefinition

Die Konstruktoren und nötigen Hilfsfunktionen werden über das Makro ObjInit(<class name>); erzeugt.

Die Zugriffsmethoden für einfache Variablen erzeugt das Makro MemVar<type>, <variable name>);

~~~~~~~~~~cpp
#include "mobs/objgen.h"

class Fahrzeug : virtual public mobs::ObjectBase
{
   public:
     ObjInit(Fahrzeug);                     // Makro zum Initialisieren, Name muss mit Klassennamen übereinstimmen

     MemVar(std::string, fahrzeugTyp);      // Makro für einen Member-Variable
     MemVar(int,         anzahlRaeder);
}
~~~~~~~~~~

Der Zugriff auf die Variablen erfolgt dann über Getter und Setter-Methoden
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
* std::chrono::time_point mit Tagen (mobs::MDate) oder Mikrosekunden (mobs::MTime)

Die Klasse UxTime steht als Wrapper für den Typ time_t zur Verfügung, um Zeitpunkte in den Objekten verwenden zu können

Zusätzlich können auch wieder MObs-Objekte als Member verwendet werden sowie Vektoren von diesen Elementen.

Dazu stehen die Makros
* MemVar für Basisitypen
* MemObj für Objekte
* MemVarVector für Vektor von Basisitypen
* MemVector für Vektoren von Objekt-Klassen
* MemMobsEnumVar, MOBS_ENUM_DEF und MOBS_ENUM_VAL für enum-definitionen
zur Verfügung
~~~~~~~~~~cpp
MOBS_ENUM_DEF(device,  fax,   sms,   mobil,   privat,   arbeit );
MOBS_ENUM_VAL(device, "fax", "sms", "mobil", "privat", "arbeit" );

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
Die Vektoren werden bei Zugriffen automatisch vergrößert, so dass eine Überprüfung entfallen kann.

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
Über das Token EMBEDDED bei einer Objektvariable werden alle Elemente dieser beim Serialisieren flach, also auf der Ebene 
der Variable selbst eingebunden.

Beim Token DBJSON wird das betroffene Objekt als JSON-Text abgelegt. Dies ist ausschließlich für spaltenbasierte Speicherung 
vorgesehen. 

## Features
### Basisklasse
Alle MObs-Objekte sind von  mobs::ObjectBase abgeleitet. Alle Funktionen und Zugriffe auf die Elemente sind auch über die Basisklasse möglich.

~~~~~~~~~~cpp
mobs::ObjectBase *b = ...
b->setVariable("kontakte[3].number", "00-00-00");
cout << b->kontakte[1].number();

cout << b->to_string();  // Ausgabe als vereinfachtes JSON
~~~~~~~~~~

### Dynamische Objekte
Mobs-Objekte könne einen dynamischen Anteil in form von Ableitungen einer Basisklasse enthalten.

Dazu existiert eine Methode um beliebige Mobs-Objekte einer Basisiklasse in einen Vektor<MobsUnion> aufzunehmen. Das Objekt eines solchen 
MobsUnions kann zur Laufzeit ausgetauscht werden.

### Null-Values
Alle Elemente haben zusätzliche Eigenschaften für NULL.
Wurde eine Variable mit "USENULL" definiert, so wir sie standardmäßig mit null initialisiert.
Dies kann zur Laufzeit über  nullAllowed(bool) angepasst werden. 
Wird ein Objekt auf null gesetzt, werden alle Unterelemente ebenfalls gelöscht.
Wird Unterelement eines null-Objektes beschrieben, so wird automatisch der null-Status aufgehoben

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
* exportExtended() Ausgabe im Klartext von enums und Uhrzeit
* exportWoNull() Ausgabe von null-Werten überspringen

Um ein Objekt aus einem std::string einzulesen wird  string2Obj verwendet
~~~~~~~~~~cpp
mobs::string2Obj("{id:12,a:17,b:null,c:33,o:null,d:[null]}", object, mobs::ConvObjFromStr().useDontShrink().useForceNull())
~~~~~~~~~~
Im Fehlerfall wird ein std::runtime_error geworfen.

Konfiguriert werden kann:
* useCompactValues() Werte in Kurzform akzeptieren (zB. Zahl anstatt enum-Text)
* useExtendedValues() Werte in Langform akzeptieren (zB. enum-Text, Datum)
* useAutoValues() Werte in beliebiger Form akzeptieren
* useOriginalNames() Nur Original-Namen akzeptieren
* useAlternativeNames() Nur Alternativ-Namen akzeptieren
* useAutoNames() Original oder Alternativ-Namen akzeptieren
* useDontShrink() Vektoren beim Schreiben entsprechend verkleinern
* exceptionIfUnknown() werfe eine Exception falls ein Element nicht gefunden wird; ansonsten werden zusätzliche Felder ignoriert
* useExceptNull() null-Elemente werden beim Einlesen abhängig von "USENULL"  auf null gesetzt. Im Fehlerfall erfolgt eine Exception
* useOmitNull() null-Elemente werden beim Einlesen überlesen
* useClearNull() null-Elemente werden beim Einlesen abhängig von "USENULL"  auf null gesetzt. Ansonsten nur gelöscht.
* useForceNull() null-Elemente werden beim Einlesen unabhängig von "USENULL"   auf null gesetzt.

### Binäre Objekte

Als binäres Objekt kann eine MemVar vom Typ std::vector<u_char> verwendet werden. Es ist ratsam, keine zu großen Objekte
zu speichern, da bei einigen Operationen, wie auch der Zuweisung, der Inhalt umkopiert werden muss. 

Bei der Serialisierung wird automatisch in Base64 gewandelt

### Traversierung
Über einen Operator kann ein Objekt rekursiv durchlaufen werden
Über Callback-Functions können Aktionen bei
* Betreten eines Objektes
* Verlassen eines Objektes
* Betreten eines Vectors
* Verlassen eines Vektos
* Zugriff auf einen Basistyp
ausgelöste werden.

### Dynamische Navigation innerhalb eines Objektes
Über den ObjectNavigator könne Objekte-Teile gezielt angesprochen werden.


## Datenbank-Interface

### Datenbank Manager
Alle Datenbanken werden im DatabaseManager zusammengefasst. Dort werden alle Datenbanken die verwendet werden registriert.
Der DatabaseManager kann nur einmal instanziiert werden. Dies muss nach main() erfolgen. Das Objekt ist so anzulegen,
dass es beim Verlassen des Scopes von main() auch wieder vernichtet wird, um korrekt aufzuräumen. (Siehe CERT-ERR58-CPP)

~~~~~~~~~~cpp
main() {
  mobs::DatabaseManager dbMgr;
  dbMgr.addConnection("my_mongo_db", mobs::ConnectionInformation("mongodb://localhost:27017", "mobs"));
  dbMgr.addConnection("my_maria_db", mobs::ConnectionInformation("mariadb://localhost:", "mobs"));
  dbMgr.addConnection("my_informix", mobs::ConnectionInformation("informix://ol_informix1210", "mobs", user, passwd));
~~~~~~~~~~
Über eine static Member kann global darauf zugegriffen werden.
Vom DatabaseManager werden Kopien vom DatabaseInterface abgerufen.
~~~~~~~~~~cpp
  mobs::DatabaseInterface dbi = dbMgr.getDbIfc("my_mongo_db");
~~~~~~~~~~

### DatabaseInterface
Das DatabaseInterface dient als Schnittstelle für alle Datenbankoperationen. Dazu muss in den den Objektdefinitionen
ein oder mehrere Schlüsselelemente definiert werden, die als Primary-Key dienen. 
Es können noch weitere Optionsfelder wie abweichende Datenbankbezeichner oder Versionierungs-Informationen
angegeben werden.
~~~~~~~~~~cpp
 class MobsObject : virtual public mobs::ObjectBase
 {
 public:
   ObjInit(MobsObject, COLNAME(vehicle));  // COLNAME übergibt einen vom Klassennamen abweichenden Tabellennamen an
 
   MemVar(int, id, KEYELEMENT1);          // KEYELEMENT definiert die Schlüsselelemente
   MemVar(int, version, VERSIONFIELD);    // VERSIONFIELD definiert das Versionsfeld, falls Versionierung erwünscht
   MemVar(string, typ, ALTNAME(bezeichnug) LENGTH(50));
                                          // Weiterer Inhalt, mit alternativen Spaltennamen und Angabe einer optionalen Feldlänge
~~~~~~~~~~
Der Zugriff auf Objekte erfolgt dann, indem die Schlüsselelemente des Objektes gesetzt und im
Anschluss das Objekt vom DatabaseInterface angefordert wird.
~~~~~~~~~~cpp
   MobsObjetct f3;
    f3.id(2);
    if (dbi.load(f3))
      ...
~~~~~~~~~~
Zum Speichern wird einfach das gefüllte Objekt mittels save an das DatabaseInterface übergeben.

Für Die Suche kann entweder die Bedingung als String übergeben werden. Diese Bedingung muss aber
entsprechend der Datenbank formuliert werden.

Einfacher ist die Query By Example. Hier wird anhand beliebig vorbesetzter Felder eines Objektes
gesucht:
~~~~~~~~~~cpp
  f2.clearModified();
  f2.haenger[0].achsen(2);
  for (auto cursor = dbi.qbe(f2); not cursor->eof(); cursor->next()) {
    dbi.retrieve(f2, cursor);
    ...
  }
~~~~~~~~~~

Wird über den Inhalt eines Sub-Arrays gesucht, darf nur das erste Element eines Vektors benutzt werden. Vektorelemente
größer als 0 werden ignoriert.
Werden mehrere Felder gesetzt, so müssen alle Bedingungen zutreffen.

Soll die Ausgabe sortiert erfolgen, son kann bei query() und qbe() eine Sortierangabe ergolgen:

~~~~~~~~~~cpp
class Fahrzeug : public ObjectBase
{
  public:
    ObjInit(Fahrzeug);
 
    MemVar(std::string, fahrzeugTyp, KEYELEMENT1);
    MemVar(int,         anzahlRaeder);
    MemVar(float,       leistung);
};
...
Fahrzeug f;
QueryOrder sort;
sort << f.leistung << QueryOrder::descending << f.anzahlRaeder;
auto cursor = dbi.qbe(f, sort);
~~~~~~~~~~

Reicht die Möglichkeit von "Query by Example" nicht aus, so kann über den QueryGenerator eine
erweiterte Bedingung erzeugt werden. Diese ist im Idealfall dann Datenbank-unabhängig.

~~~~~~~~~~cpp
 Fahrzeug f;

 using Q = mobs::QueryGenerator;    // Erleichtert die Tipp-Arbeit
 Q filter;
 filter << f.anzahlRaeder.QiIn({2,6}) << f.leistung.Qi(">", 90.5);
 auto cursor = dbi.query(f, filter);
~~~~~~~~~~
Standardmäßig werden die Filter Und-verknüpft. 
Ein QueryOrder-Objekt kann als 3. Parameter bei query() angegeben werden.

Eine komplexe Abfrage mit verschachtelten Und/Oder-Bedingungen könnte wie folgt aussehen:

~~~~~~~~~~cpp
 using Q = mobs::QueryGenerator;
 Q filter;
 filter << Q::AndBegin << kunde.Nr.Qi("!=", 7)
        << Q::OrBegin << Q::Not << kunde.Ort.Qi("LIKE", "%Nor_heim") << kunde.Ort.Qi("=", "Nordheim") << Q::OrEnd
        << kunde.eintritt.QiBetween("2020-01-01", "2020-06-30") << Q::Not << kunde.status.QiIn({7,8,3}) << Q::AndEnd;
~~~~~~~~~~

Für Extremfälle kann auch ein freier Filtertext angegeben werden. Dabei werden Membervariablen in ihren Datenbanknamen 
umgewandelt. Dieser Filter ist dann jedoch dann spezifisch für die jeweilige
Datenbank. Hier MongoDB:
~~~~~~~~~~cpp
 filter << kunde.Name.QiNotNull() << Q::literalBegin 
        << "{ \\"$expr\" : { \\"$in\" : [\\"$" << kunde.Ort << "\", \\"$" << kunde.SearchText[0].Value << "\"]}}" 
        << Q::literalEnd;
~~~~~~~~~~

Um verschachtelte Objekte in eine SQL-Datenbank schreiben zu können stehen folgende Featuere-Token
zur Verfügung:
- DBJSON: Speichert das Unterobjekt als JSON-Text
- PREFIX(xx): Setzt bei EMBEDDED Objekten vor jedem Namen ein Prefix
- DBDETAIL: Markiert ein Unterobjekt als Detail-Tabelle; diese wird nicht bearbeitet und muss anwendungsseitig 
  extra bearbeitet werden


### Atomizität
Alle Operationen auf das DatabaseInterface werden atomar ausgeführt, soweit von der Datenbank unterstützt.
Dies gilt auch, wenn bei SQL die Sub-Elemente eines Arrays in Master-Detail-Tabellen gespeichert werden.

Für zusammenhängende Operationen mehrerer beteiligter Objekte, steht eine Transaktion zur Verfügung.

Hier wird die Gesamt-Operation als Lambda-Funktion definiert, und dann komplett vom DatabaseManager
ausgeführt.
~~~~~~~~~~cpp
   mobs::DatabaseManager::transaction_callback transCb = 
           [&f1,&f2](mobs::DbTransaction *trans) {

       mobs::DatabaseInterface t_dbi = trans->getDbIfc("my_database");
       t_dbi.save(f2);
       t_dbi.destroy(f1);
       ...
   };
 
   mobs::DatabaseManager::execute(transCb);
~~~~~~~~~~
Dabei muss das DatabaseInterface erneut, vom Transaktionsinterface, geholt werden.

Wird innerhalb der Callback-Funktion eine Exception geworfen, so wird ein Rollback ausgeführt.
Der Inhalt aller involvierten Speicher-Objekte ist dann inkonsistent.

Die Atomizität bei Operationen über verschiedenen Datenbanken wird hier nur insoweit geleistet,
als dass der finale Commit für alle Datenbanken am Enden der Transaktion nacheinander abläuft.
 
### Objekt-Versionierung
Wird über das Token VERSIONFIELD eine Versions-Variable definiert, wird dort automatisch die
Version des Datensatzes verwaltet. Je Objekt darf nur eine Versionsvariable existieren. Sie muss 
im selben Ebenenbereich liegen, wie Schlüsselelemente.

Version 0 bedeutet, dass dieses Objekt noch nicht in der Datenbank existiert.

Bei jedem Speichern wird die Version automatisch hochgezählt. Bei allen Operationen wird geprüft,
ob die Objekt-Version mit der Datenbank-Version übereinstimmt. Im Fehlerfall wird die Operation 
abgebrochen.

Bei den SQL-Feature mit automatischen Master-Detail-Tabellen, Wird die Version nur in der 
Mater-Tabelle verwaltet.

### Audit Trail
Für jedes Objekt kan definiert werden, ob eine automatische Audit Trail Überwachung aktiviert
werden soll.
Dazu muss nur Das Token  "AUDITTRAIL" angegeben werden. 
Danach werden Erzeugung, alle Änderungen sowie die Löschung protokolliert. So lässt sich der Zustand des Objektes zu 
jedem beliebigen Zeitpunkt ermitteln.
Die Änderungsaktivitäten, werden pro Transaktion zusammengefasst und im Datenpool des jeweiligen
Objektes abgelegt. Werden einzelne Objekte gespeichert, so werden die Änderungsdaten zusammen mit dem Objekt
 implizit in einer Transaktion gespeichert. Zu jeder Transaktion kann auch ein Begründungstext mit angegeben werden.
Zusätzlich werden Zeit, User (Id) und Job-Id mitprotokolliert. Über die JobId kann dann bei jedem Programmstart auch eine 
weitere Info jedes ausgeführte Programm erfasst werden.

Idealerweise sollte dazu auch Objekt-Versionierung aktiviert werden. 

## Objektverwaltung NamedPool
Über ein extra Modul können beliebige Objekte in einen Pool abgelegt werden. Der Zugriff erfolgt nur
über den Objekt-Namen.

Die Verwendung der einzelnen Objekte kommt dann globalen Variablen gleich und führt zu analogen Nebeneffekten.
Einer Verwendung als globalen Object-Cache wird, aus langjähriger Erfahrung, dringend abgeraten. 

Darüber lassen sich
* einfache Caches
* Objekt-Referenzen,
* On-Demand Datenbankzugriffe
* in-Memory Datenbanken
usw. realisieren

## Installation
Zum Übersetzen wird ein c++11 Compiler inkl. STL benötigt. Zusätzlich muss die UUID-Lib vorhanden sein.
Für die Test-Suite wird googletest benötigt
Die Entwicklung erfolgt mit clang version 11.0.3

Build und Installation erfolgen über cmake
Folgende Defines können angegeben werden:
- BUILD_MARIA_INTERFACE
- BUILD_SQLITE_INTERFACE
- BUILD_MONGO_INTERFACE
- BUILD_INFORMIX_INTERFACE
- BUILD_DOC
- PACKAGE_TESTS

Für die jeweiligen Optionen werden die entsprechenden Zusatzpakete benötigt.

Bei Informix Connect-Client ab ca. 4.10.12 wurde ein BUG in der Datums-Konvertierung entfernt
hier kann für ältere Versionen INFORMIX_DTFMT_BUG definiert werden

## Datenbanken

### Unterstützte Funktionen

| Datenbanken | MariaDB | MongoDB | SQLite | Informix | 
|-------------|---------|---------|--------|----------|
| uint64_t    | +       | +       | -      | -        |
| MTime Auflösung | 1 µs    | 1000 µs | 1µs | 10 µs   |
| Binary-Variable | Varchar | binary | blob | Varchar |
| Transaktionen | +     | -       | +       | +       |
| DB-Name     | +       | +       | -       | +       |
| Zugriffskontrolle (Login) | + | -       | -       | +       |

* Binärdaten werden bei MariaDB und Informix als BASE64 im Varchar gespeichert
* Zeitangaben werden bei SQLite als Text gespeichert

* Informix benötigt eine Environment-Variable CLIENT_LOCALE die auf einen UTF-8 Zeichensatz
 gestellt ist z.B.: de_DE.UTF8
 
### URLs
* mariadb://\<host>\[.\<port>] 
* mongodb://\<host>\[.\<port>] 
* sqlite://\<file>
* sqlite://:memory: 
* informix://\<sql_host_entry> 


## TODOs

* Bei SQL-Query Joins optimieren
* Bei MongoDB Transaktionen im Cluster-Mode unterstützen
* Zugriffskontrolle Mongo
* MariaDB + Informix: Blob unterstützen 
* SQLite: Datum nicht als string speichern
* Vervollständigung DB: Isolation, transaction level, timeouts




## Module
* objtypes.h Typ-Deklarationen
* objgen.h  Deklaration um die Zielklassen zu generieren
* union.h  Deklaration um variable Objekttypen analog Union zu generieren
* converter.h	Hilfsklassen für codecs und base64
* jsonparser.h  Einfacher JSON-Parser
* xmlparser.h  Einfacher XML-Parser
* xmlread.h	Klasse um Objekte aus einen XML-String/File auszulesen
* xmlwriter.h	Klasse zur Ausgabe von XML in Dateien oder als String in verschiedenen Zeichensätzen
* xmlout.h  Klasse zur Ausgabe von Mobs-Objekte im XML-Format
* objpool.h  Klassen für Named Objects 
* dbifc.h Datenbank Interface
* queryorder.h Zusatzmodul für Datenbankabfrage mit Sortierung
* querygenerator.h Zusatzmodul für Filter bei Datenbankabfrage
* logging.h Makros und Klassen für Logging und Tracing auf stderr 
* unixtime.h Wrapper Klasse für Datum-Zeit auf Basis von Unix-time_t
* mchrono.h Definitionen für Zeit und Datum aus std::chrono
* audittrail.h Datenbankobjekte des Audit Trails
* helper.h Interne Hilfsfunktionen
* maria.h Datenbankinterface 
* mongo.h Datenbankinterface 
* sqlite.h Datenbankinterface 
* informix.h Datenbankinterface 
* infxtools.h Hilfsfunktionen Informix



[^1]: MongoDb und MariaDB sind eingetragene Markenzeichen der jeweiligen Firmen
[^2]: IBM Informix ist ein eingetragenes Markenzeichen der IBM Corp.
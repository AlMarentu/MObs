
/** \example Erzeugung von Objekten, Serialisierung in XML-Datei mit anschließendem Einlesen */


#include "mobs/logging.h"
#include "mobs/objgen.h"
#include "mobs/xmlout.h"
#include "mobs/xmlwriter.h"
#include "mobs/xmlread.h"
#include <fstream>



using namespace std;

//Objektdefinitionen
class Fahrzeug : virtual public mobs::ObjectBase
{
public:
  ObjInit(Fahrzeug);


  MemVar(string, typ);
  MemVar(int, achsen, USENULL);
  MemVar(bool, antrieb);

  void init() override { TRACE(""); };
};


class Gespann : virtual public mobs::ObjectBase
{
public:
  ObjInit(Gespann);


  MemVar(int, id, KEYELEMENT1);
  MemVar(string, typ);
  MemObj(Fahrzeug, zugmaschiene);
  MemVector(Fahrzeug, haenger);

  void init() override { TRACE(""); };
};
ObjRegister(Gespann);


// Hilfsklasse zum Einlesen von XML-Dateien
class XmlInput : public mobs::XmlReader {
public:
  explicit XmlInput(wistream &str) : XmlReader(str) { }

  void StartTag(const std::string &element) override {
    LOG(LM_INFO, "start " << element);
    // Wenn passendes Tag gefunden, dann Objekt einlesen
    if (elementRemovePrefix(element) == "Gespann")
      fill(new Gespann);
  }
  void EndTag(const std::string &element) override {
    LOG(LM_INFO, "end " << element);
  }
  void filled(mobs::ObjectBase *obj, const string &error) override {
    LOG(LM_INFO, "filled " << obj->to_string() << (error.empty() ? " OK":" ERROR = ") << error);
    delete obj;
    stop(); // optionaler Zwischenstop
  }

};

int main(int argc, char* argv[])
{
  try {
    Gespann f1, f2;

    f1.id(1);
    f1.typ("Brauereigespann");
    f1.zugmaschiene.typ("Sechsspänner");
    f1.zugmaschiene.achsen(0);
    f1.zugmaschiene.antrieb(true);
    f1.haenger[0].typ("Bräuwagen");
    f1.haenger[0].achsen(2);

    f2.id(2);
    f2.typ("Schlepper mit 2 Anhängern");
    f2.zugmaschiene.typ("Traktor");
    f2.zugmaschiene.achsen(2);
    f2.zugmaschiene.antrieb(true);
    f2.haenger[0].typ("Anhänger");
    f2.haenger[0].achsen(2);
    f2.haenger[1].typ("Anhänger");
    f2.haenger[1].achsen(2);


// Ausgabe XML

    mobs::ConvObjToString cth;
    wofstream xout("gespann.xml", ios::trunc);
    if (not xout.is_open())
      throw runtime_error("File not open");

    // Writer-Klasse mit File, und Optionen initialisieren
    mobs::XmlWriter xf(xout, mobs::XmlWriter::CS_utf16_le, true);
    xf.setPrefix(L"m:"); // optionaler XML Namespace
    mobs::XmlOut xo(&xf, cth);
    // XML-Header
    xf.writeHead();
    // Listen-Tag
    xf.writeTagBegin(L"list");

    // Objekt schreiben
    f1.traverse(xo);
    // optionaler Kommentar
    xf.writeComment(L"und noch einer");

    // Objekt schreiben
    f2.traverse(xo);

    string json = R"({
      id:3,
      typ:"PKW",
      zugmaschiene:{
        typ:"PKW",
        achsen:2,
        antrieb:true}
      })";
    f2.clear();
    mobs::string2Obj(json, f2);

    // Objekt schreiben
    f2.traverse(xo);

    // Listen-Tag schließen
    xf.writeTagEnd();
    // file schließen
    xout.close();

    // File öffnen
    wifstream xin("gespann.xml");
    if (not xin.is_open())
      throw runtime_error("in File not open");

    // Import-Klasee mit FIle initialisieren
    XmlInput xr(xin);
    xr.setPrefix("m:"); // optionaler XML Namespace

    while (not xr.eof()) {
      // File parsen
      xr.parse();
      LOG(LM_INFO, "Zwischenpause");
    }

    // fertig
    xin.close();


  }
  catch (exception &e)
  {
    LOG(LM_ERROR, "Exception " << e.what());
    return 1;
  }
  return 0;
}


/** \example Erzeugung von Objekten, Serialisierung in XML-Datei mit anschließendem Einlesen; optional mit Verschlüsselung */


#include "mobs/logging.h"
#include "mobs/objgen.h"
#include "mobs/xmlout.h"
#include "mobs/xmlwriter.h"
#include "mobs/xmlread.h"
#include "mobs/csb.h"
#include "mobs/aes.h"
#include <fstream>
#include <array>



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
#ifdef CLASSIC_IOSTREAM
    wofstream xout("gespann.xml", ios::trunc);
#else
    ofstream xout("gespann.xml", ios::trunc);
#endif
    if (not xout.is_open())
      throw runtime_error("File not open");

#ifndef CLASSIC_IOSTREAM
    // neuen Crypto Streambuffer initialisieren
    mobs::CryptOstrBuf streambuf(xout, new mobs::CryptBufAes( "12345"));
    // und wostream damit initialisieren
    std::wostream x2out(&streambuf);
    // Base64 Mode aktivieren
    x2out << mobs::CryptBufBase::base64(true);
#endif

    // Writer-Klasse mit File, und Optionen initialisieren
    mobs::XmlWriter xf(x2out, mobs::XmlWriter::CS_utf16_le, true);
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
#ifndef CLASSIC_IOSTREAM
//    x2out << mobs::CryptBufBase::base64(false);

//    x2out << mobs::CryptBufBase::final();
    // Verschlüsselung finalisieren
    streambuf.finalize();
#endif
    xout.close();

    // openssl aes-256-cbc  -d -in cmake-build-debug/gespann.xml   -md sha1 -k 12345
    // openssl aes-256-cbc  -d -in cmake-build-debug/gespann.xml -a -A  -md sha1 -k 12345

//    return 0;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // File öffnen
#ifdef CLASSIC_IOSTREAM
    wifstream xin("gespann.xml");
#else
    ifstream xin("gespann.xml");
#endif
    if (not xin.is_open())
      throw runtime_error("in File not open");

    // neuen Crypto Streambuffer initialisieren
    mobs::CryptIstrBuf streambufI(xin, new mobs::CryptBufAes("12345"));
    // base64 Modus aktivieren
    streambufI.getCbb()->setBase64(true);
    // Streambuffer an instream binden
    std::wistream x2in(&streambufI);
//  x2in >> mobs::CryptBufBase::base64(true);


  // Import-Klasee mit FIle initialisieren
    XmlInput xr(x2in);
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

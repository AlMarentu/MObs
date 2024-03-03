
/** \example Erzeugung von Objekten, Serialisierung in XML-Datei mit anschließendem Einlesen; optional mit Verschlüsselung */


#include "mobs/logging.h"
#include "mobs/objgen.h"
#include "mobs/xmlout.h"
#include "mobs/xmlwriter.h"
#include "mobs/xmlread.h"
#include "mobs/csb.h"
#include "mobs/aes.h"
#include "mobs/rsa.h"
#include <fstream>
#include <array>
#include <converter.h>


using namespace std;

bool objOk = false;
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
  MemVar(string, fahrer, XMLENCRYPT);
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
    static int cnt = 0;
    LOG(LM_INFO, "filled " << ++cnt << ": " << obj->to_string() << (error.empty() ? " OK":" ERROR = ") << error);
    if (cnt == 2) {
      if (auto gp = dynamic_cast<Gespann *>(obj)) {
        if (gp->typ() == "Schlepper mit 2 Anhängern ẞßß")
          objOk = true;
        else
          LOG(LM_ERROR, "Typ falsch: soll " << "Schlepper mit 2 Anhängern ẞßß" << " IST " << gp->typ());
      } else {
        LOG(LM_ERROR, "Objekt nicht erkannt");
      }
    }
    delete obj;
    stop(); // optionaler Zwischenstop
  }
  void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, mobs::CryptBufBase *&cryptBufp) override {
    LOG(LM_INFO, "Encryption " << algorithm << " keyName " << keyName << " cipher " << cipher);
    if (algorithm == "aes-256-cbc")
      cryptBufp = new mobs::CryptBufAes( "12345");
  }

};

#define CLASSIC_IOSTREAM

int main(int argc, char* argv[])
{
  //setlocale(LC_CTYPE, ".utf8");
  setlocale(LC_CTYPE, "");
  std::cerr << "LC_CTYPE=" << setlocale(LC_CTYPE, nullptr) << '\n';

  try {
    Gespann f1, f2;

    f1.id(1);
    f1.typ("Brauereigespann");
    f2.fahrer("Otto");
    f1.zugmaschiene.typ("Sechsspänner");
    f1.zugmaschiene.achsen(0);
    f1.zugmaschiene.antrieb(true);
    f1.haenger[0].typ("Bräuwagen");
    f1.haenger[0].achsen(2);

    f2.id(2);
    f2.fahrer("Karl-Heinz");
    f2.typ("Schlepper mit 2 Anhängern ẞßß");
    f2.zugmaschiene.typ("Traktor");
    f2.zugmaschiene.achsen(2);
    f2.zugmaschiene.antrieb(true);
    f2.haenger[0].typ("Anhänger");
    f2.haenger[0].achsen(2);
    f2.haenger[1].typ("Anhänger");
    f2.haenger[1].achsen(2);


// Ausgabe XML
#if 1

    mobs::ConvObjToString cth = mobs::ConvObjToString().setEncryptor([](){return new mobs::CryptBufAes( "12345", "john");});
    //mobs::ConvObjToString cth;
#ifdef CLASSIC_IOSTREAM
    wofstream x2out("gespann.xml", ios::trunc | ios::binary);
    if (not x2out.is_open())
      throw runtime_error("File not open");
#else
    ofstream xout("gespann.xml", ios::trunc);
    if (not xout.is_open())
      throw runtime_error("File not open");

    // neuen Crypto Streambuffer initialisieren
    mobs::CryptOstrBuf streambuf(xout, new mobs::CryptBufAes( "12345"));
    // und wostream damit initialisieren
    std::wostream x2out(&streambuf);
    // Base64 Mode aktivieren
    x2out << mobs::CryptBufBase::base64(true);
#endif

    // Writer-Klasse mit File, und Optionen initialisieren
    mobs::XmlWriter xf(x2out, mobs::XmlWriter::CS_utf8_bom, true);
    xf.setPrefix(L"m:"); // optionaler XML Namespace
    mobs::XmlOut xo(&xf, cth);
    // XML-Header
    xf.writeHead();
    xf.writeAttribute(L"xmlns", L"abc.xml");
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
      fahrer:"Peter",
      zugmaschiene:{
        typ:"PKW",
        achsen:2,
        antrieb:true}
      })";
    f2.clear();
    mobs::string2Obj(json, f2);

    // Objekt schreiben
    f2.traverse(xo);

    xf.stopEncrypt();
    // Listen-Tag schließen
    xf.writeTagEnd();
    // file schließen
#ifdef CLASSIC_IOSTREAM
    x2out.close();
#else
//    x2out << mobs::CryptBufBase::base64(false);

//    x2out << mobs::CryptBufBase::final();
    // Verschlüsselung finalisieren
    streambuf.finalize();
    xout.close();

    // openssl aes-256-cbc  -d -in cmake-build-debug/gespann.xml   -md sha1 -k 12345
    // openssl aes-256-cbc  -d -in cmake-build-debug/gespann.xml -a -A  -md sha1 -k 12345
#endif

//    return 0;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LOG(LM_INFO, "Datei Erzeugt");
#endif

    // File öffnen
#ifdef CLASSIC_IOSTREAM
    wifstream x2in("gespann.xml");
    if (not x2in.is_open())
      throw runtime_error("in File not open");
#else
    ifstream xin("gespann.xml");
    if (not xin.is_open())
      throw runtime_error("in File not open");

    // neuen Crypto Streambuffer initialisieren
    mobs::CryptIstrBuf streambufI(xin, new mobs::CryptBufAes("12345"));
    // base64 Modus aktivieren
    streambufI.getCbb()->setBase64(true);
    // Streambuffer an instream binden
    std::wistream x2in(&streambufI);
//  x2in >> mobs::CryptBufBase::base64(true);
#endif

  // Import-Klasee mit FIle initialisieren
    XmlInput xr(x2in);
    xr.setPrefix("m:"); // optionaler XML Namespace

    while (not xr.eof()) {
      // File parsen
      xr.parse();
      LOG(LM_INFO, "Zwischenpause");
    }

    // fertig
#ifdef CLASSIC_IOSTREAM
    x2in.close();
#else
    xin.close();
#endif

    if (objOk)
      LOG(LM_INFO, "Objekt gefunden");
    else
      LOG(LM_ERROR, "Fehler");

  }
  catch (exception &e)
  {
    LOG(LM_ERROR, "Exception " << e.what());
    return 1;
  }
  return 0;
}


/** \example Erzeugung von Objekten, Serialisierung in XML-Datei mit RSA-Verschlüsselung anschließendem Einlesen */


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
    LOG(LM_INFO, "filled " << obj->to_string() << (error.empty() ? " OK":" ERROR = ") << error);
    delete obj;
    stop(); // optionaler Zwischenstop
  }
  void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, mobs::CryptBufBase *&cryptBufp) override {
    LOG(LM_INFO, "Encryption " << algorithm << " keyName " << keyName << " cipher " << cipher);
    if (algorithm == "rsa-1_5" and keyName == "Det") {
      vector<u_char> data;
      mobs::from_string_base64(cipher, data);
      cryptBufp = new mobs::CryptBufRsa("p2pr.pem", data, "222");
    }
  }

};


int main(int argc, char* argv[])
{
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
    f2.typ("Schlepper mit 2 Anhängern");
    f2.zugmaschiene.typ("Traktor");
    f2.zugmaschiene.achsen(2);
    f2.zugmaschiene.antrieb(true);
    f2.haenger[0].typ("Anhänger");
    f2.haenger[0].achsen(2);
    f2.haenger[1].typ("Anhänger");
    f2.haenger[1].achsen(2);


    // RSA-Schlüssel erzeugen (sollten bereits vorhanden sein)
    mobs::generateRsaKey("p1pr.pem", "p1pu.pem", "111");
    mobs::generateRsaKey("p2pr.pem", "p2pu.pem", "222");
    mobs::generateRsaKey("p3pr.pem", "p3pu.pem", "333");


    // Ausgabe XML
    mobs::ConvObjToString cth;
    wofstream x2out("rsa.xml", ios::trunc);
    if (not x2out.is_open())
      throw runtime_error("File not open");

    // Writer-Klasse mit File, und Optionen initialisieren
    mobs::XmlWriter xf(x2out, mobs::XmlWriter::CS_utf8, true);
    mobs::XmlOut xo(&xf, cth);
    // XML-Header
    xf.writeHead();
    // Listen-Tag
    xf.writeTagBegin(L"list");

    std::list<mobs::CryptBufRsa::PubKey> pks;
    pks.emplace_back("p1pu.pem", "Charlie");
    pks.emplace_back("p2pu.pem", "Det");
    pks.emplace_back("p3pu.pem", "Egon");
    xf.startEncrypt(new mobs::CryptBufRsa(pks));

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
    x2out.close();




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LOG(LM_INFO, "Datei Erzeugt");

    // File öffnen

    wifstream x2in("rsa.xml");
    if (not x2in.is_open())
      throw runtime_error("in File not open");

    // Import-Klasee mit FIle initialisieren
    XmlInput xr(x2in);

    while (not xr.eof()) {
      // File parsen
      xr.parse();
      LOG(LM_INFO, "Zwischenpause");
    }

    // fertig
    x2in.close();

  }
  catch (exception &e)
  {
    LOG(LM_ERROR, "Exception " << e.what());
    return 1;
  }
  return 0;
}

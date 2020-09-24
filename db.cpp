
#include "mobs/logging.h"
#include "mobs/objgen.h"
#include "mobs/dbifc.h"
#include "mobs/queryorder.h"
#include "mobs/querygenerator.h"
#include "mobs/unixtime.h"
#include "mobs/mchrono.h"
#include "mobs/audittrail.h"

using namespace std;



namespace gespann {


//Objektdefinitionen
class Fahrzeug : virtual public mobs::ObjectBase
{
public:
  ObjInit(Fahrzeug);

  MemVar(string, typ, LENGTH(40));
  MemVar(int, achsen, USENULL);
  MemVar(bool, antrieb);
};


class Gespann : virtual public mobs::ObjectBase
{
public:
  ObjInit(Gespann, COLNAME(vehicle) AUDITTRAIL);

  MemVar(int, id, KEYELEMENT1);
  MemVar(int, version, VERSIONFIELD);
  MemVar(string, typ, ALTNAME(bezeichnug) LENGTH(50));
  MemObj(Fahrzeug, zugmaschiene);
  MemVector(Fahrzeug, haenger, COLNAME(vehicle_part));
};
ObjRegister(Gespann);

void worker(mobs::DatabaseInterface &dbi) {
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
    f2.typ("Schlepper mit 1 Anhänger");
    f2.zugmaschiene.typ("Traktor");
    f2.zugmaschiene.achsen(2);
    f2.zugmaschiene.antrieb(true);
    f2.haenger[0].typ("Anhänger");
    f2.haenger[0].achsen(1);

    // Tabelle/Collection löschen !!!
    dbi.dropAll(f1);
    // Strukturen neu anlegem
    dbi.structure(f1);

    dbi.save(f1);
    dbi.save(f2);

    f2.typ("Schlepper mit 2 Anhängern");
    f2.haenger[0].achsen(2);
    f2.haenger[1].typ("Anhänger");
    f2.haenger[1].achsen(2);
    dbi.save(f2);

    vector<string> objs {
            R"({ id:3, typ:"PKW", zugmaschiene:{ typ:"PKW", achsen:2, antrieb:true}})",
            R"({ id:4, typ:"Mutter mit Kind", zugmaschiene:{ typ:"Fahhrad", achsen:2, antrieb:true}, haenger:[
                 { "typ" : "Fahrradanhänger", "achsen" : 1 } ]})",
            R"({ id:5, typ:"Damfplokomotive", zugmaschiene:{ typ:"Lokomotive", achsen:10, antrieb:true}, haenger:[
                 { "typ" : "Tender", "achsen" : 4 } ]})",
    };
    for (auto const &j:objs) {
      f2.clear();
      mobs::string2Obj(j, f2);
      dbi.save(f2);
    }

    // Query By Example
    f2.clearModified();
    f2.haenger[0].achsen(2);
    mobs::QueryOrder sort;
    sort << f2.haenger[0].typ;
    for (auto cursor = dbi.withQueryLimit(300).withQuerySkip(1).qbe(f2, sort); not cursor->eof(); cursor->next()) {
      dbi.retrieve(f2, cursor);
      LOG(LM_INFO, "QBE result: pos=" << cursor->pos() << " id=" << f2.id() << " " << f2.typ());
    }

    // Count
    auto cursor = dbi.withCountCursor().query(f2, mobs::QueryGenerator());
    LOG(LM_INFO, "Anzahl " << cursor->pos());


    Gespann f3;
    f3.id(2);
    if (dbi.load(f3))
      LOG(LM_INFO, "Gespann 2 hat " << f3.haenger.size() << " Anhänger");
    else
      LOG(LM_ERROR, "Gesapann 2 exisitiert nicht");

    f3.id(12);
    if (not dbi.load(f3))
      LOG(LM_INFO, "Gesapann 12 exisitiert nicht");

    f3.id(4);

    string conName = dbi.connectionName();
    // Transaktion als Lambda Funktion; wird dort eine Exception geworfen, wird automatisch ein Rollback ausgeführt
    mobs::DatabaseManager::transaction_callback transCb = [&f3, conName](mobs::DbTransaction *trans) {
      LOG(LM_INFO, "Transaktion mit " << conName);
      mobs::DatabaseInterface t_dbi = trans->getDbIfc(conName);
      // Mit Objektversionierung muss das Objekt vorher geladen werden um die Version zu ermitteln
      if (not t_dbi.load(f3))
        LOG(LM_INFO, "Gespann 4 existiert nicht");

      if (t_dbi.destroy(f3))
        LOG(LM_INFO, "Gespann 4 gelöscht");

      Gespann f;
      f.id(1);
      if (t_dbi.load(f)) {
        f.zugmaschiene.antrieb(false);
        t_dbi.save(f);
      }
    };

    // Transaktion ausführen
    mobs::DatabaseManager::execute(transCb);



    class Data2 : virtual public mobs::ObjectBase
    {
    public:
      ObjInit(Data2);
      MemVar(std::string, aa);
      MemVarVector(int, zz);
    };

    class Data : virtual public mobs::ObjectBase
    {
    public:
      ObjInit(Data);

      MemVar(int, id, KEYELEMENT1);
      MemVar(string, text, USENULL LENGTH(2000));
      MemVar(mobs::MDate, datum, USENULL);
      MemVar(mobs::UxTime, utime, USENULL);
      MemVar(mobs::MTime, time, USENULL);
      MemVar(double, flkz, USENULL);
      MemVar(bool, an, USENULL);
      MemVar(bool, aus, USENULL);
      MemVar(char, ch, USENULL);
      MemVar(uint64_t , ulolo, USENULL);
      MemObj(Data2, d2, DBJSON LENGTH(100));
    };


    Data d,e;
    d.id(1);
    e.id(2);
    d.text("Ottos Möpse kotzen");
    d.datum.fromStrExplizit("1966-05-18");
    d.utime.fromStrExplizit("2001-02-03T12:01:02+01:00");
    d.time.fromStrExplizit("2001-02-03T12:01:02.678129");
    d.flkz(3.14);
    d.an(true);
    d.aus(false);
    d.ch('X');
    d.ulolo(1234567890123456L);
    d.d2.aa("abdc");
    d.d2.zz[0](7);
    d.d2.zz[1](5);
    d.d2.zz[2](3);

    LOG(LM_INFO, "D " << d.to_string());
    try {
      dbi.dropAll(d);
    } catch(...) {}
    dbi.structure(d);
    dbi.save(d);
    dbi.save(e);

    Data r;
    for (int i = 1; i <= 2; i++) {
      r.id(i);
      if (dbi.load(r))
        LOG(LM_INFO, "R " << r.to_string());
    }

  }
  catch (exception &e)
  {
    LOG(LM_ERROR, "Exception " << e.what());
    exit(2);
  }
}
}


int main(int argc, char* argv[])
{
  TRACE("");
  try {
    // Datenbank Verbindung einrichten
    mobs::DatabaseManager dbMgr;  // singleton, darf nur einmalig eingerichtet werden und muss bis zum letzten Verwenden einer Datenbank bestehen bleiben

    // Datenbank-Verbindungen
    dbMgr.addConnection("my_mongo", mobs::ConnectionInformation("mongodb://localhost:27017", "mobs"));
    dbMgr.addConnection("my_maria", mobs::ConnectionInformation("mariadb://localhost", "mobs"));
    dbMgr.addConnection("my_informix", mobs::ConnectionInformation("informix://ol_informix1210", "mobs", "informix", "db"));
    dbMgr.addConnection("my_sqlite", mobs::ConnectionInformation("sqlite://sqlite.db", ""));
    dbMgr.addConnection("my_sqlitem", mobs::ConnectionInformation("sqlite://:memory:", ""));

//    mobs::DatabaseInterface dbi = dbMgr.getDbIfc("my_mongo");
    mobs::DatabaseInterface dbi = dbMgr.getDbIfc("my_sqlite");
//    mobs::DatabaseInterface dbi = dbMgr.getDbIfc("my_maria");
//    mobs::DatabaseInterface dbi = dbMgr.getDbIfc("my_informix");

    gespann::worker(dbi);

  }
  catch (exception &e)
  {
    LOG(LM_ERROR, "Exception " << e.what());
    return 1;
  }
  return 0; 
}


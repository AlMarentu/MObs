
#include "mobs/logging.h"
#include "mobs/objgen.h"
#include "mobs/dbifc.h"

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
  ObjInit(Gespann, COLNAME(vehicle));

  MemVar(int, id, KEYELEMENT1);
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
    f2.typ("Schlepper mit 2 Anhängern");
    f2.zugmaschiene.typ("Traktor");
    f2.zugmaschiene.achsen(2);
    f2.zugmaschiene.antrieb(true);
    f2.haenger[0].typ("Anhänger");
    f2.haenger[0].achsen(2);

    // Tabelle/Collection löschen !!!
    dbi.dropAll(f1);
    // Strukturen neu anlegem
    dbi.structure(f1);

    dbi.save(f1);
    dbi.save(f2);

    f2.haenger[1].typ("Anhänger");
    f2.haenger[1].achsen(2);
    dbi.save(f2);

    vector<string> objs {
            R"({ id:3, typ:"PKW", zugmaschiene:{ typ:"PKW", achsen:2, antrieb:true}})",
            R"({ id:4, typ:"Mutter mit Kind", zugmaschiene:{ typ:"Fahhrad", achsen:2, antrieb:true}, haenger:[
                 { "typ" : "Fahrradanhänger", "achsen" : 1 } ]})",
            R"({ id:5, typ:"Dankplokomotive", zugmaschiene:{ typ:"Lokomotive", achsen:10, antrieb:true}, haenger:[
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
    for (auto cursor = dbi.qbe(f2); not cursor->eof(); cursor->next()) {
      dbi.retrieve(f2, cursor);
      LOG(LM_INFO, "QBE result: pos=" << cursor->pos() << " id=" << f2.id() << " " << f2.typ());
    }

    // Count
    auto cursor = dbi.withCountCursor().query(f2, "");
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
      if (t_dbi.destroy(f3))
        LOG(LM_INFO, "Gespann 4 gelöscht");
    };

    // Transaktion ausführen
    mobs::DatabaseManager::execute(transCb);


  }
  catch (exception &e)
  {
    LOG(LM_ERROR, "Exception " << e.what());
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

//    mobs::DatabaseInterface dbi = dbMgr.getDbIfc("my_mongo");
    mobs::DatabaseInterface dbi = dbMgr.getDbIfc("my_maria");

    gespann::worker(dbi);

  }
  catch (exception &e)
  {
    LOG(LM_ERROR, "Exception " << e.what());
    return 1;
  }
  return 0; 
}


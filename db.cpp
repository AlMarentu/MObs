#include <iostream>

//#define NDEBUG
#include "logging.h"



#include "objgen.h"
#include "objpool.h"

using namespace mobs;

class DatabaseInterface
{
  public:
    DatabaseInterface() {};
    virtual ~DatabaseInterface() {};
    virtual bool load(ObjectBase &obj) = 0;
    virtual bool load(std::list<ObjectBase> &result, std::string objType, std::string query) = 0;
    virtual bool save(const ObjectBase &obj) = 0;

  protected:
};

class FileDatabase : public DatabaseInterface
{
  public:
    FileDatabase(std::string path);
    virtual ~FileDatabase();
    virtual bool load(ObjectBase &obj);
    virtual bool load(std::list<ObjectBase> &result, std::string objType, std::string query);
    virtual bool save(const ObjectBase &obj);
    //bool load(std::shared_ptr<NamedObject> &ptr) { return load(*obj); };

  protected:
    std::string base;
};

////////////////

#include "xmlout.h"
#include <fstream>
#include <sstream>

using namespace std;

#if 0
class KeyString : virtual public ObjTravConst {
  public:
    virtual bool doObjBeg(ObjTravConst &ot, const ObjectBase &obj) { return true; };
    virtual void doObjEnd(ObjTravConst &ot, const ObjectBase &obj) { };
    virtual bool doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec) { return false; }; // Keine Arrays im Key
    virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec) { };
    virtual void doMem(ObjTravConst &ot, const MemberBase &mem)
    {
      TRACE("");
      if (mem.key() > 0)
      {
        keystr += ".";
        keystr += mem.toStr(false);
        //cerr << "  Mem " << mem.name() << " = ";
        //cerr << endl;
      }
    };
    const string &key() const { return keystr; };
  protected:
    string keystr;
};
#endif

FileDatabase::FileDatabase(string path) : base(path)
{
  TRACE(PARAM(path));
}

FileDatabase::~FileDatabase()
{
  TRACE("");
}

bool FileDatabase::load(ObjectBase &obj)
{
  TRACE(PARAM(obj.typName()));
  ConvToStrHint cth(false);
  list<string> key;
  obj.getKey(key, cth);

  stringstream fname;
  fname << base << "/" << obj.typName();
  for (auto &k:key)
    fname << '.' << k;
  cerr << "FileDatabase::load " << obj.typName() << " name: " << obj.keyStr() << endl;
  ifstream f(fname.str());
  if (not f.is_open())
    throw runtime_error(string("File open error ") + fname.str());
  ostringstream data;
  data << f.rdbuf();
  cout << "DATA " << data.str() << endl;
  string2Obj(data.str(), obj);

  return true;
}

bool FileDatabase::load(list<ObjectBase> &result, string objType, string query)
{
  TRACE(PARAM(objType) << PARAM(query));
  // TODO
  return false;
}

bool FileDatabase::save(const ObjectBase &obj)
{
  TRACE(PARAM(obj.typName()));
  
  stringstream fname;
  fname << base << "/" << obj.typName() << "." << obj.keyStr();
  fstream f(fname.str(), f.trunc | f.out);
  if (not f.is_open())
    throw runtime_error(string("File open error ") + fname.str());
  f << obj.to_string(ConvObjToString().exportJson()) << endl;
  f.close();
  if (f.fail())
    throw runtime_error(string("File write error ") + fname.str());

  cerr << "FileDatabase::save " << obj.typName() << " name: " << obj.keyStr() << endl;
  return true;
}

#include "objgen.h"
#include "objpool.h"

using namespace std;


class Fahrzeug : virtual public NamedObject, virtual public ObjectBase
{
  public:
    ObjInit(Fahrzeug);

    MemVar(int, id, KEYELEMENT1);
    MemVar(string, typ);
    MemVar(int, achsen, USENULL);

    void init() { TRACE(""); };
    // ist das nötig=?
    string objName() const { TRACE(""); return typName() + "." + std::to_string(id()); };
};
ObjRegister(Fahrzeug);

int main(int argc, char* argv[])
{
  TRACE("");
  try {
    shared_ptr<NamedObjPool> pool = make_shared<NamedObjPool>();

    NamedObjRef<Fahrzeug> f1(pool, "1");
    f1.create();
    f1->id(1);
    f1->typ("Traktor");
    f1->achsen(2);
    cout << "Fahrzeug[" << f1->id() << "] hat " << f1->achsen() << " Achsen und ist ein " << f1->typ() << endl;

    FileDatabase db("/Users/lautner/Work/mobs/mobs/mobs/data");
    db.save(*f1);

    NamedObjRef<Fahrzeug> f2(pool, "2");
    if (not f2.exists())
    {
      f2.create();
      f2->id(2);
//      logging::Trace::traceOn = true;

      db.load(*f2);
    }
    cout << "Fahrzeug[" << f2->id() << "] hat " << f2->achsen() << " Achsen und ist ein " << f2->typ() << endl;


    list<ObjectBase> result;
    db.load(result, "Fahrzeug", "id = 2");
    
    ConvObjToString cth;
    XmlOut dx2(ConvObjToString().doIndent());
    dx2.startList("list");
    f2->traverse(dx2);
    f2->traverse(dx2);
    cerr << dx2.getString() << endl;
    

  }
  catch (exception &e)
  {
    cerr << "Exception " << e.what() << endl;
    return 1;
  }
  return 0; 
}


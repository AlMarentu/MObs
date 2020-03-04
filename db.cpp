#include <iostream>

#include "objgen.h"
#include "objpool.h"

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

#include "dumpjson.h"
#include "readjson.h"
#include <fstream>
#include <sstream>

using namespace std;

class KeyString : virtual public ObjTravConst {
  public:
    virtual void doObjBeg(ObjTravConst &ot, const ObjectBase &obj) { };
    virtual void doObjEnd(ObjTravConst &ot, const ObjectBase &obj) { };
    virtual void doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec) { };
    virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec) { };
    virtual void doMem(ObjTravConst &ot, const MemberBase &mem)
    {
      if (mem.key() > 0)
      {
        keystr += ".";
        keystr += mem.toStr();
        //cerr << "  Mem " << mem.name() << " = ";
        //mem.strOut(cerr);
        //cerr << endl;
      }
    };
    const string &key() const { return keystr; };
  protected:
    string keystr;
};

FileDatabase::FileDatabase(string path) : base(path)
{
}

FileDatabase::~FileDatabase()
{
}

bool FileDatabase::load(ObjectBase &obj)
{
  KeyString k;
  obj.traverse(k);
  stringstream fname;
  fname << base << "/" << obj.typName() << k.key();
  cerr << "FileDatabase::load " << obj.typName() << " name: " << k.key() << endl;
  ifstream f(fname.str());
  if (not f.is_open())
    throw runtime_error(string("File open error ") + fname.str());
  ostringstream data;
  data << f.rdbuf();
  JsonRead j(data.str());
  j.fill(obj);
  return true;
}

bool FileDatabase::load(list<ObjectBase> &result, string objType, string query)
{
  cerr << "FileDatabase::load " << objType << " query: " << query << endl;
  return true;
}

bool FileDatabase::save(const ObjectBase &obj)
{
  KeyString k;
  obj.traverse(k);
  JsonOut j;
  obj.traverse(j);
  stringstream fname;
  fname << base << "/" << obj.typName() << k.key();
  fstream f(fname.str(), f.trunc | f.out);
  if (not f.is_open())
    throw runtime_error(string("File open error ") + fname.str());
  f << j.getString() << endl;
  f.close();
  if (f.fail())
    throw runtime_error(string("File write error ") + fname.str());

  cerr << "FileDatabase::save " << obj.typName() << " name: " << k.key() << endl;
  return true;
}

#include "objgen.h"
#include "objpool.h"

using namespace std;


class Fahrzeug : virtual public NamedObject, virtual public ObjectBase
{
  public:
    ObjInit(Fahrzeug);

    MemVar(int, id);
    MemVar(string, typ);
    MemVar(int, achsen);

    void init() { achsen.nullAllowed(true); keylist << id; };
    // ist das nötig=?
    string objName() const { return typName() + "." + std::to_string(id()); };
};
ObjRegister(Fahrzeug);

int main(int argc, char* argv[])
{
  try {
    cerr << "HHHH" << endl;
    shared_ptr<NamedObjPool> pool = make_shared<NamedObjPool>();

    NamedObjRef<Fahrzeug> f1(pool, "1");
    f1.create();
    f1->id(1);
    f1->typ("Traktor");
    f1->achsen(2);

    FileDatabase db("data");
    db.save(*f1);

    NamedObjRef<Fahrzeug> f2(pool, "2");
    if (f2.lock() == nullptr)
    {
      f2.create();
      f2->id(2);
      db.load(*f2);
    }
    cerr << "Fahrzeug[" << f2->id() << "] hat " << f2->achsen() << " Achsen und ist ein " << f2->typ() << endl;


    list<ObjectBase> result;
    db.load(result, "Fahrzeug", "id = 2");

  }
  catch (exception &e)
  {
    cerr << "Exception " << e.what() << endl;
  }
  return 0; 
}


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

using namespace std;

FileDatabase::FileDatabase(string path) : base(path)
{
}

FileDatabase::~FileDatabase()
{
}

bool FileDatabase::load(ObjectBase &obj)
{
  cerr << "FileDatabase::load " << obj.typName() << " name: " << obj.name() << endl;
  return true;
}

bool FileDatabase::load(list<ObjectBase> &result, string objType, string query)
{
  cerr << "FileDatabase::load " << objType << " query: " << query << endl;
  return true;
}

bool FileDatabase::save(const ObjectBase &obj)
{
  cerr << "FileDatabase::load " << obj.typName() << " name: " << obj.name() << endl;
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
cerr << "HHHH" << endl;
  shared_ptr<NamedObjPool> pool = make_shared<NamedObjPool>();

  NamedObjRef<Fahrzeug> f1(pool, "1");
  f1.create();
  f1->id(1);
  f1->typ("traktor");
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

  
  list<ObjectBase> result;
  db.load(result, "Fahrzeug", "id = 2");

  
}


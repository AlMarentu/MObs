#include "objgen.h"
#include "objpool.h"
#include "readjson.h"
#include "dumpjson.h"

#include <iostream>

using namespace std;

string js = 
"{\n"
"    \"otto\": 7,\n"
"    \"peter\": 44,\n"
"    \"pims\": null,\n"
"    \"bums\": {\n"
"        \"vorn\": \"\",\n"
"        \"hinten\": \"hh\"\n"
"    },\n"
"    \"mom\": \"GAGA\",\n"
"    \"susi\": [],\n"
"    \"luzifer\": [\n"
"        {\n"
"            \"vorn\": \"\",\n"
"            \"hinten\": \"\"\n"
"        },\n"
"        {\n"
"            \"vorn\": \"\",\n"
"            \"hinten\": \"cwluluhh1111111\"\n"
"        },\n"
"        {\n"
"            \"vorn\": \"\",\n"
"            \"hinten\": \"cwluluhh2\"\n"
"        },\n"
"        {\n"
"            \"vorn\": \"\",\n"
"            \"hinten\": \"cwluluhh333\"\n"
"        },\n"
"        {\n"
"            \"vorn\": \"\",\n"
"            \"hinten\": \"cwluluhh\",\n"
"            \"unten\": \"dkfhsfsj\"\n"
"        }\n"
"    ],\n"
"    \"friederich\": [\n"
"        \"\",\n"
"        \"aaa\",\n"
"        \"bbb\",\n"
"        \"ccc\"\n"
"    ]\n"
"}\n";


class ObjDump : virtual public ObjTrav {
  public:
    virtual void doObjBeg(ObjTrav &ot, ObjectBase &obj)
    {
      cerr << "Obj " << obj.name() << "(" << obj.typName() << ") :{" << endl;
    };
    virtual void doObjEnd(ObjTrav &ot, ObjectBase &obj)
    {
      cerr << "----}" << endl;
    };
    virtual void doArrayBeg(ObjTrav &ot, MemBaseVector &vec)
    {
      cerr << "Obj " << vec.name() << ":[" << endl;
    };
    virtual void doArrayEnd(ObjTrav &ot, MemBaseVector &vec)
    {
      cerr << "----]" << endl;
    };
    virtual void doMem(ObjTrav &ot, MemberBase &mem)
    {
      cerr << "  Mem " << mem.name() << " = ";
      mem.strOut(cerr);
      cerr << endl;
    };
};


class Part : virtual public ObjectBase {
public:
  ObjInit(Part);
  
  MemVar(string, vorn);
  MemVar(string, hinten);
};
//ObjRegister(Part); unnötig,da kein Basis-Objekt

class Info : virtual public NamedObject, virtual public ObjectBase {
public:
  ObjInit(Info);
  
  MemVar(int, otto);
  MemVar(int, peter);
  ObjVar(Part, pims);
  ObjVar(Part, bums);

  MemVar(string, mom);
  MemVector(string, susi);
  ObjVector(Part, luzifer);
  MemVector(string, friederich);

  virtual string objName() const { return typName() + "." + mom() + "." + std::to_string(otto()); };
  virtual void init() { otto.nullAllowed(true); pims.nullAllowed(true); pims.setNull(true);
                        luzifer.nullAllowed(true); luzifer.setNull(true); keylist << peter << otto; };
};
ObjRegister(Info);

int main(int argc, char *argv[])
{
  cerr << "-----" << endl;

  Info info;
  Info info2;
  info.otto(7);
  info.peter(2);
  cerr << info.otto() << endl;
  info2.otto(99);
  info2.peter(105);
  info2.mom("HAL");
  Info *ip = dynamic_cast<Info *>(ObjectBase::createObj("Info"));
  ip->mom("GAGA");
  ip->bums.hinten("hh");
  cerr << "1111" << endl;
  static_cast<MemBaseVector *>(&ip->luzifer)->resize(5);
  for (size_t i = 1; i < 5; i++)
    cerr << ip->luzifer.getObjInfo(i) << endl;
  cerr << "AA " << &ip->luzifer << " " << ip->getVecInfo("luzifer") << endl;
  cerr << "BB " << &ip->luzifer[3] << " " << dynamic_cast<Part *>(ip->luzifer.getObjInfo(3)) << endl;
  cerr << "BB " << static_cast<ObjectBase *>(&ip->luzifer[3]) << " " << ip->luzifer.getObjInfo(3) << endl;
  cerr << "CC " << ip << " " << dynamic_cast<ObjectBase *>(ip) << endl;
  cerr << "DD " << &ip->luzifer[0].vorn << " " << dynamic_cast<Member<string> *>(ip->luzifer[0].getMemInfo("vorn")) << endl;
  cerr << "DD " << &ip->luzifer[1].vorn << " " << dynamic_cast<Member<string> *>(ip->luzifer[1].getMemInfo("vorn")) << endl;
  cerr << typeid(&ip->luzifer[0].vorn).name() << endl;
  cerr << typeid(ip->luzifer[0].getMemInfo("vorn")).name() << endl;
  cerr << "DD " << dynamic_cast<MemberBase *>(&ip->luzifer[0].vorn) << " " << ip->luzifer[0].getMemInfo("vorn") << endl;
  ip->luzifer[4].hinten("cwluluhh");
  ip->luzifer[3].hinten("cwluluhh333");
  ip->luzifer[2].hinten("cwluluhh2");
  ip->luzifer[1].hinten("cwluluhh1111111");
  ip->friederich[1]("aaa");
  ip->friederich[2]("bbb");
  ip->friederich[3]("ccc");
  cerr << "2222" << endl;
  //exit(0);


  ObjDump dump;

  info.traverse(dump);

  info2.traverse(dump);
  ip->traverse(dump);
  cerr << "** ";
  ip->get("mom").strOut(cerr);
  cerr << endl;
  cerr << "######################" << endl;

  Info infoN;
  infoN = *ip;
  cerr << "2######################" << endl;
  infoN.traverse(dump);

  //JsonDump dj;
  JsonOut dj;
  infoN.traverse(dj);
  cerr << dj.getString() << endl;
  info.otto.setNull(true);
  {
    JsonOut dj2;
    info.traverse(dj2);
    cerr << dj2.getString() << endl;
  }

  cerr << "##############################################################################################################################################" << endl;
  Info read;
  //read.luzifer.resize(4);
  JsonRead jr(js);
  jr.fill(read);
  cerr << "Reader durch" << endl;
  cerr << "AA " << &read.luzifer << " " << read.getVecInfo("luzifer") << endl;
  cerr << "BB " << &read.luzifer[0] << " " << dynamic_cast<Part *>(read.luzifer.getObjInfo(0)) << endl;
  cerr << typeid(&read.luzifer[0]).name() << endl;
  cerr << "BB " << static_cast<ObjectBase *>(&read.luzifer[0]) << " " << read.luzifer.getObjInfo(0) << endl;
  cerr << "CC " << &read << " " << dynamic_cast<ObjectBase *>(&read) << endl;
  cerr << "DD " << &read.luzifer[0].vorn << " " << dynamic_cast<Member<string> *>(read.luzifer[0].getMemInfo("vorn")) << endl;
  cerr << "DD " << &read.luzifer[1].vorn << " " << dynamic_cast<Member<string> *>(read.luzifer[1].getMemInfo("vorn")) << endl;
  cerr << typeid(&read.luzifer[0].vorn).name() << endl;
  cerr << typeid(read.luzifer[0].getMemInfo("vorn")).name() << endl;
  cerr << "DD " << dynamic_cast<MemberBase *>(&read.luzifer[0].vorn) << " " << read.luzifer[0].getMemInfo("vorn") << endl;
  for (size_t i = 0; i < 5; i++)
  {
  cerr << " v  " << read.luzifer[i].vorn() << endl;
  cerr << " h  " << read.luzifer[i].hinten() << endl;
  }
  //read.traverse(jr);
  read.traverse(dump);
  cerr << "++++++++++++++++++++" << endl;
  {
    JsonOut dj2;
    read.traverse(dj2);
    cerr << dj2.getString() << endl;
  }
  cerr << "++++++++++++++++++++" << endl;

}

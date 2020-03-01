
#include "objgen.h"

class JsonOutData;
class JsonOut : virtual public ObjTrav {
  public:
    JsonOut();
    ~JsonOut();
    virtual void doObjBeg(ObjTrav &ot, ObjectBase &obj);
    virtual void doObjEnd(ObjTrav &ot, ObjectBase &obj);
    virtual void doArrayBeg(ObjTrav &ot, MemBaseVector &vec);
    virtual void doArrayEnd(ObjTrav &ot, MemBaseVector &vec);
    virtual void doMem(ObjTrav &ot, MemberBase &mem);

    std::string getString();
    void clear();
  private:
    JsonOutData *data;

};

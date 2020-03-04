
#include "objgen.h"

class JsonOutData;
class JsonOut : virtual public ObjTravConst {
  public:
    JsonOut();
    ~JsonOut();
    virtual void doObjBeg(ObjTravConst &ot, const ObjectBase &obj);
    virtual void doObjEnd(ObjTravConst &ot, const ObjectBase &obj);
    virtual void doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec);
    virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec);
    virtual void doMem(ObjTravConst &ot, const MemberBase &mem);

    std::string getString();
    void clear();
  private:
    JsonOutData *data;

};

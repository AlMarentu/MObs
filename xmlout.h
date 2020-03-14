
#include "objgen.h"

class XmlOutData;
class XmlOut : virtual public ObjTravConst {
  public:
    XmlOut(bool indent = true);
    ~XmlOut();
    virtual void doObjBeg(ObjTravConst &ot, const ObjectBase &obj);
    virtual void doObjEnd(ObjTravConst &ot, const ObjectBase &obj);
    virtual void doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec);
    virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec);
    virtual void doMem(ObjTravConst &ot, const MemberBase &mem);

    std::string getString();
    void clear();
    void startList(std::string name);
  private:
    XmlOutData *data;

};

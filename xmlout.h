// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs
//
// MObs is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


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

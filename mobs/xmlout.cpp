// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs https://github.com/AlMarentu/MObs.git
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



#include "xmlout.h"
#include "xmlwriter.h"


#include <stack>


using namespace std;

namespace mobs {





bool XmlOut::doObjBeg(const ObjectBase &obj)
{
  if (obj.isNull() and cth.omitNull())
    return false;
  
  wstring name;
  if (not elements.empty())
    name = elements.top();
  if (name.empty())
    name = to_wstring(obj.getName(cth));
  if (name.empty())
    name = data->level() == 0 ? L"root": to_wstring(obj.getObjectName());

  data->writeTagBegin(name);

  if (obj.isNull())
  {
    data->writeTagEnd();
    return false;
  }
  
  elements.push(L"");
  return true;
}

void XmlOut::doObjEnd(const ObjectBase &obj)
{
  elements.pop();
  data->writeTagEnd();
}

bool XmlOut::doArrayBeg(const MemBaseVector &vec)
{
//  if (vec.isNull() and cth.omitNull())
//    return false;
  if (vec.isNull())
    return false;

  elements.push(to_wstring(vec.getName(cth)));
  return true;
}

void XmlOut::doArrayEnd(const MemBaseVector &vec)
{
  elements.pop();
}

void XmlOut::doMem(const MemberBase &mem)
{
  if (mem.isNull() and cth.omitNull())
    return;
  wstring name;
  if (not elements.empty())
    name = elements.top();
  if (name.empty())
    name = to_wstring(mem.getName(cth));

  if (mem.hasFeature(XmlAsAttr) and data->attributeAllowed())
  {
    if (not mem.isNull())
    {
      const wstring &value = to_wstring(mem.toStr(cth));
      data->writeAttribute(to_wstring(name), value);
    }
  }
  else
  {
    data->writeTagBegin(name);
    if (not mem.isNull())
    {
      const wstring &value = to_wstring(mem.toStr(cth));
      MobsMemberInfo mi;
      mem.memInfo(mi);
      if (mi.isBlob)
        data->writeCdata(value);
      else
        data->writeValue(value);
    }
    data->writeTagEnd();
  }
}


}

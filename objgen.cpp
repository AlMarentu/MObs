#include "objgen.h"

using namespace std;

void MemberBase::traverse(ObjTrav &trav)
{
  trav.doMem(trav, *this);
}

void MemberBase::traverse(ObjTravConst &trav) const
{
  trav.doMem(trav, *this);
}

void ObjectBase::regMem(MemberBase *mem)
{
  mlist.push_back(MlistInfo(mem, 0, 0));
}

void ObjectBase::regObj(ObjectBase *obj)
{
  mlist.push_back(MlistInfo(0, obj, 0));
}

void ObjectBase::regArray(MemBaseVector *vec)
{
  mlist.push_back(MlistInfo(0, 0, vec));
}

map<string, ObjectBase *(*)()> ObjectBase::createMap;

void ObjectBase::regObject(string n, ObjectBase *fun())
{
  createMap[n] = fun;
  cerr << "reg " << n << endl;
}

ObjectBase *ObjectBase::createObj(string n)
{
  auto it = createMap.find(n);
  if (it == createMap.end())
    return 0;
  return (*it).second();
}

MemberBase *ObjectBase::getMemInfo(const string &name)
{
  for (auto i:mlist)
  {
    if (i.mem and name == i.mem->name())
      return i.mem;
  }
  return 0;
}

ObjectBase *ObjectBase::getObjInfo(const string &name)
{
  for (auto i:mlist)
  {
    if (i.obj and name == i.obj->name())
      return i.obj;
  }
  return 0;
}

MemBaseVector *ObjectBase::getVecInfo(const string &name)
{
  for (auto i:mlist)
  {
    if (i.vec and name == i.vec->name())
      return i.vec;
  }
  return 0;
}

MemberBase &ObjectBase::get(string name)
{
  for (auto m:mlist)
  {
    if (m.mem && name == m.mem->name())
      return *m.mem;
  }
  throw runtime_error("ObjectBase::get: element not found");
}

void ObjectBase::traverse(ObjTravConst &trav) const
{
  trav.doObjBeg(trav, *this);
  if (not isNull())
  {
    for (auto const &m:mlist)
    {
      if (m.mem)
        m.mem->traverse(trav);
      if (m.vec)
        m.vec->traverse(trav);
      if (m.obj)
      {
        //cerr << "ooo " << m.obj->varName()  << endl;
        m.obj->traverse(trav);
      }
    }
  }
  trav.doObjEnd(trav, *this);
}

void ObjectBase::traverse(ObjTrav &trav)
{
  trav.doObjBeg(trav, *this);
  if (not isNull())
  {
    for (auto const &m:mlist)
    {
      if (m.mem)
        m.mem->traverse(trav);
      if (m.vec)
        m.vec->traverse(trav);
      if (m.obj)
      {
        //cerr << "ooo " << m.obj->varName()  << endl;
        m.obj->traverse(trav);
      }
    }
  }
  trav.doObjEnd(trav, *this);
}

void ObjectBase::doCopy(const ObjectBase &other)
{
  if (typName() != other.typName())
    throw runtime_error("ObjectBase::doCopy: invalid Type");
  auto src = other.mlist.begin();
  for (auto const &m:mlist)
  {
    if (src == other.mlist.end())
      throw runtime_error("ObjectBase::doCopy: invalid Element (num)");
    if (m.mem)
    {
      if (not src->mem)
        throw runtime_error("ObjectBase::doCopy: invalid Element (Member)");
      m.mem->fromStr(src->mem->toStr());
    }
    if (m.vec)
    {
      if (not src->vec)
        throw runtime_error("ObjectBase::doCopy: invalid Element (vector)");
      m.vec->doCopy(*src->vec);
    }
    if (m.obj)
    {
      if (not src->obj)
        throw runtime_error("ObjectBase::doCopy: invalid Element (Object)");
      m.obj->doCopy(*src->obj);
    }
    src++;
  }
  if (src != other.mlist.end())
      throw runtime_error("ObjectBase::doCopy: invalid Element (num2)");
}



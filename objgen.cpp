#include "objgen.h"
//#include <iostream>

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
  TRACE(PARAM(n));
  createMap[n] = fun;
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
  throw runtime_error(u8"ObjectBase::get: element not found");
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
    throw runtime_error(u8"ObjectBase::doCopy: invalid Type");
  auto src = other.mlist.begin();
  for (auto const &m:mlist)
  {
    if (src == other.mlist.end())
      throw runtime_error(u8"ObjectBase::doCopy: invalid Element (num)");
    if (m.mem)
    {
      if (not src->mem)
        throw runtime_error(u8"ObjectBase::doCopy: invalid Element (Member)");
      m.mem->fromStr(src->mem->toStr());
    }
    if (m.vec)
    {
      if (not src->vec)
        throw runtime_error(u8"ObjectBase::doCopy: invalid Element (vector)");
      m.vec->doCopy(*src->vec);
    }
    if (m.obj)
    {
      if (not src->obj)
        throw runtime_error(u8"ObjectBase::doCopy: invalid Element (Object)");
      m.obj->doCopy(*src->obj);
    }
    src++;
  }
  if (src != other.mlist.end())
      throw runtime_error(u8"ObjectBase::doCopy: invalid Element (num2)");
}


  
bool ObjectInserter::enter(const std::string &element) {
  TRACE(PARAM(element));
  path.push(element);
  
  if (objekte.empty())
    throw std::runtime_error(u8"XmlRead: Fatal: keine Objekt");
  
  if (memBase)  // War bereits im Member -> als Dummy-Objekt tarnen
    objekte.push(ObjectInserter::Objekt(nullptr, memName));
  
  memName = objekte.top().objName;
  memBase = 0;
  //    LOG(LM_DEBUG, "Sind im Object " << memName);
  if (objekte.top().obj)
  {
    ObjectBase *o = objekte.top().obj->getObjInfo(element);
    if (o)
    {
      //        LOG(LM_INFO, element << " ist ein Objekt");
      memName += "." + o->name();
      objekte.push(ObjectInserter::Objekt(o, memName));
      return true;
    }
    MemBaseVector *v = objekte.top().obj->getVecInfo(element);
    if (v)
    {
      size_t s = v->size();
      //        LOG(LM_INFO, element << " ist ein Vector " << s);
      memName += ".";
      memName += v->name();
      memName += "[";
      memName += std::to_string(s);
      memName += "]";
      v->resize(s+1);
      MemberBase *m = v->getMemInfo(s);
      ObjectBase *o = v->getObjInfo(s);
      if (o)
      {
        //          cerr << "Objekt" << endl;
        objekte.push(ObjectInserter::Objekt(o, memName));
        return true;
      }
      else if (m)
      {
        memName += m->name();
        memBase = m;
        //          LOG(LM_INFO, "Member: " << memName)
        return true;
      }
      objekte.push(ObjectInserter::Objekt(nullptr, memName));
      
      return false;
    }
    MemberBase *m = objekte.top().obj->getMemInfo(element);
    if (m)
    {
      //        cerr << element << " ist ein Member" << endl;
      memName += "." + m->name();
      memBase = m;
      //        LOG(LM_INFO, "Member: " << memName);
      return true;
    }
  }
  memName += ".";
  memName += element;
  objekte.push(ObjectInserter::Objekt(nullptr, memName));
  
  //    LOG(LM_INFO, memName << " ist ein WeisNichtWas");
  return false;
}

void ObjectInserter::exit(const std::string &element) {
  TRACE(PARAM(element));
  if (memBase)
    memBase = 0;
  else if (objekte.empty())
    throw std::runtime_error(u8"Objektstack underflow");
  else
    objekte.pop();
  if (not element.empty() and path.top() != element)
    throw std::runtime_error(u8"exit Object expected " + path.top() + " got " + element);
  path.pop();
}

void ObjectInserter::pushObject(ObjectBase &obj, const std::string &name) {
   objekte.push(ObjectInserter::Objekt(&obj, name));
 }

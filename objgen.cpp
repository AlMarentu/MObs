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

#include "objgen.h"
#include "jsonparser.h"
//#include <iostream>

using namespace std;

namespace mobs {

/////////////////////////////////////////////////
/// MemberBase
/////////////////////////////////////////////////

void MemberBase::traverse(ObjTrav &trav)
{
  trav.doMem(trav, *this);
}

void MemberBase::traverse(ObjTravConst &trav) const
{
  trav.doMem(trav, *this);
}

/////////////////////////////////////////////////
/// ObjectBase
/////////////////////////////////////////////////

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

/////////////////////////////////////////////////
/// ObjectBae::doCopy
/////////////////////////////////////////////////

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

/////////////////////////////////////////////////
/// Object Inserter
/////////////////////////////////////////////////

  
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

void ObjectInserter::leave(const std::string &element) {
  TRACE(PARAM(element));
  if (memBase)  // letzte Ebene war ein MemberVariable
    memBase = 0;
  else if (objekte.empty() or path.empty())
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



/////////////////////////////////////////////////
/// to_string
/////////////////////////////////////////////////



string to_string(const ObjectBase &obj) {

class ObjDump : virtual public ObjTravConst {
public:
  ObjDump() { inArray.push(false); };
  virtual void doObjBeg(ObjTravConst &ot, const ObjectBase &obj)
  {
    inArray.push(false);
    if (not fst)
      res << ",";
    fst = true;
    if (not obj.name().empty())
      res << obj.name() << ":";
    if (obj.isNull())
      res << "null";
    else
      res << "{";
  };
  virtual void doObjEnd(ObjTravConst &ot, const ObjectBase &obj)
  {
    if (not obj.isNull())
      res << "}";
    fst = false;
    inArray.pop();
  };
  virtual void doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec)
  {
    inArray.push(true);
    if (not fst)
      res << ",";
    fst = true;
    res << vec.name() << ":[";
  };
  virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec)
  {
    res << "]";
    fst = false;
    inArray.pop();
  };
  virtual void doMem(ObjTravConst &ot, const MemberBase &mem)
  {
    if (not fst)
      res << ",";
    fst = false;
    if (not inArray.empty() and not inArray.top())
      res << boolalpha << mem.name() << ":";
    if (mem.isNull())
      res << "null";
    else if (mem.is_chartype())
    {
      res << '"';
      string s = mem.toStr();
      if (s.length() != 1 or s[0] != 0)
      {
        size_t pos = 0;
        size_t pos2 = 0;
        while ((pos = s.find('"', pos2)) != string::npos)
        {
          res << s.substr(pos2, pos - pos2) << "\\\"";
          pos2 = pos+1;
        }
        res << s.substr(pos2);
      }
      res << '"';

    }
    else
      mem.strOut(res);

  };
  std::string result() const { return res.str(); };
private:
  bool fst = true;
  stringstream res;
  stack<bool> inArray;
};
  
  ObjDump od;
  
  obj.traverse(od);
  return od.result();
}



/////////////////////////////////////////////////
/// string2Obj
/////////////////////////////////////////////////

void string2Obj(const std::string &str, ObjectBase &obj)
{
  class JsonReadData : public ObjectInserter, public JsonParser  {
  public:
    JsonReadData(const string &input) : JsonParser(input) { }
    
    void Value(const string &val, bool charType) {
      TRACE(PARAM(val));
      if (enter(lastKey) and member())
      {
        if (not member())
        {
          LOG(LM_WARNING, "Variable fehlt " << showName());
        }
        else if (not charType and val == "null")
        {
          if (member()->nullAllowed())
            member()->setNull(true);
          else
          {
            LOG(LM_ERROR, u8"JSON: null not allowed  in variable " + member()->name());
            member()->clear();
          }
        }
        else if (not member()->fromStr(val))
          throw runtime_error(u8"JSON: invalid type in variable " + member()->name());
      }
      leave();
    }
    void StartObject() {
      TRACE(PARAM(lastKey));
      if (++level > 1)
        enter(lastKey);
    }
    void Key(const string &key) {
      lastKey = key;
    }
    void EndObject() {
      TRACE("");
      lastKey = current();
      if (level-- > 1)
        leave();
    }
    void StartArray() {
      TRACE("");
    }
    void EndArray() {
      TRACE("");
    }
    
  private:
    int level = 0;
    string lastKey;
    string prefix;
  };
  
  JsonReadData jd(str);
  jd.pushObject(obj);
  jd.parse();
}

/////////////////////////////////////////////////


}

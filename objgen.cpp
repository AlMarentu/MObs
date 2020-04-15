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
#include "xmlout.h"
#include <map>

using namespace std;

namespace mobs {


enum mobs::MemVarCfg mobsToken(MemVarCfg base, std::vector<std::string> &confToken, const std::string &s) {
  confToken.push_back(s);
  return MemVarCfg(confToken.size() + base -1);
}

/////////////////////////////////////////////////
/// MemberBase
/////////////////////////////////////////////////

void MemberBase::doConfig(MemVarCfg c)
{
  switch(c) {
    case Unset: break;
    case InitialNull: nullAllowed(true); break;
    case Key1 ... Key5: m_key = c - Key1 + 1; break;
    case AltNameBase ... AltNameBaseEnd: m_altName = c - AltNameBase; break;
    case VectorNull: break;
  }
}

void MemberBase::traverse(ObjTrav &trav)
{
  trav.doMem(trav, *this);
}

void MemberBase::traverse(ObjTravConst &trav) const
{
  trav.doMem(trav, *this);
}

void MemberBase::activate()
{
//  LOG(LM_INFO, "ACTIVATE MemberBase " << m_name);
  setNull(false);
  if (m_parVec)
    m_parVec->activate();
  else if (m_parent)
    m_parent->activate();
}



void MemBaseVector::activate()
{
//  LOG(LM_INFO, "ACTIVATE MemberBaseVector " << m_name);
  setNull(false);
  if (parent()) parent()->activate();
}

void MemBaseVector::doConfig(MemVarCfg c)
{
  switch(c) {
    case Unset: break;
    case VectorNull: nullAllowed(true); break;
    case Key1 ... Key5: break;
    case AltNameBase ... AltNameBaseEnd: m_altName = c - AltNameBase; break;
    case InitialNull: m_c.push_back(c); break;
  }
  
}

/////////////////////////////////////////////////
/// ObjectBase
/////////////////////////////////////////////////

void ObjectBase::activate()
{
//  LOG(LM_INFO, "ACTIVATE ObjectBase " << typName() << "::" << name());
  setNull(false);
  if (m_parVec)
    m_parVec->activate();
  else if (m_parent)
    m_parent->activate();
}

void ObjectBase::doConfig(MemVarCfg c)
{
  switch(c) {
    case Unset: break;
    case InitialNull: nullAllowed(true); break;
    case Key1 ... Key5: m_key = c - Key1 + 1; break;
    case AltNameBase ... AltNameBaseEnd: m_altName = c - AltNameBase; break;
    case VectorNull: break;
  }
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

size_t ObjectBase::findConfToken(const std::string &name)
{
  auto i = find(m_confToken.begin(), m_confToken.end(), name);
  if (i == m_confToken.end())
    return SIZE_T_MAX;
  return distance(m_confToken.begin(), i);
}

MemberBase *ObjectBase::getMemInfo(const size_t ctok)
{
  for (auto i:mlist)
  {
    if (i.mem and ctok == i.mem->cAltName())
      return i.mem;
  }
  return 0;
}

ObjectBase *ObjectBase::getObjInfo(const size_t ctok)
{
  for (auto i:mlist)
  {
    if (i.obj and ctok == i.obj->cAltName())
      return i.obj;
  }
  return 0;
}

MemBaseVector *ObjectBase::getVecInfo(const size_t ctok)
{
  for (auto i:mlist)
  {
    if (i.vec and ctok == i.vec->cAltName())
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

void ObjectBase::getKey(std::list<std::string> &key, const ConvToStrHint &cth) const
{
  // Element-Liste nach Key-Nummer sortieren
  multimap<int, const MlistInfo *> tmp;
  for (auto const &m:mlist)
  {
    if (m.mem and m.mem->key() > 0)
      tmp.insert(make_pair(m.mem->key(), &m));
    if (m.obj and m.obj->key() > 0)
      tmp.insert(make_pair(m.obj->key(), &m));
  }
  // Key-Elemente an Liste hängen
  for (auto const &i:tmp)
  {
    auto &m = *i.second;
    if (m.mem)
    {
      if (isNull() or m.mem->isNull())
        key.push_back("");
      else
        key.push_back(m.mem->toStr(cth));
    }
    if (m.obj)
    {
      list<string> keyTmp;
      m.obj->getKey(keyTmp, cth);
      if (isNull())
        for (auto i:keyTmp)
          key.push_back("");
      else
        key.splice(key.end(), keyTmp);
    }
  }
}

std::string ObjectBase::keyStr() const
{
  ConvToStrHint cth(false);
  list<string> key;
  getKey(key, cth);
  stringstream s;
  bool f = true;
  for (auto const &k:key)
  {
    // TODO Quoten
    if (f)
      f = false;
    else
      s << '.';
    s << k;
  }
  return s.str();
}

/////////////////////////////////////////////////
/// ObjectBae::doCopy
/////////////////////////////////////////////////
class ConvFromStrHintDoCopy : virtual public ConvFromStrHint {
public:
  ConvFromStrHintDoCopy() {}
  virtual ~ConvFromStrHintDoCopy() {}
  virtual bool acceptCompact() const { return true; }
  virtual bool acceptExtented() const { return false; }
};

void ObjectBase::doCopy(const ObjectBase &other)
{
  ConvFromStrHintDoCopy cfh;
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
      m.mem->fromStr(src->mem->toStr(ConvToStrHint(true)), cfh);
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
/// ObjectBae::clear
/////////////////////////////////////////////////

void ObjectBase::clear()
{
  for (auto const &m:mlist)
  {
    if (m.mem)
      m.mem->clear();
    if (m.vec)
      m.vec->clear();
    if (m.obj)
      m.obj->clear();
  }
  if (nullAllowed())
    setNull(true);
  else
    activate();
}



/////////////////////////////////////////////////
/// ObjectBase::get/set/Variable
/////////////////////////////////////////////////

bool ObjectBase::setVariable(const std::string &path, const std::string &value) {
  ObjectNavigator on;
  on.pushObject(*this);
  if (not on.find(path))
    return false;
  MemberBase *m = on.member();
  if (not m)
    return false;
  
  return m->fromStr(value, ConvFromStrHint::convFromStrHintDflt);
}

std::string ObjectBase::getVariable(const std::string &path, bool *found, bool compact) const {
  ObjectNavigator on;
  on.pushObject(*const_cast<ObjectBase *>(this));
  if (found)
    *found = false;
  
  if (not on.find(path))
    return std::string();
  MemberBase *m = on.member();
  if (not m)
    return std::string();
  
  if (found)
    *found = true;
  
  return m->toStr(ConvToStrHint(compact));
}

/////////////////////////////////////////////////
/// Object Inserter
/////////////////////////////////////////////////

bool ObjectNavigator::find(const std::string &path) {
  TRACE(PARAM(path));
  for (size_t pos = 0; pos < path.length();)
  {
    size_t pos2 = path.find_first_of(".[", pos);
    if (pos2 == std::string::npos)
    {
      enter(path.substr(pos));
      return true;
    }
    string element = path.substr(pos, pos2-pos);
    size_t index = SIZE_MAX;
    if (path[pos2] == '[')
    {
      pos = pos2 +1;
      pos2 = path.find(']', pos);
      if (pos2 == std::string::npos)
        break;
      string i = path.substr(pos, pos2-pos);
      if (not mobs::string2x(i, index))
        break;
//      std::cerr << "XX " << element << " " << index << " " << i << std::endl;
      pos2++;
    }
    enter(element, index);
    if (pos2 == path.length())
      return true;
    if (path[pos2] != '.')
      break;
    pos = pos2 +1;
  }
  return false;
}

bool ObjectNavigator::enter(const std::string &element, std::size_t index) {
  TRACE(PARAM(element) << PARAM(index))
  path.push(element);
  
  if (objekte.empty())
    throw std::runtime_error(u8"XmlRead: Fatal: keine Objekt");
  
  if (memBase)  // War bereits im Member -> als Dummy-Objekt tarnen
    objekte.push(ObjectNavigator::Objekt(nullptr, memName));
  
  memName = objekte.top().objName;
  memBase = 0;
  size_t altNamtok = SIZE_T_MAX;
  //    LOG(LM_DEBUG, "Sind im Object " << memName);
  if (objekte.top().obj)
  {
    if (cfs.acceptAltNames())
      altNamtok = objekte.top().obj->findConfToken(element);
//    LOG(LM_INFO, "ALT " << altNamtok << " " << boolalpha << cfs.acceptAltNames());
    MemBaseVector *v = nullptr;
    if (altNamtok != SIZE_T_MAX)
      v = objekte.top().obj->getVecInfo(altNamtok);
    if (v == nullptr)
    {
      v = objekte.top().obj->getVecInfo(element);
      if (v and not cfs.acceptOriNames() and v->cAltName() != SIZE_T_MAX)
        v = nullptr;
    }
    if (v)
    {
      size_t s = v->size();
      if (index != SIZE_MAX and index < s)
        s = index;
      else
      {
        if (index != SIZE_MAX)
          s = index;
        v->resize(s+1);
      }
//              LOG(LM_INFO, element << " ist ein Vector " << s);
      memName += ".";
      memName += v->name();
      memName += "[";
      memName += std::to_string(s);
      memName += "]";
      MemberBase *m = v->getMemInfo(s);
      ObjectBase *o = v->getObjInfo(s);
      if (o)
      {
        //          cerr << "Objekt" << endl;
        objekte.push(ObjectNavigator::Objekt(o, memName));
        return true;
      }
      else if (m)
      {
        memName += m->name();
        memBase = m;
        //          LOG(LM_INFO, "Member: " << memName)
        return true;
      }
      objekte.push(ObjectNavigator::Objekt(nullptr, memName));
      
      return false;
    }
    if (index == SIZE_MAX)
    {
      ObjectBase *o = nullptr;
      if (altNamtok != SIZE_T_MAX)
        o = objekte.top().obj->getObjInfo(altNamtok);
      if (o == nullptr)
      {
        o = objekte.top().obj->getObjInfo(element);
        if (o and not cfs.acceptOriNames() and o->cAltName() != SIZE_T_MAX)
          o = nullptr;
      }
      if (o)
      {
        //        LOG(LM_INFO, element << " ist ein Objekt");
        memName += "." + o->name();
        objekte.push(ObjectNavigator::Objekt(o, memName));
        return true;
      }
      MemberBase *m = nullptr;
      if (altNamtok != SIZE_T_MAX)
        m = objekte.top().obj->getMemInfo(altNamtok);
      if (m == nullptr)
      {
        m = objekte.top().obj->getMemInfo(element);
        if (m and not cfs.acceptOriNames() and m->cAltName() != SIZE_T_MAX)
          m = nullptr;
      }
      if (m)
      {
        //        cerr << element << " ist ein Member" << endl;
        memName += "." + m->name();
        memBase = m;
        //        LOG(LM_INFO, "Member: " << memName);
        return true;
      }
    }
  }
  memName += ".";
  memName += element;
  objekte.push(ObjectNavigator::Objekt(nullptr, memName));
  
  //    LOG(LM_INFO, memName << " ist ein WeisNichtWas");
  return false;
}

void ObjectNavigator::leave(const std::string &element) {
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

void ObjectNavigator::pushObject(ObjectBase &obj, const std::string &name) {
   objekte.push(ObjectNavigator::Objekt(&obj, name));
}


/////////////////////////////////////////////////
/// to_string
/////////////////////////////////////////////////

namespace  {
class ObjDump : virtual public ObjTravConst {
public:
  ObjDump(const ConvObjToString &c) : quoteKeys(c.withQuotes() ? "\"":""), cth(c) { inArray.push(false); };
  virtual void doObjBeg(ObjTravConst &ot, const ObjectBase &obj)
  {
    inArray.push(false);
    if (not fst)
      res << ",";
    fst = true;
    if (not obj.name().empty())
    {
      size_t n = obj.parent() and cth.useAltNames() ? obj.cAltName() : SIZE_T_MAX;
      res << quoteKeys << (n == SIZE_T_MAX ? obj.name() : obj.parent()->getConf(n)) << quoteKeys << ":";
    }
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
    size_t n = vec.parent() and cth.useAltNames() ? vec.cAltName() : SIZE_T_MAX;
    inArray.push(true);
    if (not fst)
      res << ",";
    fst = true;
    res << quoteKeys << (n == SIZE_T_MAX ? vec.name() : vec.parent()->getConf(n)) << quoteKeys << ":";
    if (vec.isNull())
      res << "null";
    else
      res << "[";
  };
  virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec)
  {
    if (not vec.isNull())
      res << "]";
    fst = false;
    inArray.pop();
  };
  virtual void doMem(ObjTravConst &ot, const MemberBase &mem)
  {
    size_t n = mem.parent() and cth.useAltNames() ? mem.cAltName() : SIZE_T_MAX;
    if (not fst)
      res << ",";
    fst = false;
    if (not inArray.empty() and not inArray.top())
      res << boolalpha << quoteKeys << (n == SIZE_T_MAX ? mem.name() : mem.parent()->getConf(n)) << quoteKeys << ":";
    if (mem.isNull())
      res << "null";
    else if (mem.is_chartype(cth))
    {
      res << '"';
      string s = mem.toStr(cth);
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
      res << mem.toStr(cth);

  };
  std::string result() const { return res.str(); };
private:
  std::string quoteKeys;
  bool fst = true;
  stringstream res;
  stack<bool> inArray;
  ConvObjToString cth;
};

}

std::string ObjectBase::to_string(ConvObjToString cth) const
{
  if (cth.toJson())
  {
    ObjDump od(cth);
    traverse(od);
    return od.result();
  }
  else if (cth.toXml())
  {
    XmlOut xd(cth);
    traverse(xd);
    return xd.getString();
  }
  else
    return "";
}





/////////////////////////////////////////////////
/// string2Obj
/////////////////////////////////////////////////

void string2Obj(const std::string &str, ObjectBase &obj, ConvObjFromStr cfh)
{
  class JsonReadData : public ObjectNavigator, public JsonParser  {
  public:
    JsonReadData(const string &input, const ConvObjFromStr &c) : JsonParser(input) { cfs = c; }
    
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
        else if (not member()->fromStr(val, cfs))
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
  
  JsonReadData jd(str, cfh);
  jd.pushObject(obj);
  jd.parse();
}

/////////////////////////////////////////////////


}

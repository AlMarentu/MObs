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
#include "xmlout.h"
#include "xmlwriter.h"
#include <map>

using namespace std;

namespace mobs {

/// \private
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
    case XmlAsAttr: m_XmlAsAttr = true; break;
    case VectorNull: break;
  }
}

void MemberBase::traverse(ObjTrav &trav)
{
  trav.doMem(*this);
}

void MemberBase::traverse(ObjTravConst &trav) const
{
  trav.doMem(*this);
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

std::string MemberBase::getName(const ConvToStrHint &cth) const {
  size_t n = m_parent and cth.useAltNames() ? m_altName : SIZE_T_MAX;
  return (n == SIZE_T_MAX) ? name() : m_parent->getConf(n);
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
    case XmlAsAttr: break;
    case VectorNull: nullAllowed(true); break;
    case Key1 ... Key5: break;
    case AltNameBase ... AltNameBaseEnd: m_altName = c - AltNameBase; break;
    case InitialNull: m_c.push_back(c); break;
  }
  
}

std::string MemBaseVector::getName(const ConvToStrHint &cth) const {
  size_t n = m_parent and cth.useAltNames() ? m_altName : SIZE_T_MAX;
  return (n == SIZE_T_MAX) ? name() : m_parent->getConf(n);
}


/////////////////////////////////////////////////
/// ObjectBase
/////////////////////////////////////////////////

std::string ObjectBase::getName(const ConvToStrHint &cth) const {
  size_t n = m_parent and cth.useAltNames() ? m_altName : SIZE_T_MAX;
  return (n == SIZE_T_MAX) ? name() : m_parent->getConf(n);
}


void ObjectBase::activate()
{
//  LOG(LM_INFO, "ACTIVATE ObjectBase " << typeName() << "::" << name());
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
    case XmlAsAttr: break;
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
  if (obj == nullptr) // used by MobsUnion
    mlist.clear();
  else
    mlist.push_back(MlistInfo(0, obj, 0));
}

void ObjectBase::regArray(MemBaseVector *vec)
{
  mlist.push_back(MlistInfo(0, 0, vec));
}

map<string, ObjectBase *(*)(ObjectBase *)> ObjectBase::createMap;

void ObjectBase::regObject(string n, ObjectBase *fun(ObjectBase *))
{
  TRACE(PARAM(n));
  createMap[n] = fun;
}

ObjectBase *ObjectBase::createObj(string n, ObjectBase *p)
{
  auto it = createMap.find(n);
  if (it == createMap.end())
    return 0;
  return (*it).second(p);
}

MemberBase *ObjectBase::getMemInfo(const std::string &name)
{
  for (auto i:mlist)
  {
    if (i.mem and name == i.mem->name())
      return i.mem;
  }
  return 0;
}

ObjectBase *ObjectBase::getObjInfo(const std::string &name)
{
  for (auto i:mlist)
  {
    if (i.obj and name == i.obj->name())
      return i.obj;
  }
  return 0;
}

MemBaseVector *ObjectBase::getVecInfo(const std::string &name)
{
  for (auto i:mlist)
  {
    if (i.vec and name == i.vec->name())
      return i.vec;
  }
  return 0;
}

size_t ObjectBase::findConfToken(const std::string &name) const
{
  auto i = find(m_confToken.begin(), m_confToken.end(), name);
  if (i == m_confToken.end())
    return SIZE_T_MAX;
  return distance(m_confToken.begin(), i);
}

MemberBase *ObjectBase::getMemInfo(size_t ctok) const
{
  for (auto i:mlist)
  {
    if (i.mem and ctok == i.mem->cAltName())
      return i.mem;
  }
  return 0;
}

ObjectBase *ObjectBase::getObjInfo(size_t ctok) const
{
//  if (ctok == SIZE_T_MAX) // MobsUnion-object
//  {
//    if (mlist.size() == 1)
//      return mlist.front().obj;
//  }
//  else
    for (auto i:mlist)
    {
      if (i.obj and ctok == i.obj->cAltName())
        return i.obj;
    }
  return 0;
}

MemBaseVector *ObjectBase::getVecInfo(size_t ctok) const
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
  bool inNull = trav.inNull;
  
  if (trav.doObjBeg(*this))
  {
    size_t arrayIndex = trav.arrayIndex;
    for (auto const &m:mlist)
    {
      trav.arrayIndex = SIZE_MAX;
      trav.inNull = inNull or isNull();
      if (m.mem)
        m.mem->traverse(trav);
      if (m.vec)
        m.vec->traverse(trav);
      if (m.obj)
        m.obj->traverse(trav);
    }
    trav.inNull = inNull;
    trav.arrayIndex = arrayIndex;
    trav.doObjEnd(*this);
  }
}

void ObjectBase::traverse(ObjTrav &trav)
{
  if (trav.doObjBeg(*this))
  {
    size_t arrayIndex = trav.arrayIndex;
    for (auto const &m:mlist)
    {
      trav.arrayIndex = SIZE_MAX;
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
    trav.arrayIndex = arrayIndex;
    trav.doObjEnd(*this);
  }
}

void ObjectBase::traverseKey(ObjTravConst &trav) const
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
  // Key-Elemente jetzt in richtiger Reihenfolge durchgehen
  bool inNull = trav.inNull;
  trav.keyMode = true;
  trav.doObjBeg(*this);
  for (auto const &i:tmp)
  {
    auto &m = *i.second;
    trav.inNull = inNull or isNull();
    if (m.mem)
      trav.doMem(*m.mem);
    if (m.obj)
      m.obj->traverseKey(trav);
  }
  trav.inNull = inNull;
  trav.doObjEnd(*this);
}

std::string ObjectBase::keyStr() const
{
  class KeyDump : virtual public mobs::ObjTravConst {
  public:
    KeyDump(const mobs::ConvToStrHint &c) : cth(c) { };
    virtual bool doObjBeg(const mobs::ObjectBase &obj) { return true; };
    virtual void doObjEnd(const mobs::ObjectBase &obj) { };
    virtual bool doArrayBeg(const mobs::MemBaseVector &vec) { return false; }
    virtual void doArrayEnd(const mobs::MemBaseVector &vec) { }
    virtual void doMem(const mobs::MemberBase &mem) {
      if (not fst)
        res << ",";
      fst = false;
      if (inNull or mem.isNull())
        ;
      else if (mem.is_chartype(cth))
        res << mobs::to_quote(mem.toStr(cth));
      else
        res << mem.toStr(cth);
    };
    std::string result() { return res.str(); };
  private:
    bool fst = true;
    stringstream res;
    mobs::ConvToStrHint cth;
  };


  ConvToStrHint cth(false);
  KeyDump kd(cth);
  traverseKey(kd);
  return kd.result();
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
  if (typeName() != other.typeName())
    throw runtime_error(u8"ObjectBase::doCopy: invalid Type");
  if (other.isNull())
  {
    forceNull();
    return;
  }
  auto src = other.mlist.begin();
  for (auto const &m:mlist)
  {
    if (src == other.mlist.end())
      throw runtime_error(u8"ObjectBase::doCopy: invalid Element (num)");
    if (m.mem)
    {
      if (not src->mem)
        throw runtime_error(u8"ObjectBase::doCopy: invalid Element (Member)");
      if (src->mem->isNull())
        m.mem->forceNull();
      else if (not m.mem->doCopy(src->mem))
        m.mem->fromStr(src->mem->toStr(ConvToStrHint(true)), cfh); // Fallback auf String umkopieren
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
/// ObjectBase::clear
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
  cleared(); // Callback
}



/////////////////////////////////////////////////
/// ObjectBase::get/set/Variable
/////////////////////////////////////////////////

bool ObjectBase::setVariable(const std::string &path, const std::string &value) {
  ObjectNavigator on;
  on.cfs = on.cfs.useDontShrink();
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
  on.cfs = on.cfs.useDontShrink();
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
  memName = "";
  for (size_t pos = 0; pos < path.length();)
  {
    size_t pos2 = path.find_first_of(".[", pos);
    if (pos2 == std::string::npos)
    {
      enter(path.substr(pos));
      return true;
    }
    string element = path.substr(pos, pos2-pos);
    size_t index = SIZE_T_MAX; // Vector selbst
    if (path[pos2] == '[')
    {
      pos = pos2 +1;
      pos2 = path.find(']', pos);
      if (pos2 == std::string::npos)
      {
        index = INT_MAX;  // hinten anhängen
        break;
      }
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

bool ObjectNavigator::setNull() {
  TRACE("");
  if (memVec)
  {
    switch (cfs.nullHandling()) {
      case ConvObjFromStr::omit: break;
      case ConvObjFromStr::force: memVec->forceNull(); break;
      case ConvObjFromStr::clear: memVec->clear(); break;
      case ConvObjFromStr::except:
        if (not memVec->nullAllowed())
          throw runtime_error(u8"ObjectNavigator: writing null to member " + showName() + " w/o nullAllowed");
        memVec->forceNull(); break;
      case ConvObjFromStr::ignore:
        if (not memVec->nullAllowed())
          return false;
        memVec->forceNull(); break;
    }
    return true;
  }
  if (member())
  {
    switch (cfs.nullHandling()) {
      case ConvObjFromStr::omit: break;
      case ConvObjFromStr::force: member()->forceNull(); break;
      case ConvObjFromStr::clear: member()->clear(); break;
      case ConvObjFromStr::except:
        if (not member()->nullAllowed())
          throw runtime_error(u8"ObjectNavigator: writing null to member " + showName() + " w/o nullAllowed");
        member()->forceNull(); break;
      case ConvObjFromStr::ignore:
        if (not member()->nullAllowed())
          return false;
        member()->forceNull(); break;
    }
    return true;
  }
  if (objekte.empty())
    return false;
  if (objekte.top().obj)
  {
    switch (cfs.nullHandling()) {
      case ConvObjFromStr::omit: break;
      case ConvObjFromStr::force: objekte.top().obj->forceNull(); break;
      case ConvObjFromStr::clear: objekte.top().obj->clear(); break;
      case ConvObjFromStr::except:
        if (not objekte.top().obj->nullAllowed())
          throw runtime_error(u8"ObjectNavigator: writing null to member " + showName() + " w/o nullAllowed");
        objekte.top().obj->forceNull(); break;
      case ConvObjFromStr::ignore:
        if (not objekte.top().obj->nullAllowed())
          return false;
        objekte.top().obj->forceNull(); break;
    }
    return true;
  }
  return false;
}
  
bool ObjectNavigator::enter(const std::string &element, std::size_t index) {
  TRACE(PARAM(element) << PARAM(index))
//  LOG(LM_INFO, "enter " << element << " " << index)
  path.push(element);
  
  if (objekte.empty())
    throw std::runtime_error(u8"ObjectNavigator: Fatal: no object");
  
  if (memBase)  // War bereits im Member -> als Dummy-Objekt tarnen
    objekte.push(ObjectNavigator::Objekt(nullptr, memName));
  
  memVec = nullptr;
  memName = objekte.top().objName;
  memBase = nullptr;
  size_t altNamtok = SIZE_T_MAX;
//  LOG(LM_DEBUG, "Im Object " << memName);
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
      if (index == SIZE_T_MAX)  // Wenn mit index gearbeitet wird, ist bei SIZE_MAX der Vector selbst gemeint
      {
        memVec = v;
      }
      if (index < INT_MAX and index < s)
      {
        s = index;
        if (cfs.shrinkArray())
          v->resize(s+1);
      }
      else if (index != SIZE_T_MAX)
      {
        if (index < INT_MAX)
          s = index;
        v->resize(s+1);
      }
//      LOG(LM_INFO, element << " ist ein Vector " << s);
      memName += ".";
      memName += v->name();
      memName += "[";
      if (index != SIZE_T_MAX)
        memName += std::to_string(s);
      memName += "]";
      if (index == SIZE_T_MAX)
        return true;
      MemberBase *m = v->getMemInfo(s);
      ObjectBase *o = v->getObjInfo(s);
      if (o)
      {
//        LOG(LM_INFO, "Obkekt")
        objekte.push(ObjectNavigator::Objekt(o, memName));
        return true;
      }
      else if (m)
      {
        memName += m->name();
        memBase = m;
        //        LOG(LM_INFO, "Member: " << memName)
        return true;
      }
      // Vector-element ist weder von MemberBase noch von ObjectBase abgeleitet
      throw runtime_error(u8"ObjectNavigator: structural corruption, vector without Elements in " + memName);
//      objekte.push(ObjectNavigator::Objekt(nullptr, memName));
//      return false;
    }
    if (index >= INT_MAX)
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
  if (cfs.exceptionIfUnknown())
    throw runtime_error(u8"ObjectNavigator: Element " + memName + " not found");
//  LOG(LM_DEBUG, u8"Element " + memName + " ist nicht vorhanden");
  return false;
}

void ObjectNavigator::leave(const std::string &element) {
  TRACE(PARAM(element));
//  LOG(LM_INFO, "leave " << element)
  if (memBase)  // letzte Ebene war ein MemberVariable
    memBase = nullptr;
  else if (objekte.empty() or path.empty())
    throw std::runtime_error(u8"ObjectNavigator: Objektstack underflow");
  else
    objekte.pop();
  if (not element.empty() and path.top() != element)
    throw std::runtime_error(u8"ObjectNavigator: exit Object expected " + path.top() + " got " + element);
  path.pop();
  memVec = nullptr;
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
  ObjDump(const ConvObjToString &c) : quoteKeys(c.withQuotes() ? "\"":""), cth(c) { };
  void newline() {
    if (needBreak and cth.withIndentation())
    {
      string s;
      s.resize(level * 2, ' ');
      res << '\n' << s;
    }
    needBreak = false;
  };
  virtual bool doObjBeg(const ObjectBase &obj)
  {
    if (obj.isNull() and cth.omitNull())
      return false;
    if (not fst)
      res << ",";
    newline();
    fst = true;
    if (not obj.name().empty())
      res << quoteKeys << obj.getName(cth) << quoteKeys << ":";
    if (obj.isNull())
    {
      res << "null";
      fst = false;
//      if (inArray())
      needBreak = true;
      return false;
    }
    res << "{";
    needBreak = true;
    level++;
    return true;
  };
  virtual void doObjEnd(const ObjectBase &obj)
  {
    if (obj.isNull() and cth.omitNull())
      return;
    level--;
    newline();
    res << "}";
    if (level == 0)
      needBreak = true;
    fst = false;
  };
  virtual bool doArrayBeg(const MemBaseVector &vec)
  {
    if (vec.isNull() and cth.omitNull())
      return false;
    if (not fst)
      res << ",";
    newline();
    fst = true;
    res << quoteKeys << vec.getName(cth) << quoteKeys << ":";
    needBreak = true;
    if (vec.isNull())
    {
      res << "null";
      fst = false;
      return false;
    }
    res << "[";
    return true;
  };
  virtual void doArrayEnd(const MemBaseVector &vec)
  {
    res << "]";
    fst = false;
    needBreak = true;
  };
  virtual void doMem(const MemberBase &mem)
  {
    if (mem.isNull() and cth.omitNull())
      return;
    if (not fst)
      res << ",";
    newline();
    fst = false;
    if (not inArray())
      res << boolalpha << quoteKeys << mem.getName(cth) << quoteKeys << ":";
    if (mem.isNull())
      res << "null";
    else if (mem.is_chartype(cth))
      res << mobs::to_quote(mem.toStr(cth));
    else
      res << mem.toStr(cth);
    needBreak = true;
  };
  std::string result() { newline(); return res.str(); };
private:
  std::string quoteKeys;
  bool fst = true;
  bool needBreak = false;
  int level = 0;
  stringstream res;
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
    XmlWriter wr(XmlWriter::CS_utf8, cth.withIndentation());
    XmlOut xd(&wr, cth);
    wr.writeHead();
    traverse(xd);
    return wr.getString();
  }
  else
    return "";
}



/////////////////////////////////////////////////


}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-branch-clone"
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
#include "converter.h"
#include <map>


using namespace std;

namespace mobs {

/// \private
enum mobs::MemVarCfg mobsToken(MemVarCfg base, std::vector<std::string> &confToken, const std::string &s) {
  confToken.push_back(s);
  return MemVarCfg(confToken.size() + base -1);
}

static mobs::MemVarCfg hatFeatureAllg(mobs::MemVarCfg c, const std::vector<mobs::MemVarCfg> &config)
{
  for (auto i:config) {
    if (i == c)
      return i;
    else if (c == AltNameBase and i >= AltNameBase and i <= AltNameEnd)
      return i;
    else if (c == ColNameBase and i >= ColNameBase and i <= ColNameEnd)
      return i;
    else if (c == PrefixBase and i >= PrefixBase and i <= PrefixEnd)
      return i;
  }
    return Unset;
}

/////////////////////////////////////////////////
/// MemberBase
/////////////////////////////////////////////////

void MemberBase::doConfig(MemVarCfg c)
{
  switch(c) {
    case DbCompact:
    case XmlAsAttr: m_config.push_back(c); break;
    case InitialNull: nullAllowed(true); break;
    case Key1 ... Key5: m_key = c - Key1 + 1; break;
    case AltNameBase ... AltNameEnd: m_altName = c; break;
    case Unset:
    case Embedded:
    case DbDetail:
    case ColNameBase ... ColNameEnd:
    case PrefixBase ... PrefixEnd:
    case VectorNull: break;
  }
}

MemVarCfg MemberBase::hasFeature(MemVarCfg c) const
{
  return hatFeatureAllg(c, m_config);
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

static std::string getNameAll(const mobs::ObjectBase *parent, const std::string &name,  MemVarCfg altName, const ConvToStrHint &cth)
{
  MemVarCfg n = parent and cth.useAltNames() ? altName : Unset;
  std::string tmp;
  if (parent and ((cth.usePrefix() and (n or not parent->hasFeature(Embedded))) or
                  (n and parent->hasFeature(Embedded)))) {
    MemVarCfg p = parent->hasFeature(PrefixBase);
    if (p and parent->parent())
      tmp = parent->parent()->getConf(p);
  }
  tmp += ((n == Unset) ? name : parent->getConf(n));
  if (cth.toLowercase()) {
    wstring tx = to_wstring(tmp);
    return to_string(mobs::toLower(tx));
  }
  return tmp;
}


std::string MemberBase::getName(const ConvToStrHint &cth) const {
  return getNameAll(m_parent, name(), m_altName, cth);
}


void MemBaseVector::activate()
{
//  LOG(LM_INFO, "ACTIVATE MemberBaseVector " << m_name);
  setNull(false);
  if (m_parent) m_parent->activate();
}

void MemBaseVector::doConfig(MemVarCfg c)
{
  switch(c) {
    case PrefixBase ... PrefixEnd:
    case ColNameBase ... ColNameEnd:
    case DbDetail: m_config.push_back(c); break; // für Vector selbst
    case DbCompact:
    case InitialNull: m_c.push_back(c); break; // für Member-Elemente
    case VectorNull: nullAllowed(true); break;
    case AltNameBase ... AltNameEnd: m_altName = c; break;
    case Unset:
    case Key1 ... Key5:
    case Embedded:
    case XmlAsAttr: break;
  }

}

MemVarCfg MemBaseVector::hasFeature(MemVarCfg c) const
{
  return hatFeatureAllg(c, m_config);
}

std::string MemBaseVector::getName(const ConvToStrHint &cth) const {
  return getNameAll(m_parent, name(), m_altName, cth);
}

/////////////////////////////////////////////////
/// ObjectBase
/////////////////////////////////////////////////

std::string ObjectBase::getName(const ConvToStrHint &cth) const {
  return getNameAll(m_parent, name(), m_altName, cth);
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

void ObjectBase::clearModified() {
  class ClearModified  : virtual public ObjTrav {
  public:
    bool doObjBeg(ObjectBase &obj) override { obj.setModified(false); return true; }
    void doObjEnd(ObjectBase &obj) override {  }
    bool doArrayBeg(MemBaseVector &vec) override { vec.setModified(false); return true; }
    void doArrayEnd(MemBaseVector &vec) override { }
    void doMem(MemberBase &mem) override { mem.setModified(false); }
  };
  ClearModified cm;
  traverse(cm);
}

void ObjectBase::doConfig(MemVarCfg c)
{
  switch(c) {
    case DbDetail:
    case PrefixBase ... PrefixEnd:
    case Embedded: m_config.push_back(c); break;
    case InitialNull: nullAllowed(true); break;
    case Key1 ... Key5: m_key = c - Key1 + 1; break;
    case AltNameBase ... AltNameEnd: m_altName = c; break;
    case Unset:
    case XmlAsAttr:
    case ColNameBase ... ColNameEnd:
    case DbCompact:
    case VectorNull: break;
  }
}

void ObjectBase::doConfigObj(MemVarCfg c)
{
  switch(c) {
    case Unset: break;
    case XmlAsAttr: break;
    case Embedded: break;
    case InitialNull: nullAllowed(true); break;
    case Key1 ... Key5: break;
    case AltNameBase ... AltNameEnd: break;
    case PrefixBase ... PrefixEnd: break;
    case ColNameBase ... ColNameEnd: m_config.push_back(c); break;
    case DbCompact:
    case DbDetail:
    case VectorNull: break;
  }
}

const std::string &ObjectBase::getConf(MemVarCfg c) const
{
  static const std::string x;
  size_t i;
  switch(c) {
    case AltNameBase ... AltNameEnd: i = c - AltNameBase; break;
    case ColNameBase ... ColNameEnd: i = c - ColNameBase; break;
    case PrefixBase ... PrefixEnd: i = c - PrefixBase; break;
    default: return x;
  }
  if (i < m_confToken.size())
    return m_confToken[i];
  return x;
}

MemVarCfg ObjectBase::hasFeature(MemVarCfg c) const
{
  return hatFeatureAllg(c, m_config);
}

void ObjectBase::regMem(MemberBase *mem)
{
  mlist.emplace_back(mem, nullptr, nullptr);
}

void ObjectBase::regObj(ObjectBase *obj)
{
  if (obj == nullptr) // used by MobsUnion
    mlist.clear();
  else if (obj->hasFeature(Embedded)) {
    // Objekt-Prefix auf Membernamen übertragen
    MemVarCfg pfx = obj->hasFeature(PrefixBase);
    if (pfx and obj->parent()) {
      string prefix = obj->parent()->getConf(pfx);

      for (auto &m:obj->mlist) {
        if (m.obj) m.obj->m_varNam.insert(0, prefix);
        if (m.vec) m.vec->m_name.insert(0, prefix);
        if (m.mem) m.mem->m_name.insert(0, prefix);
      }
    }
    mlist.splice(mlist.end(), obj->mlist);
  } else
    mlist.emplace_back(nullptr, obj, nullptr);
}

void ObjectBase::regArray(MemBaseVector *vec)
{
  mlist.emplace_back(nullptr, nullptr, vec);
}

extern map<string, ObjectBase *(*)(ObjectBase *)> *ObjectBase_Reg_createMap;
map<string, ObjectBase *(*)(ObjectBase *)> *ObjectBase_Reg_createMap = nullptr;

void ObjectBase::regObject(string n, ObjectBase *fun(ObjectBase *)) noexcept
{
  if (ObjectBase_Reg_createMap == nullptr)
    ObjectBase_Reg_createMap = new map<string, ObjectBase *(*)(ObjectBase *)>;

  ObjectBase_Reg_createMap->insert(make_pair(n, fun));
}

ObjectBase *ObjectBase::createObj(const string& n, ObjectBase *p)
{
  if (ObjectBase_Reg_createMap == nullptr)
    return nullptr;
  auto it = ObjectBase_Reg_createMap->find(n);
  if (it == ObjectBase_Reg_createMap->end())
    return nullptr;
  return (*it).second(p);
}

ObjectBase *ObjectBase::getObjInfo(const std::string &name, const ConvObjFromStr &cfh)
{
  for (auto i:mlist) {
    if (not i.obj) continue;
    if ((cfh.acceptOriNames() and name == i.obj->getName(ConvToStrHint(false, false, false, cfh.caseinsesitive()))) or
        (cfh.acceptAltNames() and name == i.obj->getName(ConvToStrHint(false, true, false, cfh.caseinsesitive()))))
      return i.obj;
  }
  return nullptr;
}

MemBaseVector *ObjectBase::getVecInfo(const std::string &name, const ConvObjFromStr &cfh)
{
  for (auto i:mlist) {
    if (not i.vec) continue;
    if ((cfh.acceptOriNames() and name == i.vec->getName(ConvToStrHint(false, false, false, cfh.caseinsesitive()))) or
        (cfh.acceptAltNames() and name == i.vec->getName(ConvToStrHint(false, true, false, cfh.caseinsesitive()))))
      return i.vec;
  }
  return nullptr;
}

MemberBase *ObjectBase::getMemInfo(const std::string &name, const ConvObjFromStr &cfh)
{
  for (auto i:mlist) {
    if (not i.mem) continue;
    if ((cfh.acceptOriNames() and name == i.mem->getName(ConvToStrHint(false, false, false, cfh.caseinsesitive()))) or
        (cfh.acceptAltNames() and name == i.mem->getName(ConvToStrHint(false, true, false, cfh.caseinsesitive()))))
      return i.mem;
  }
  return nullptr;
}



std::string ObjectBase::keyStr() const
{
  class KeyDump : virtual public mobs::ObjTravConst {
  public:
    explicit KeyDump(const mobs::ConvToStrHint &c) : cth(c) { };
    bool doObjBeg(const mobs::ObjectBase &obj) override { return true; };
    void doObjEnd(const mobs::ObjectBase &obj) override { };
    bool doArrayBeg(const mobs::MemBaseVector &vec) override { return false; }
    void doArrayEnd(const mobs::MemBaseVector &vec) override { }
    void doMem(const mobs::MemberBase &mem) override {
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
/// ObjectBase::doCopy
/////////////////////////////////////////////////
class ConvFromStrHintDoCopy : virtual public ConvFromStrHint {
public:
  ~ConvFromStrHintDoCopy() override = default;
  bool acceptCompact() const override { return true; }
  bool acceptExtented() const override { return false; }
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
/// traverse
/////////////////////////////////////////////////


void ObjectBase::traverse(ObjTravConst &trav) const
{
  bool wasParentMode = trav.parentMode;
  if (trav.parentMode) {
    if (m_parent)
      m_parent->traverseKey(trav);
    else if (m_parVec)
      m_parVec->traverseKey(trav);
    trav.parentMode = false;
  }

  bool inNull = trav.inNull;
  trav.keyMode = false;
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
  bool wasParentMode = trav.parentMode;
  if (trav.parentMode) {
    if (m_parent)
      m_parent->traverseKey(trav);
    else if (m_parVec)
      m_parVec->traverseKey(trav);
    trav.parentMode = false;
  }
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
  if (not wasParentMode and not trav.doObjBeg(*this))
    return;
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


void MemBaseVector::traverseKey(ObjTravConst &trav) const {
  if (trav.parentMode) {
    if (m_parent)
      m_parent->traverseKey(trav);
    trav.parentMode = false;
  }
}





/////////////////////////////////////////////////
/// ObjectBase::get/set/Variable
/////////////////////////////////////////////////

bool ObjectBase::setVariable(const std::string &path, const std::string &value) {
  ObjectNavigator on(ConvObjFromStr().useDontShrink());
  on.pushObject(*this);
  if (not on.find(path))
    return false;
  MemberBase *m = on.member();
  if (not m)
    return false;

  return m->fromStr(value, ConvFromStrHint::convFromStrHintDflt);
}

std::string ObjectBase::getVariable(const std::string &path, bool *found, bool compact) const {
  ObjectNavigator on(ConvObjFromStr().useDontShrink());
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

bool ObjectNavigator::find(const std::string &varName) {
  TRACE(PARAM(varName));
  memName = "";
  for (size_t pos = 0; pos < varName.length();)
  {
    size_t pos2 = varName.find_first_of(".[", pos);
    if (pos2 == std::string::npos)
    {
      enter(varName.substr(pos));
      return true;
    }
    string element = varName.substr(pos, pos2 - pos);
    size_t index = SIZE_T_MAX; // Vector selbst
    if (varName[pos2] == '[')
    {
      pos = pos2 +1;
      pos2 = varName.find(']', pos);
      if (pos2 == std::string::npos)
      {
        index = MemBaseVector::nextpos;  // hinten anhängen
        break;
      }
      string i = varName.substr(pos, pos2 - pos);
      if (not mobs::string2x(i, index))
        break;
//      std::cerr << "XX " << element << " " << index << " " << i << std::endl;
      pos2++;
    }
    enter(element, index);
    if (pos2 == varName.length())
      return true;
    if (varName[pos2] != '.')
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
  TRACE(PARAM(element) << PARAM(index));
//  LOG(LM_INFO, "enter " << element << " " << index)
  path.push(element);

  if (objekte.empty())
    throw std::runtime_error(u8"ObjectNavigator: Fatal: no object");

  if (memBase)  // War bereits im Member -> als Dummy-Objekt tarnen
    objekte.push(ObjectNavigator::Objekt(nullptr, memName));

  memVec = nullptr;
  memName = objekte.top().objName;
  memBase = nullptr;
//  LOG(LM_DEBUG, "Im Object " << memName);
  std::string elementFind = element;
  if (cfs.caseinsesitive())
    elementFind = mobs::toLower(element);
  if (objekte.top().obj)
  {
    mobs::MemBaseVector *v = objekte.top().obj->getVecInfo(elementFind, cfs);

    if (v)
    {
      size_t s = v->size();
      if (index == SIZE_T_MAX)  // Wenn mit index gearbeitet wird, ist bei SIZE_MAX der Vector selbst gemeint
      {
        memVec = v;
      }
      if (index < MemBaseVector::nextpos and index < s)
      {
        s = index;
        if (cfs.shrinkArray())
          v->resize(s+1);
      }
      else if (index != SIZE_T_MAX)
      {
        if (index < MemBaseVector::nextpos)
          s = index;
        v->resize(s+1);
      }
//      LOG(LM_INFO, ele << " ist ein Vector " << s);
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
    if (index >= MemBaseVector::nextpos) {
      mobs::ObjectBase *o = objekte.top().obj->getObjInfo(elementFind, cfs);
      if (o) {
//        LOG(LM_INFO, ele << " ist ein Objekt");
        memName += "." + o->name();
        objekte.push(ObjectNavigator::Objekt(o, memName));
        return true;
      }
      mobs::MemberBase *m = objekte.top().obj->getMemInfo(elementFind, cfs);
      if (m) {
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
  explicit ObjDump(const ConvObjToString &c) : quoteKeys(c.withQuotes() ? "\"":""), cth(c) { };
  void newline() {
    if (needBreak and cth.withIndentation())
    {
      string s;
      s.resize(level * 2, ' ');
      res << '\n' << s;
    }
    needBreak = false;
  };
  bool doObjBeg(const ObjectBase &obj) override
  {
    if (obj.isNull() and cth.omitNull())
      return false;
    if (not obj.isModified() and cth.modOnly())
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
  void doObjEnd(const ObjectBase &obj) override
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
  bool doArrayBeg(const MemBaseVector &vec) override
  {
    if (vec.isNull() and cth.omitNull())
      return false;
    if (not vec.isModified() and cth.modOnly())
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
  void doArrayEnd(const MemBaseVector &vec) override
  {
    res << "]";
    fst = false;
    needBreak = true;
  };
  void doMem(const MemberBase &mem) override
  {
    if (mem.isNull() and cth.omitNull())
      return;
    if (not mem.isModified() and cth.modOnly())
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

std::string ObjectBase::to_string(const ConvObjToString& cth) const
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


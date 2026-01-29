#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-branch-clone"
// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2025 Matthias Lautner
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
#include "csb.h"
#include "objcache.h"
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
    else if (c == LengthBase and i >= LengthBase and i <= LengthEnd)
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
    case XmlEncrypt:
    case LengthBase ... LengthEnd:
    case XmlAsAttr: m_config.push_back(c); break;
    case InitialNull: nullAllowed(true); m_null = true; break;
    case Key1 ... Key5: m_key = c - Key1 + 1; break;
    case DbVersionField: m_key = INT_MAX; break;
    case AltNameBase ... AltNameEnd: m_altName = c; break;
    case Unset:
    case Embedded:
    case DbDetail:
    case DbAuditTrail:
    case ColNameBase ... ColNameEnd:
    case PrefixBase ... PrefixEnd:
    case VectorNull:
    case DbJson:
    case OTypeAsXRoot:
      break;
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
  if (parent and parent->hasFeature(Embedded)) {
    auto p = parent;
    while (p and p->hasFeature(Embedded)) {
      MemVarCfg pf = p->hasFeature(PrefixBase);
      p = p->getParentObject();
      if (pf and p)
        tmp = p->getConf(pf) + tmp;
    }
  }
  else if (parent and cth.usePrefix()) {
    MemVarCfg pf = parent->hasFeature(PrefixBase);
    if (pf and parent->getParentObject())
      tmp += parent->getParentObject()->getConf(pf);
  }
  tmp += ((n == Unset) ? name : parent->getConf(n));
  if (cth.toLowercase()) {
    wstring tx = to_wstring(tmp);
    return to_string(mobs::toLower(tx));
  }
  return tmp;
}


std::string MemberBase::getName(const ConvToStrHint &cth) const {
  return getNameAll(m_parent, getElementName(), m_altName, cth);
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
    case LengthBase ... LengthEnd:
    case DbJson:
    case XmlEncrypt:
    case DbDetail: m_config.push_back(c); break; // für Vector selbst
    case DbCompact:
    case InitialNull: m_c.push_back(c); break; // für Member-Elemente
    case VectorNull: nullAllowed(true); break;
    case AltNameBase ... AltNameEnd: m_altName = c; break;
    case Unset:
    case Key1 ... Key5:
    case DbAuditTrail:
    case Embedded:
    case XmlAsAttr:
    case DbVersionField:
    case OTypeAsXRoot:
      break;
  }

}

MemVarCfg MemBaseVector::hasFeature(MemVarCfg c) const
{
  return hatFeatureAllg(c, m_config);
}

std::string MemBaseVector::getName(const ConvToStrHint &cth) const {
  return getNameAll(m_parent, getElementName(), m_altName, cth);
}

/////////////////////////////////////////////////
/// ObjectBase
/////////////////////////////////////////////////

std::string ObjectBase::getName(const ConvToStrHint &cth) const {
  return getNameAll(m_parent, getElementName(), m_altName, cth);
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

void ObjectBase::startAudit() {
  class ClearModified  : virtual public ObjTrav {
  public:
    bool doObjBeg(ObjectBase &obj) override { obj.setModified(false); return true; }
    void doObjEnd(ObjectBase &obj) override {  }
    bool doArrayBeg(MemBaseVector &vec) override {  return true; }
    void doArrayEnd(MemBaseVector &vec) override { vec.doStartAudit(); }
    void doMem(MemberBase &mem) override { mem.doStartAudit(); }
  };
  ClearModified cm;
  traverse(cm);
}

void ObjectBase::clearModified() {
  class ClearModified  : virtual public ObjTrav {
  public:
    bool doObjBeg(ObjectBase &obj) override { obj.setModified(false); return true; }
    void doObjEnd(ObjectBase &obj) override {  }
    bool doArrayBeg(MemBaseVector &vec) override {  return true; }
    void doArrayEnd(MemBaseVector &vec) override { vec.setModified(false); }
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
    case LengthBase ... LengthEnd:
    case DbJson:
    case XmlEncrypt:
    case OTypeAsXRoot:
    case Embedded: m_config.push_back(c); break;
    case InitialNull: nullAllowed(true); break;
    case Key1 ... Key5: m_key = c - Key1 + 1; break;
    case AltNameBase ... AltNameEnd: m_altName = c; break;
    case Unset:
    case XmlAsAttr:
    case ColNameBase ... ColNameEnd:
    case DbAuditTrail:
    case DbCompact:
    case VectorNull:
    case DbVersionField:
      break;
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
    case DbAuditTrail:
    case OTypeAsXRoot:
    case ColNameBase ... ColNameEnd: m_config.push_back(c); break;
    case LengthBase ... LengthEnd:
    case DbCompact:
    case DbDetail:
    case VectorNull:
    case DbVersionField:
    case DbJson:
    case XmlEncrypt:
      break;
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
  else
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
    if ((cfh.acceptOriNames() and name == i.obj->getName(ConvToStrHint(false, false, false, cfh.caseInsensitive()))) or
        (cfh.acceptAltNames() and name == i.obj->getName(ConvToStrHint(false, true, false, cfh.caseInsensitive()))))
      return i.obj;
  }
  // Dito für Embedded
  for (auto i:mlist) {
    if (not i.obj or not i.obj->hasFeature(Embedded)) continue;
    MemVarCfg pfx = i.obj->hasFeature(PrefixBase);
    size_t pfxLen = 0;
    if (pfx and getParentObject()) {
      string pfix = i.obj->getParentObject()->getConf(pfx);
      pfxLen = pfix.length();
      if (pfxLen and (name.length() <= pfxLen or name.substr(0, pfxLen) != pfix))
        continue;
    }
    if (auto res = i.obj->getObjInfo(name.substr(pfxLen), cfh))
      return res;
  }
  return nullptr;
}

MemBaseVector *ObjectBase::getVecInfo(const std::string &name, const ConvObjFromStr &cfh)
{
  for (auto i:mlist) {
    if (not i.vec) continue;
    if ((cfh.acceptOriNames() and name == i.vec->getName(ConvToStrHint(false, false, false, cfh.caseInsensitive()))) or
        (cfh.acceptAltNames() and name == i.vec->getName(ConvToStrHint(false, true, false, cfh.caseInsensitive()))))
      return i.vec;
  }
  // Dito für Embedded
  for (auto i:mlist) {
    if (not i.obj or not i.obj->hasFeature(Embedded)) continue;
    MemVarCfg pfx = i.obj->hasFeature(PrefixBase);
    size_t pfxLen = 0;
    if (pfx and getParentObject()) {
      string pfix = i.obj->getParentObject()->getConf(pfx);
      pfxLen = pfix.length();
      if (pfxLen and (name.length() <= pfxLen or name.substr(0, pfxLen) != pfix))
        continue;
    }
    if (auto res = i.obj->getVecInfo(name.substr(pfxLen), cfh))
      return res;
  }
  return nullptr;
}

MemberBase *ObjectBase::getMemInfo(const std::string &name, const ConvObjFromStr &cfh)
{
  for (auto i:mlist) {
    if (not i.mem) continue;
    if ((cfh.acceptOriNames() and name == i.mem->getName(ConvToStrHint(false, false, false, cfh.caseInsensitive()))) or
        (cfh.acceptAltNames() and name == i.mem->getName(ConvToStrHint(false, true, false, cfh.caseInsensitive()))))
      return i.mem;
  }
  // Dito für Embedded
  for (auto i:mlist) {
    if (not i.obj or not i.obj->hasFeature(Embedded)) continue;
    MemVarCfg pfx = i.obj->hasFeature(PrefixBase);
    size_t pfxLen = 0;
    if (pfx and getParentObject()) {
      string pfix = i.obj->getParentObject()->getConf(pfx);
      pfxLen = pfix.length();
      if (pfxLen and (name.length() <= pfxLen or name.substr(0, pfxLen) != pfix))
        continue;
    }
    if (auto res = i.obj->getMemInfo(name.substr(pfxLen), cfh))
      return res;
  }
  return nullptr;
}

static string escapeColon(const string &s) {
  string res;
  for (auto c:s) {
    if (c == ':')
      res += "\\:";
    else if (c == '\\')
      res += "\\\\";
    else
      res += c;
  }
  return res;
}

std::string ObjectBase::keyStr(int64_t *ver) const
{
  class KeyDump : virtual public mobs::ObjTravConst {
  public:
    explicit KeyDump(const mobs::ConvToStrHint &c) : cth(c) { withVersionField = true; };
    bool doObjBeg(const mobs::ObjectBase &obj) override { return true; };
    void doObjEnd(const mobs::ObjectBase &obj) override { };
    bool doArrayBeg(const mobs::MemBaseVector &vec) override { return false; }
    void doArrayEnd(const mobs::MemBaseVector &vec) override { }
    void doMem(const mobs::MemberBase &mem) override {
      if (mem.isVersionField()) {
        if (version < 0 ) {
          MobsMemberInfo mi;
          mem.memInfo(mi);
          if (mi.isUnsigned) {
            if (mi.u64 > mi.max)
              throw std::runtime_error("VersionElement overflow");
            version = mi.u64;
          } else if (mi.isSigned) {
            version = mi.i64;
          }
        }
        return;
      }
      if (not fst)
        res << ":";
      fst = false;
      if (inNull() or mem.isNull())
        ;
      else
        res << escapeColon(mem.auditValue());
    };
    std::string result() { return res.str(); };
    int64_t version = -1;
    bool fst = true;
    stringstream res;
    mobs::ConvToStrHint cth;
  };


  ConvToStrHint cth(false);
  KeyDump kd(cth);
  traverseKey(kd);
  if (ver)
    *ver = kd.version;
  if (kd.fst)
    throw runtime_error(STRSTR(getObjectName() << u8"::keyStr: KEYELEMENT missing"));
  return kd.result();
}

std::string ObjectBase::objNameKeyStr(int64_t *ver) const
{
  string result = escapeColon(getObjectName());
  result += ':';
  result += keyStr(ver);
  return result;
}

/////////////////////////////////////////////////
/// ObjectBase::doCopy
/////////////////////////////////////////////////
class ConvFromStrHintDoCopy : virtual public ConvFromStrHint {
public:
  ~ConvFromStrHintDoCopy() override = default;
  bool acceptCompact() const override { return true; }
  bool acceptExtended() const override { return false; }
};

void ObjectBase::doCopy(const ObjectBase &other)
{
  if (this == &other)
    return;
  ConvFromStrHintDoCopy cfh;
  if (getObjectName() != other.getObjectName())
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
      throw runtime_error(u8"ObjectBase::doCopy: invalid Element (source missing)");
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
    throw runtime_error(u8"ObjectBase::doCopy: invalid Element (target missing)");
}

void ObjectBase::carelessCopy(const ObjectBase &other) {
  if (this == &other)
    return;
  ConvFromStrHintDoCopy cfh;
  if (other.isNull()) {
    if (getObjectName() == other.getObjectName() and not isNull())
      forceNull();
    return;
  }
  for (auto const &src:other.mlist)
    for (auto const &m:mlist) {
      if (src.mem and m.mem and src.mem->m_name == m.mem->m_name) {
        if (src.mem->isNull()) {
          if (isModified() or not m.mem->isNull())
            m.mem->forceNull();
        } else if (not m.mem->compareAndCopy(src.mem)) {
          string tmp = src.mem->toStr(ConvToStrHint(true));
          if (isModified() or m.mem->toStr(ConvToStrHint(true)) != tmp)
            m.mem->fromStr(tmp, cfh); // Fallback auf String umkopieren
        }
        break;
      }
      if (m.vec and src.vec and src.vec->m_name == m.vec->m_name) {
        if (src.vec->isNull()) {
          if (not m.vec->isNull())
            m.vec->forceNull();
        } else
          m.vec->carelessCopy(*src.vec);
        break;
      }
      if (m.obj and src.obj and src.obj->m_varNam == m.obj->m_varNam) {
        // null handling in ObjectBase::carelessCopy
        m.obj->carelessCopy(*src.obj);
        break;
      }
    }
}





void MemberBase::doAudit() {
  if (m_saveOld) {
    m_saveOld = false;
    m_oldVal = auditValue();
    m_oldNull = isNull();
  }
}

std::string MemberBase::auditValue() const {
  if (isNull())
    return "";
  bool compact = hasFeature(mobs::DbCompact);
  return toStr(ConvToStrHint(compact));
}

void MemberBase::getInitialValue(std::string &old, bool &null) const {
  if (m_saveOld) {
    old = auditValue();
    null = isNull();
  } else {
    old = m_oldVal;
    null = m_oldNull;
  }
}




/////////////////////////////////////////////////
/// ObjectBase::clear
/////////////////////////////////////////////////

void ObjectBase::clear(bool null)
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
  if (null)
    setNull(true);
  else
    activate();
  cleared(); // Callback
}

/////////////////////////////////////////////////
/// ObjectBase::doConfigObj
/////////////////////////////////////////////////
void ObjectBase::doConfigObj(const vector <mobs::MemVarCfg> &cv)
{
  for (auto c:cv)
    doConfigObj(c);
}


/////////////////////////////////////////////////
/// traverse
/////////////////////////////////////////////////


void ObjectBase::traverse(ObjTravConst &trav) const
{
//  /// traversiere zusätzlich die Schlüsselelement vom Start über parent()
//  // in ObjTravConst: inbool parentMode = false;
//
//  bool wasParentMode = trav.parentMode;
//  if (trav.parentMode) {
//    if (m_parent)
//      m_parent->traverseKey(trav);
//    else if (m_parVec)
//      m_parVec->traverseKey(trav);
//    trav.parentMode = false;
//  }

  bool inNull = trav.m_inNull;
  trav.m_keyMode = false;
  bool embedded = hasFeature(Embedded);
  if (embedded or trav.doObjBeg(*this))
  {
    size_t arrayIndex = trav.m_arrayIndex;
    for (auto const &m:mlist)
    {
      trav.m_arrayIndex = SIZE_MAX;
      trav.m_inNull = inNull or isNull();
      if (m.mem)
        m.mem->traverse(trav);
      if (m.vec)
        m.vec->traverse(trav);
      if (m.obj)
        m.obj->traverse(trav);
    }
    trav.m_inNull = inNull;
    trav.m_arrayIndex = arrayIndex;
    if (not embedded)
      trav.doObjEnd(*this);
  }
}

void ObjectBase::traverse(ObjTrav &trav)
{
  bool embedded = hasFeature(Embedded);
  if (embedded or trav.doObjBeg(*this))
  {
    size_t arrayIndex = trav.m_arrayIndex;
    for (auto const &m:mlist)
    {
      trav.m_arrayIndex = SIZE_MAX;
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
    trav.m_arrayIndex = arrayIndex;
    if (not embedded)
      trav.doObjEnd(*this);
  }
}

void ObjectBase::traverseKey(ObjTravConst &trav) const
{
  trav.m_keyMode = true;
//  bool wasParentMode = trav.parentMode;
//  if (trav.parentMode) {
//    if (m_parent)
//      m_parent->traverseKey(trav);
//    else if (m_parVec)
//      m_parVec->traverseKey(trav);
//    trav.parentMode = false;
//  }

  // Element-Liste nach Key-Nummer sortieren
  multimap<int, const MlistInfo *> tmp;
  for (auto const &m:mlist)
  {
    if (m.mem and m.mem->keyElement() > 0)
      tmp.insert(make_pair(m.mem->keyElement(), &m));
    if (m.obj and m.obj->keyElement() > 0)
      tmp.insert(make_pair(m.obj->keyElement(), &m));
  }
  // Key-Elemente jetzt in richtiger Reihenfolge durchgehen
  bool inNull = trav.m_inNull;
  trav.m_keyMode = true;
//  if (not wasParentMode and not trav.doObjBeg(*this))
  bool embedded = hasFeature(Embedded);
  if (not embedded and not trav.doObjBeg(*this))
    return;
  for (auto const &i:tmp)
  {
    auto &m = *i.second;
    trav.m_inNull = inNull or isNull();
    if (m.mem and (trav.withVersionField or not m.mem->isVersionField()))
      trav.doMem(*m.mem);
    if (m.obj)
      m.obj->traverseKey(trav);
  }
  trav.m_inNull = inNull;
  if (not embedded)
    trav.doObjEnd(*this);
}

void ObjectBase::traverseKey(ObjTrav &trav)
{
  trav.m_keyMode = true;
  // Element-Liste nach Key-Nummer sortieren
  multimap<int, const MlistInfo *> tmp;
  for (auto const &m:mlist)
  {
    if (m.mem and m.mem->keyElement() > 0)
      tmp.insert(make_pair(m.mem->keyElement(), &m));
    if (m.obj and m.obj->keyElement() > 0)
      tmp.insert(make_pair(m.obj->keyElement(), &m));
  }
  // Key-Elemente jetzt in richtiger Reihenfolge durchgehen
  trav.m_keyMode = true;
//  if (not wasParentMode and not trav.doObjBeg(*this))
  bool embedded = hasFeature(Embedded);
  if (not embedded and not trav.doObjBeg(*this))
    return;
  for (auto const &i:tmp)
  {
    auto &m = *i.second;
    if (m.mem and (trav.withVersionField or not m.mem->isVersionField()))
      trav.doMem(*m.mem);
    if (m.obj)
      m.obj->traverseKey(trav);
  }
  if (not embedded)
    trav.doObjEnd(*this);
}

void ObjectBase::visit(ObjVisitor &visitor) {
  visitor.visit(*this);
}

void ObjectBase::visit(ObjVisitorConst &visitor) const {
  visitor.visit(*this);
}


void MemBaseVector::traverseKey(ObjTravConst &trav) const {
//  if (trav.parentMode) {
//    if (m_parent)
//      m_parent->traverseKey(trav);
//    trav.parentMode = false;
//  }
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

  return m->fromStr(value, ConvFromStrHintDefault());
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
      case ConvObjFromStr::clear:
        if (not memVec->nullAllowed())
          memVec->clear();
        else
          memVec->forceNull();
        break;
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
      case ConvObjFromStr::clear:
        if (not member()->nullAllowed())
          member()->clear();
        else
          member()->forceNull();
        break;
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
      case ConvObjFromStr::clear:
        if (not objekte.top().obj->nullAllowed())
          objekte.top().obj->clear();
        else
          objekte.top().obj->forceNull();
        break;
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
//  LOG(LM_INFO, "enter " << element << " " << index);
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
  if (cfs.caseInsensitive())
    elementFind = mobs::toLower(element);
  if (objekte.top().obj)
  {
    mobs::MemBaseVector *v = objekte.top().obj->getVecInfo(elementFind, cfs);

    if (v)
    {
      size_t s = v->size();
      if (index == SIZE_T_MAX)  // Wenn mit index gearbeitet wird, ist bei SIZE_T_MAX der Vector selbst gemeint
        memVec = v;
      else if (index < MemBaseVector::nextpos and index < s)
      {
        s = index;
        if (cfs.shrinkArray())
          v->resize(s+1);
      }
      else
      {
        if (index < MemBaseVector::nextpos)
          s = index;
        v->resize(s+1);
      }
//      LOG(LM_INFO, ele << " ist ein Vector " << s);
      memName += ".";
      memName += v->getElementName();
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
        memName += m->getElementName();
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
        memName += "." + o->getElementName();
        objekte.push(ObjectNavigator::Objekt(o, memName));
        return true;
      }
      mobs::MemberBase *m = objekte.top().obj->getMemInfo(elementFind, cfs);
      if (m) {
        memName += "." + m->getElementName();
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
//  LOG(LM_INFO, "leave " << element << " " << (path.empty() ? string("--") : path.top()));
  if (memVec)  // letzte Ebene war der Vector selbst (ohne [])
    memVec = nullptr;
  else if (memBase)  // letzte Ebene war ein MemberVariable
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
    if (not obj.getElementName().empty() and level > 0)
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
    if (level > 0) {
      res << quoteKeys << vec.getName(cth) << quoteKeys << ":";
      needBreak = true;
    }
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
      res << mobs::to_quoteJson(mem.toStr(cth));
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
    stringstream ss;
    mobs::CryptOstrBuf streambuf(ss);
    std::wostream xStrOut(&streambuf);
    XmlWriter wr(xStrOut, XmlWriter::CS_utf8, cth.withIndentation());
    XmlOut xd(&wr, cth);
    wr.writeHead();
    traverse(xd);
    streambuf.finalize();
    return ss.str();
  }
  else
    return "";
}

std::string MemBaseVector::to_string(const ConvObjToString& cth) const
{
  if (cth.toJson())
  {
    ObjDump od(cth);
    traverse(od);
    return od.result();
  }
  else if (cth.toXml())
  {
    stringstream ss;
    mobs::CryptOstrBuf streambuf(ss);
    std::wostream xStrOut(&streambuf);
    XmlWriter wr(xStrOut, XmlWriter::CS_utf8, cth.withIndentation());
    XmlOut xd(&wr, cth);
    wr.writeHead();
    traverse(xd);
    streambuf.finalize();
    return ss.str();
  }
  else
    return "";
}



/////////////////////////////////////////////////
// aus objCache weil escapeColon hier static
std::string ObjCache::escapeKey(const std::string &key) {
  return escapeColon(key);
}

}


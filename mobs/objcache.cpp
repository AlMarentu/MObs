// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2024 Matthias Lautner
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



#include "objcache.h"


namespace mobs {

class ObjCacheData {
public:
  class Info {
  public:
    explicit Info(std::shared_ptr<const ObjectBase> p) : ptr(std::move(p)), pos(++cnt) { }
    std::shared_ptr<const ObjectBase> ptr;
    u_int64_t pos;
  };
  void used(Info &info) {
    auto it = lru.find(info.pos);
    if (it == lru.end())
      THROW("cache is inconsistent");
    info.pos = ++cnt;
    lru.emplace(info.pos, it->second);
    lru.erase(it);
  }
  // um ein Element verkleinern
  void reduce() {
    auto it = lru.begin();
    if (it == lru.end())
      return;
    cache.erase(it->second);
    lru.erase(it);
  }
  std::map<std::string, Info> cache;
  std::map<u_int64_t, std::string> lru;
  static u_int64_t cnt;
};

u_int64_t ObjCacheData::cnt = 0;

void ObjCache::save(const ObjectBase &obj) {
  std::string key = obj.objNameKeyStr();
  auto p = std::shared_ptr<ObjectBase>(obj.createNew());
  p->doCopy(obj);
  auto it = data->cache.find(key);
  if (it == data->cache.end()) {
    it = data->cache.emplace(key, ObjCacheData::Info(std::move(p))).first;
    if (it == data->cache.end())
      THROW("can't save " << key);
    data->lru[it->second.pos] = key;
  } else {
    it->second.ptr = std::move(p);
    data->used(it->second);
  }
}

void ObjCache::save(std::shared_ptr<const ObjectBase> &op) {
  std::string key = op->objNameKeyStr();
  //auto p = std::shared_ptr<const ObjectBase>(op.release());
  auto it = data->cache.find(key);
  if (it == data->cache.end()) {
    it = data->cache.emplace(key, ObjCacheData::Info(op)).first;
    if (it == data->cache.end())
      THROW("can't save " << key);
    data->lru[it->second.pos] = key;
  } else {
    it->second.ptr = op;
    data->used(it->second);
  }
}

bool ObjCache::load(ObjectBase &obj) const{
  auto it = data->cache.find(obj.objNameKeyStr());
  if (it == data->cache.end())
    return false;
  data->used(it->second);
  obj.doCopy(*it->second.ptr);
  return true;
}

bool ObjCache::exists(const ObjectBase &obj) const {
  auto it = data->cache.find(obj.objNameKeyStr());
  if (it == data->cache.end())
    return false;
  return true;
}

std::shared_ptr<const ObjectBase> ObjCache::searchObj(const std::string &objIdent) const {
  auto it = data->cache.find(objIdent);
  if (it == data->cache.end())
    return {};
  data->used(it->second);
  return it->second.ptr;
}

size_t ObjCache::reduce(size_t n) {
  while (data->lru.size() > n)
    data->reduce();
  return data->lru.size();
}

ObjCache::ObjCache() {
  data = new ObjCacheData;
}

ObjCache::~ObjCache() {
  delete data;
}




}

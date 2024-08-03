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
#include "lrucache.h"


namespace mobs {

class ObjCacheData {
public:
  LRUCache<const ObjectBase> cache;
};


void ObjCache::save(const ObjectBase &obj) {
  std::string key = obj.objNameKeyStr();
  auto p = std::shared_ptr<ObjectBase>(obj.createNew());
  p->doCopy(obj);
  data->cache.insert(key, p);
}

void ObjCache::save(std::shared_ptr<const ObjectBase> &op) {
  std::string key = op->objNameKeyStr();
  //auto p = std::shared_ptr<const ObjectBase>(op.release());
  data->cache.insert(std::move(key), op);
}

bool ObjCache::load(ObjectBase &obj) const{
  auto o = data->cache.lookup(obj.objNameKeyStr());
  if (not o)
    return false;
  obj.doCopy(*o);
  return true;
}

bool ObjCache::exists(const ObjectBase &obj) const {
  return data->cache.exists(obj.objNameKeyStr());
}

std::shared_ptr<const ObjectBase> ObjCache::searchObj(const std::string &objIdent) const {
  return data->cache.lookup(objIdent);
}

size_t ObjCache::reduce(size_t n) {
  return data->cache.reduceCount(n);
}

ObjCache::ObjCache() {
  data = new ObjCacheData;
}

ObjCache::~ObjCache() {
  delete data;
}




}

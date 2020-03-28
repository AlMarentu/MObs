// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs
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



#ifndef MOBS_OBJPOOL_H
#define MOBS_OBJPOOL_H

#include <string>
#include <memory>

namespace mobs {


class NamedObject
{
  public:
    NamedObject() {};
    virtual std::string objName() const = 0;
    virtual ~NamedObject() {};
    bool nOdestroyed() const { return not valid; };
    void setNOdestroyed() {  valid = false; };
  private:
    bool valid = true;
};

class NOPData;
class NamedObjPool
{
public:
  NamedObjPool();
  ~NamedObjPool();
  void assign(std::string objName, std::shared_ptr<NamedObject> obj);
  bool lookup(std::string objName, std::weak_ptr<NamedObject> &ptr);
  void garbageCollect();
protected:
  NOPData *data;
  

};

template <class T>
class NamedObjRef
{
  public:
    NamedObjRef(std::shared_ptr<NamedObjPool> nOPool, std::string objName) : pool(nOPool), name(objName) {
      pool->lookup(name, ptr);
    };
    ~NamedObjRef() {};
      T *operator=(T *t) {
      auto tmp = std::shared_ptr<T>(t);
      pool->assign(name, tmp);
      ptr = tmp;
      return t;
    };
    std::shared_ptr<T> create() {
      auto tmp = std::make_shared<T>();
      pool->assign(name, tmp);
      return tmp;
    };
    std::shared_ptr<T> lock() {
      if ((ptr.expired() or ptr.lock()->nOdestroyed()) and not pool->lookup(name, ptr))
        return nullptr;
      return std::dynamic_pointer_cast<T>(ptr.lock());
    };
    T &operator*() {
      auto t = lock();
      if (t == nullptr)
        throw std::runtime_error(std::string("NamedObject ") + name + " nullptr access");
      return *t;
    };
    T *operator->() {
      auto t = lock();
      if (t == nullptr)
        throw std::runtime_error(std::string("NamedObject ") + name + " nullptr access");
      T *p = t.get();  

      return p;
    };
  protected:
    std::shared_ptr<NamedObjPool> pool;
    std::string name;
    std::weak_ptr<NamedObject> ptr;

};

}

#endif

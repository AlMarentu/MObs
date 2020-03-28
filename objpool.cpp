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


#include "objpool.h"
#include "logging.h"

#include <iostream>
#include <unordered_map>
#include <list>

using namespace std;

namespace mobs {

namespace {  
  class NOD {
    public:
      NOD(string objName, shared_ptr<NamedObject> obj) /* : name(objName)*/ {
        ptr = obj;
      };
      //string name;
      shared_ptr<NamedObject> ptr;
      bool deleteLater = false;
  };
}


class NOPData {
  public:
    NOPData() { TRACE(""); };
    ~NOPData() { TRACE(""); garbageCollect(); };
    unordered_map<string, NOD> pool;
    void assign(string objName, shared_ptr<NamedObject> obj);
    bool lookup(string objName, weak_ptr<NamedObject> &ptr);
    void garbageCollect();

};

void NOPData::assign(string objName, shared_ptr<NamedObject> obj)
{
  TRACE(PARAM(objName));
  auto search = pool.find(objName);
  if (search != pool.end())
  {
    search->second.ptr->setNOdestroyed();

    if (not obj)
    {
      pool.erase(search);
    }
    else
    {
      cerr << "Element " << objName << " ersetzt " << search->second.ptr.use_count() << endl;
      search->second.ptr = obj;
    }
  }
  else if ( obj )
  {
    auto result = pool.emplace(make_pair(objName, NOD(objName, obj)));
    if (not result.second)
      throw runtime_error(string("Element ") + objName + " insert error");
    cerr << "Element " << objName << " inserted" << endl;
  }
}

bool NOPData::lookup(string objName, weak_ptr<NamedObject> &ptr)
{
  TRACE(PARAM(objName));
  auto result = pool.find(objName);
  if (result == pool.end())
    return false;
  ptr =  result->second.ptr;
  return true;
}

NamedObjPool::NamedObjPool()
{
  TRACE("");
  data = new NOPData();
}

NamedObjPool::~NamedObjPool()
{
  TRACE("");
  delete data;
  data = 0;
}

void NamedObjPool::assign(string objName, shared_ptr<NamedObject> obj)
{
  TRACE(PARAM(objName));
  return data->assign(objName, obj);
}

bool NamedObjPool::lookup(string objName, weak_ptr<NamedObject> &ptr)
{
  TRACE(PARAM(objName));
  return data->lookup(objName, ptr);
}

void NOPData::garbageCollect()
{
  list<unordered_map<string, NOD>::iterator> remove_it;

  for(auto p = pool.begin(); p != pool.end(); p++)
  {
    cerr << "Element " << p->first << " use " << p->second.ptr.use_count() << endl;
    if (p->second.ptr.use_count() <= 1)
    {
      p->second.ptr->setNOdestroyed();
      p->second.ptr.reset();
      remove_it.push_back(p);
    }
  }
  for (auto r:remove_it)
    pool.erase(r);
}

void NamedObjPool::garbageCollect()
{
  TRACE("");
  data->garbageCollect();
}

}


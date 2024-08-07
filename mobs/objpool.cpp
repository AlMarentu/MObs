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


#include "objpool.h"
#include "logging.h"

#include <iostream>
#include <unordered_map>
#include <map>
#include <list>
#include <utility>
#include <climits>

using namespace std;

namespace mobs {

namespace {  
  class NOD {
    public:
      NOD(const string &/*objName*/, shared_ptr<NamedObject> obj) /* : name(objName)*/ {
        ptr = std::move(obj);
      };
      //string name;
      shared_ptr<NamedObject> ptr;
//      bool deleteLater = false;
  };
}

class NOPData {
public:
//  NOPData() { TRACE(""); };
  virtual ~NOPData() = default;
  virtual void assign(const string &objName, shared_ptr<NamedObject> obj) = 0;
  virtual bool lookup(const string &objName, weak_ptr<NamedObject> &ptr) = 0;
  virtual void search(const string &searchName, std::list<pair<std::string, std::weak_ptr<NamedObject>>> &result) = 0;
  virtual void clearUnlocked() = 0;

};
///////////////////////////////////////////
// NOPDataMap
//////////////////////////////////////////

class NOPDataMap : virtual public NOPData {
  public:
    NOPDataMap() { TRACE(""); };
    ~NOPDataMap() override { TRACE(""); NOPDataMap::clearUnlocked(); }
    map<string, NOD> pool;
    void assign(const string &objName, shared_ptr<NamedObject> obj) override;
    bool lookup(const string &objName, weak_ptr<NamedObject> &ptr) override;
    void search(const string &searchName, std::list<pair<std::string, std::weak_ptr<NamedObject>>> &result) override;
    void clearUnlocked() override;

};

void NOPDataMap::assign(const string &objName, shared_ptr<NamedObject> obj)
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
      LOG(LM_DEBUG, "Element " << objName << " ersetzt " << search->second.ptr.use_count());
      search->second.ptr = obj;
    }
  }
  else if ( obj )
  {
    auto result = pool.emplace(objName, NOD(objName, obj));
    if (not result.second)
      throw runtime_error(string("Element ") + objName + " insert error");
    LOG(LM_DEBUG, "Element " << objName << " inserted");
  }
}

bool NOPDataMap::lookup(const string &objName, weak_ptr<NamedObject> &ptr)
{
  TRACE(PARAM(objName));
  auto result = pool.find(objName);
  if (result == pool.end())
    return false;
  ptr =  result->second.ptr;
  return true;
}

void NOPDataMap::search(const string &searchName, std::list<pair<std::string, std::weak_ptr<NamedObject>>> &result)
{
  TRACE(PARAM(searchName));
  auto e = pool.upper_bound(searchName + "\177");
  for (auto i = pool.lower_bound(searchName); i != e; i++)
  {
    result.emplace(result.end(), i->first, i->second.ptr);
  }
}

void NOPDataMap::clearUnlocked()
{
  list<map<string, NOD>::iterator> remove_it;

  for(auto p = pool.begin(); p != pool.end(); p++)
  {
//    LOG(LM_DEBUG, "Element " << p->first << " use " << p->second.ptr.use_count());
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

///////////////////////////////////////////
// NOPDataUnordered
//////////////////////////////////////////


#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedStructInspection"
class NOPDataUnordered : virtual public NOPData {
public:
  NOPDataUnordered() { TRACE(""); };
  ~NOPDataUnordered() override { TRACE(""); NOPDataUnordered::clearUnlocked(); };
  unordered_map<string, NOD> pool;
//    map<string, NOD> pool;
  void assign(const string &objName, shared_ptr<NamedObject> obj) override;
  bool lookup(const string &objName, weak_ptr<NamedObject> &ptr) override;
  void search(const string &searchName, std::list<pair<std::string, std::weak_ptr<NamedObject>>> &result) override;
  void clearUnlocked() override;
};
#pragma clang diagnostic pop

void NOPDataUnordered::assign(const string &objName, shared_ptr<NamedObject> obj)
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
      LOG(LM_DEBUG, "Element " << objName << " ersetzt " << search->second.ptr.use_count());
      search->second.ptr = obj;
    }
  }
  else if ( obj )
  {
    auto result = pool.emplace(objName, NOD(objName, obj));
    if (not result.second)
      throw runtime_error(string("Element ") + objName + " insert error");
    LOG(LM_DEBUG, "Element " << objName << " inserted");
  }
}

bool NOPDataUnordered::lookup(const string &objName, weak_ptr<NamedObject> &ptr)
{
  TRACE(PARAM(objName));
  auto result = pool.find(objName);
  if (result == pool.end())
    return false;
  ptr =  result->second.ptr;
  return true;
}

void NOPDataUnordered::search(const string &searchName, std::list<pair<std::string, std::weak_ptr<NamedObject>>> &result)
{
  TRACE(PARAM(searchName));
  throw runtime_error(u8"function search not implemented");
}


void NOPDataUnordered::clearUnlocked()
{
  list<unordered_map<string, NOD>::iterator> remove_it;

  for(auto p = pool.begin(); p != pool.end(); p++)
  {
    LOG(LM_DEBUG, "Element " << p->first << " use " << p->second.ptr.use_count());
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

///////////////////////////////////////////
// NameObjPool
//////////////////////////////////////////


NamedObjPool::NamedObjPool()
{
  TRACE("");
//  data = new NOPDataUnordered();
  data = std::unique_ptr<NOPDataMap>(new NOPDataMap);
}

NamedObjPool::~NamedObjPool()
{
  TRACE("");
}

void NamedObjPool::assign(const string& objName, shared_ptr<NamedObject> obj)
{
  TRACE(PARAM(objName));
  data->assign(objName, std::move(obj));
}

bool NamedObjPool::lookup(const string &objName, weak_ptr<NamedObject> &ptr)
{
  TRACE(PARAM(objName));
  return data->lookup(objName, ptr);
}

void NamedObjPool::search(const string &searchName, std::list<pair<std::string, std::weak_ptr<NamedObject>>> &result)
{
  result.clear();
  data->search(searchName, result);
}

void NamedObjPool::clearUnlocked()
{
  TRACE("");
  data->clearUnlocked();
}

}


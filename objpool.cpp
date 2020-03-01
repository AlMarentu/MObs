
#include "objpool.h"

#include <iostream>
#include <unordered_map>
#include <list>

using namespace std;

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
    NOPData() { cerr << "NOPData" << endl;};
    ~NOPData() { garbageCollect(); cerr << "~NOPData" << endl;};
    unordered_map<string, NOD> pool;
    void assign(string objName, shared_ptr<NamedObject> obj);
    bool lookup(string objName, weak_ptr<NamedObject> &ptr);
    void garbageCollect();

};

void NOPData::assign(string objName, shared_ptr<NamedObject> obj)
{
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
  auto result = pool.find(objName);
  if (result == pool.end())
    return false;
  ptr =  result->second.ptr;
  return true;
}

NamedObjPool::NamedObjPool()
{
  data = new NOPData();
}

NamedObjPool::~NamedObjPool()
{
  delete data;
  data = 0;
}

void NamedObjPool::assign(string objName, shared_ptr<NamedObject> obj)
{
  return data->assign(objName, obj);
}

bool NamedObjPool::lookup(string objName, weak_ptr<NamedObject> &ptr)
{
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
  data->garbageCollect();
}


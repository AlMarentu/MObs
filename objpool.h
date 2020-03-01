#ifndef OBJPOOL_H
#define OBJPOOL_H

#include <string>
#include <memory>


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


#endif

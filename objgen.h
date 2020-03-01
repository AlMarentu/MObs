#ifndef OBJGEN_H
#define OBJGEN_H

#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <sstream>



#define MemVar(typ, name) Member<typ> name = Member<typ>(#name, this)
#define MemVector(typ, name) MemberVector<Member<typ> > name = MemberVector<Member<typ> >(#name, this)
#define ObjVector(typ, name) MemberVector<typ> name = MemberVector<typ>(#name, this)
#define ObjVar(typ, name) typ name = typ(#name, this)
#define ObjInit(objname) \
  objname() { init(); std::cerr << "OOO()" << std::endl; }; \
  objname(std::string name, ObjectBase *t) { if (t) t->regObj(this); m_varNam = name; std::cerr << "OOOO " << name << " " << this << std::endl; }; \
  objname &operator=(const objname &other) { std::cerr << "OOO(rhs)" << std::endl; \
    if (this != &other) { \
      doCopy(other); \
    } \
    return *this; \
  }; \
  static ObjectBase *createMe() { return new objname; } ; \
  virtual std::string typName() const { return #objname; };

#define ObjRegister(name) \
namespace  { \
class RegMe##name { \
public: \
  RegMe##name() { ObjectBase::regObject(#name, name::createMe); }; \
  static RegMe##name regme; \
}; \
RegMe##name RegMe##name::regme; \
} 


class ObjTrav;

class NullValue {
  public:
    bool isNull() const { return m_null and m_nullAllowed; };
    void setNull(bool n) { m_null = n; } ;
    void nullAllowed(bool on) { m_nullAllowed = on; };
    bool nullAllowed() const { return m_nullAllowed; };
  private:
    bool m_null = false;
    bool m_nullAllowed = false;
};

class MemberBase : public NullValue {
public:
  MemberBase(std::string n) : m_name(n) {};
  virtual ~MemberBase() {};
  std::string name() const { return m_name; };
  virtual void strOut(std::ostream &str) const  = 0;
  virtual std::string toStr() const  = 0;
  virtual void fromStr(const std::string &s) = 0;
  void traverse(ObjTrav &trav);

protected:
  std::string m_name;
};

class ObjectBase;

class MemBaseVector : public NullValue {
  public:
    MemBaseVector(std::string n) : m_name(n) { std::cerr << "VVVV()" << std::endl;};
    virtual ~MemBaseVector() { std::cerr << "~VVVV()" << std::endl;};
    virtual void traverse(ObjTrav &trav) = 0;
    virtual size_t size() const = 0;
    virtual void resize(size_t s) = 0;
    std::string name() const { return m_name; };
    virtual void doCopy(const MemBaseVector &other) = 0;
    virtual MemberBase *getMemInfo(size_t i) = 0;
    virtual ObjectBase *getObjInfo(size_t i) = 0;
  protected:
    std::string m_name;
};

class ObjectBase : public NullValue {
  public:
  ObjectBase() {};
  virtual ~ObjectBase() {};
  void regMem(MemberBase *mem);
  void regObj(ObjectBase *obj);
  void regArray(MemBaseVector *vec);
  //void regMemList(std::list<MemberBase> *m, std::string n);
  void traverse(ObjTrav &trav);
  virtual std::string typName() const = 0;
  virtual void init() {};
  std::string name() const { return m_varNam; };
  MemberBase &get(std::string name);
  MemberBase *getMemInfo(const std::string &name);
  ObjectBase *getObjInfo(const std::string &name);
  MemBaseVector *getVecInfo(const std::string &name);
  
  static void regObject(std::string n, ObjectBase *fun());
  static ObjectBase *createObj(std::string n);
  void doCopy(const ObjectBase &other);

  protected:
    std::string m_varNam;
  private:
    class MlistInfo {
      public:
        MlistInfo(MemberBase *m, ObjectBase *o, MemBaseVector *v) : mem(m), obj(o), vec(v) {};
        MemberBase *mem = 0;
        ObjectBase *obj = 0;
        MemBaseVector *vec = 0;
    };
    std::list<MlistInfo> mlist;
    static std::map<std::string, ObjectBase *(*)()> createMap;

};

template<typename T>
class Member : virtual public MemberBase {
public:
  Member() : MemberBase("") {  std::cerr << "MMMM()" << std::endl;};  // Konstriktor für Array
  Member(std::string n, ObjectBase *o) : MemberBase(n) { if (o) o->regMem(this); std::cerr << "MMMM " << n << " " << this << std::endl;}; // Konst. f. Objekt
  Member &operator=(const Member &other) = delete;
  ~Member() { std::cerr << "DDDD " << m_name << std::endl;};
  T operator() () const { return wert; };
  void operator() (const T &t) { wert = t; std::cerr << "= " << this << std::endl;};
  virtual void strOut(std::ostream &str) const { str << wert; };
  virtual std::string toStr() const { std::stringstream s; s << wert; return s.str(); };
  virtual void fromStr(const std::string &sin) { std::stringstream s; s.str(sin); T t; s >> t; operator()(t);  };
  void doCopy(const Member<T> &other) { operator()(other()); };
private:
  T wert;
};

template<class T>
class MemberVector : virtual public MemBaseVector {
  public:
    MemberVector(std::string n, ObjectBase *o) : MemBaseVector(n) { o->regArray(this); std::cerr << "MVMV " << n << " " << this << std::endl;};
    ~MemberVector() { resize(0); std::cerr << "DDDD " << m_name << std::endl;};   // heap Aufräumen
    T &operator[] (size_t t) { if (t >= size()) resize(t+1); return *werte[t]; };
    virtual size_t size() const { return werte.size(); };
    virtual void resize(size_t s);
    virtual void traverse(ObjTrav &trav);
    virtual MemberBase *getMemInfo(size_t i) { if (i >= size()) return 0; return dynamic_cast<MemberBase *>(werte[i]); };
    virtual ObjectBase *getObjInfo(size_t i) { if (i >= size()) return 0; return dynamic_cast<ObjectBase *>(werte[i]); };
  protected:
    void doCopy(const MemberVector<T> &other);
    void doCopy(const  MemBaseVector &other);
  private:
    // Vector von Heap-Elementen verwenden, da sonst Probleme beim Reorg
    std::vector<T *> werte;
};

class ObjTrav {
  public:
    virtual void doObjBeg(ObjTrav &ot, ObjectBase &obj) = 0;
    virtual void doObjEnd(ObjTrav &ot, ObjectBase &obj) = 0;
    virtual void doArrayBeg(ObjTrav &ot, MemBaseVector &vec) = 0;
    virtual void doArrayEnd(ObjTrav &ot, MemBaseVector &vec) = 0;
    virtual void doMem(ObjTrav &ot, MemberBase &mem) = 0;
};

template<class T>
void MemberVector<T>::resize(size_t s)
{
  std::cerr << "RESI " << s << " {" << std::endl;
  size_t old = size();
  if (old > s)
  {
    for (size_t i = s; i < old; i++)
      delete werte[i];
    werte.resize(s);
  }
  else if (old < s)
  {
    werte.resize(s);
    for (size_t i = old; i < s; i++)
      werte[i] = new T;
  }
  std::cerr << "} RESI" << std::endl;
}

template<class T>
void MemberVector<T>::traverse(ObjTrav &trav)
{
  trav.doArrayBeg(trav, *this);
  for (auto w:werte)
    w->traverse(trav);
  trav.doArrayEnd(trav, *this);
};

template<class T>
void MemberVector<T>::doCopy(const MemberVector<T> &other)
{
  resize(other.size());
  size_t i = 0;
  for (auto const w:other.werte)
    operator[](i++).doCopy(*w);
};

template<class T>
void MemberVector<T>::doCopy(const MemBaseVector &other)
{
  const MemberVector<T> *t = dynamic_cast<const MemberVector<T> *>(&other);
  if (not t)
    throw std::runtime_error("MemberVector::doCopy invalid");
  doCopy(*t);
};

#endif

#ifndef OBJGEN_H
#define OBJGEN_H

#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <sstream>


using namespace std;

#define MemVar(typ, name) Member<typ> name = Member<typ>(#name, this)
#define MemVector(typ, name) MemberVector<Member<typ> > name = MemberVector<Member<typ> >(#name, this)
#define ObjVector(typ, name) MemberVector<typ> name = MemberVector<typ>(#name, this)
#define ObjVar(typ, name) typ name = typ(#name, this)
#define ObjInit(objname) \
  objname() { init(); cerr << "OOO()" << endl; }; \
  objname(string name, ObjectBase *t) { if (t) t->regObj(this); m_varNam = name; cerr << "OOOO " << name << " " << this << endl; }; \
  objname &operator=(const objname &other) { cerr << "OOO(rhs)" << endl; \
    if (this != &other) { \
      doCopy(other); \
    } \
    return *this; \
  }; \
  static ObjectBase *createMe() { return new objname; } ; \
  virtual string typName() const { return #objname; };

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
  MemberBase(string n) : m_name(n) {};
  virtual ~MemberBase() {};
  string name() const { return m_name; };
  virtual void strOut(ostream &str) const  = 0;
  virtual string toStr() const  = 0;
  virtual void fromStr(const string &s) = 0;
  void traverse(ObjTrav &trav);

protected:
  string m_name;
};

class ObjectBase;

class MemBaseVector : public NullValue {
  public:
    MemBaseVector(string n) : m_name(n) { cerr << "VVVV()" << endl;};
    virtual ~MemBaseVector() { cerr << "~VVVV()" << endl;};
    virtual void traverse(ObjTrav &trav) = 0;
    virtual size_t size() const = 0;
    virtual void resize(size_t s) = 0;
    string name() const { return m_name; };
    virtual void doCopy(const MemBaseVector &other) = 0;
    virtual MemberBase *getMemInfo(size_t i) = 0;
    virtual ObjectBase *getObjInfo(size_t i) = 0;
  protected:
    string m_name;
};

class ObjectBase : public NullValue {
  public:
  ObjectBase() {};
  virtual ~ObjectBase() {};
  void regMem(MemberBase *mem);
  void regObj(ObjectBase *obj);
  void regArray(MemBaseVector *vec);
  //void regMemList(list<MemberBase> *m, string n);
  void traverse(ObjTrav &trav);
  virtual string typName() const = 0;
  virtual void init() {};
  string name() const { return m_varNam; };
  MemberBase &get(string name);
  MemberBase *getMemInfo(const string &name);
  ObjectBase *getObjInfo(const string &name);
  MemBaseVector *getVecInfo(const string &name);
  
  static void regObject(string n, ObjectBase *fun());
  static ObjectBase *createObj(string n);
  void doCopy(const ObjectBase &other);

  protected:
    string m_varNam;
  private:
    class MlistInfo {
      public:
        MlistInfo(MemberBase *m, ObjectBase *o, MemBaseVector *v) : mem(m), obj(o), vec(v) {};
        MemberBase *mem = 0;
        ObjectBase *obj = 0;
        MemBaseVector *vec = 0;
    };
    list<MlistInfo> mlist;
    static map<string, ObjectBase *(*)()> createMap;

};

template<typename T>
class Member : virtual public MemberBase {
public:
  Member() : MemberBase("") {  cerr << "MMMM()" << endl;};  // Konstriktor für Array
  Member(string n, ObjectBase *o) : MemberBase(n) { if (o) o->regMem(this); cerr << "MMMM " << n << " " << this << endl;}; // Konst. f. Objekt
  Member &operator=(const Member &other) = delete;
  ~Member() { cerr << "DDDD " << m_name << endl;};
  T operator() () const { return wert; };
  void operator() (const T &t) { wert = t; cerr << "= " << this << endl;};
  virtual void strOut(ostream &str) const { str << wert; };
  virtual string toStr() const { stringstream s; s << wert; return s.str(); };
  virtual void fromStr(const string &sin) { stringstream s; s.str(sin); T t; s >> t; operator()(t);  };
  void doCopy(const Member<T> &other) { operator()(other()); };
private:
  T wert;
};

template<class T>
class MemberVector : virtual public MemBaseVector {
  public:
    MemberVector(string n, ObjectBase *o) : MemBaseVector(n) { o->regArray(this); cerr << "MVMV " << n << " " << this << endl;};
    ~MemberVector() { resize(0); cerr << "DDDD " << m_name << endl;};   // heap Aufräumen
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
    vector<T *> werte;
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
  cerr << "RESI " << s << " {" << endl;
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
  cerr << "} RESI" << endl;
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
    throw runtime_error("MemberVector::doCopy invalid");
  doCopy(*t);
};

#endif

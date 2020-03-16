#include "xmlread.h"
#include "xmlparser.h"

#include<stack>
#include<limits.h>
#include<iostream>


using namespace std;

class ObjectInserter  {
public:
  ObjectInserter() { }
  ~ObjectInserter() { }
  
  inline MemberBase *member() const { return memBase; };
  string showpath() const {
    string p;
    for (auto &i:path)
      p += i + "::";
    return p;
  }
  void pushObject(ObjectBase &obj) {
    objekte.push(ObjectInserter::Objekt(&obj, ""));
  }
  bool enter(const string &element) {
    TRACE(PARAM(element));
    memBase = 0;
    path.push_back(element);
    
    if (objekte.empty())
      throw runtime_error(u8"XmlRead: Fatal: keine Objekt");
    
    cerr << "Sind im Object " << objekte.top().objName << endl;
    ObjectBase *o = objekte.top().obj->getObjInfo(element);
    if (o)
    {
      cerr << element << " ist ein Objekt" << endl;
      objekte.push(ObjectInserter::Objekt(o, o->name()));
      return true;
    }
    MemBaseVector *v = objekte.top().obj->getVecInfo(element);
    if (v)
    {
      size_t s = v->size();
      cerr << element << " ist ein Vector " << s << endl;
      v->resize(s+1);
      MemberBase *m = v->getMemInfo(s);
      ObjectBase *o = v->getObjInfo(s);
      if (o)
      {
        //          cerr << "Objekt" << endl;
        objekte.push(ObjectInserter::Objekt(o, element));
        return true;
      }
      else if (m)
      {
        //          cerr << "Member" << endl;
        memBase = m;
        return true;
      }
      objekte.push(ObjectInserter::Objekt(nullptr, element));
      
      return false;
    }
    MemberBase *m = objekte.top().obj->getMemInfo(element);
    if (m)
    {
      //        cerr << element << " ist ein Member" << endl;
      memBase = m;
      return true;
    }
    cerr << element << " ist ein WeisNichtWas" << endl;
    return false;
  }
  void exit(const string &element = "") {
    TRACE(PARAM(element));
    if (memBase)
      memBase = 0;
    else if (objekte.empty())
      throw runtime_error(u8"Objektstack underflow");
    else
      objekte.pop();
    if (not element.empty() and path[path.size() -1] != element)
      throw runtime_error(u8"exit Object expected " + path[path.size() -1] + " got " + element);
    path.resize(path.size()-1);
  }
  
  
  
private:
  class Objekt {
  public:
    Objekt(ObjectBase *o, const string &name) {
      obj = o;
      objName = name;
      cerr << "OBJ " << name << endl;
    };
    ~Objekt() {};
    
    string objName;
    ObjectBase *obj = 0;
  };
  stack<Objekt> objekte;
  vector<string> path;
  

  MemberBase *memBase = 0;
};




class XmlReadData : public ObjectInserter, public XmlParser  {
public:
  XmlReadData(const string &input) : XmlParser(input) {  };
  ~XmlReadData() { };
  
// stack<bool> inArray;
  
  string encoding;
    
  void NullTag(const string &element) {
    TRACE(PARAM(element));
    cerr << "NULL " << element << endl;

    if (member() and member()->nullAllowed())
    {
      member()->setNull(true);
    }
    EndTag(element);
  };
  void Attribut(const string &element, const string &attribut, const string &value) {
    TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
    cerr << "Attribut " << element << " " << attribut << " " << value << endl;

  };
  void Value(const string &value) {
    TRACE(PARAM(value));
    cerr << "Value " << value << endl;
    if (not member())
      cerr << "Variable fehlt " << showpath() << endl;
    else
      member()->fromStr(value);
  };
  void Cdata(const char *val, size_t len) {
    string value(val, len);
    TRACE(PARAM(value));
    cerr << "Cdata " << value << " atadC" << endl;
    if (not member())
      cerr << "Variable fehlt " << showpath() << endl;
    else
      member()->fromStr(value);
  };
  void StartTag(const string &element) {
    TRACE(PARAM(element));
    cerr << "Start " << element << " " << tagPath().size() <<  endl;

    if (tagPath().size() <= 1)
      return;
    if (not enter(element))
      cerr << element << " wurde nicht gefunden" << endl;


  }
  void EndTag(const string &element) {
    TRACE(PARAM(element));
    cerr << "Ende " << element << endl;
    if (tagPath().size() > 1)
      exit(element);
  }
  void ProcessingInstruction(const string &element, const string &attribut, const string &value) {
    TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
    cerr << "ProcInst " << element << " " << attribut << " " << value << endl;
    if (element == "xml" and attribut == "encoding")
      encoding = value;
  }

  
  
  private:
    string prefix;;
  };
  
  
  
  void XmlRead::parse()
  {
    TRACE("");
    data->parse();
  }
  
  XmlRead::XmlRead(const string &input)
  {
    TRACE("");
    data = new XmlReadData(input);
  }
  
  XmlRead::~XmlRead()
  {
    TRACE("");
    delete data;
  }
  
  void XmlRead::fill(ObjectBase &obj)
  {
    TRACE("");
    data->pushObject(obj);
    try {
      parse();
    } catch (exception &e) {
      cerr << "Exception " << e.what() << endl;
      size_t pos;
      const string &xml = data->info(pos);
      cerr << xml.substr(pos) << endl;
      throw runtime_error(string("Parsing failed ") + " at pos. " + to_string(pos));
    }
  }
  
  

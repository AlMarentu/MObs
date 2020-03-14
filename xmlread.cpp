#include "xmlread.h"
#include "xmlparser.h"

#include<stack>
#include<limits.h>
#include<iostream>


using namespace std;




class XmlReadData : public XmlParser  {
public:
  XmlReadData(const string &input) : XmlParser(input) {  };
  ~XmlReadData() { };
  
// stack<bool> inArray;
  
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
  string encoding;
  stack<Objekt> objekte;
  vector<string> path;
  
    
  void NullTag(const string &element) {
    TRACE(PARAM(element));
    cerr << "NULL " << element << endl;

    if (memBase and memBase->nullAllowed())
    {
      memBase->setNull(true);
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
    string p;
    for (auto &i:path)
      p += i + "::";
    if (not memBase)
      cerr << "Variable fehlt " << p << endl;
    else
      memBase->fromStr(value);
  };
  void Cdata(const char *val, size_t len) {
    string value(val, len);
    TRACE(PARAM(value));
    cerr << "Cdata " << value << " atadC" << endl;

  };
  void StartTag(const string &element) {
    TRACE(PARAM(element));
    cerr << "Start " << element << " " << tagPath().size() <<  endl;
    memBase = 0;
    path.push_back(element);
    string p;
    for (auto &i:path)
      p += i + "::";
    //cerr << "XX " << p << endl;
    
    if (objekte.empty())
      throw runtime_error(u8"XmlRead: Fatal: keine Objekt");
    if (tagPath().size() <= 1)
      return;
    
    cerr << "Sind im Object " << objekte.top().objName << endl;
    ObjectBase *o = objekte.top().obj->getObjInfo(element);
    if (o)
    {
      cerr << element << " ist ein Objekt" << endl;
      objekte.push(XmlReadData::Objekt(o, o->name()));
      return;
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
        objekte.push(XmlReadData::Objekt(o, element));
      }
      else if (m)
      {
        //          cerr << "Member" << endl;
        memBase = m;
      }
      else
        cerr << element << " ist ein Vector von WeisNichtWas" << endl;
      
      return;
    }
    MemberBase *m = objekte.top().obj->getMemInfo(element);
    if (m)
    {
      //        cerr << element << " ist ein Member" << endl;
      memBase = m;
      return;
    }
    cerr << element << " ist ein WeisNichtWas" << endl;
    
    

  }
  void EndTag(const string &element) {
    TRACE(PARAM(element));
    cerr << "Ende " << element << endl;
    path.resize(path.size()-1);
    if (memBase)
    {
      memBase = 0;
      return;
    }
    if (not objekte.empty())
      objekte.pop();
    else if (tagPath().size() > 1)
      throw runtime_error(u8"Objektstack underflow");
  }
  void ProcessingInstruction(const string &element, const string &attribut, const string &value) {
    TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
    cerr << "ProcInst " << element << " " << attribut << " " << value << endl;
    if (element == "xml" and attribut == "encoding")
      encoding = value;
  }

  
  
  private:
    string prefix;;
  MemberBase *memBase = 0;
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
    data->objekte.push(XmlReadData::Objekt(&obj, ""));
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
  
  

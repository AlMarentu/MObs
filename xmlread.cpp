#include "xmlread.h"
#include "xmlparser.h"
#include "logging.h"


#include<stack>

#if 0
class ObjectXXXInserter  {
public:
  
  inline MemberBase *member() const { return memBase; };
  inline const std::string &showName() const { return memName; };
  void pushObject(ObjectBase &obj, const std::string &name = "<obj>") {
    objekte.push(ObjectInserter::Objekt(&obj, name));
  }
  bool enter(const std::string &element) {
    TRACE(PARAM(element));
    path.push(element);
    
    if (objekte.empty())
      throw std::runtime_error(u8"XmlRead: Fatal: keine Objekt");
    
    if (memBase)  // War bereits im Member -> als Dummy-Objekt tarnen
      objekte.push(ObjectInserter::Objekt(nullptr, memName));
    
    memName = objekte.top().objName;
    memBase = 0;
//    LOG(LM_DEBUG, "Sind im Object " << memName);
    if (objekte.top().obj)
    {
      ObjectBase *o = objekte.top().obj->getObjInfo(element);
      if (o)
      {
//        LOG(LM_INFO, element << " ist ein Objekt");
        memName += "." + o->name();
        objekte.push(ObjectInserter::Objekt(o, memName));
        return true;
      }
      MemBaseVector *v = objekte.top().obj->getVecInfo(element);
      if (v)
      {
        size_t s = v->size();
//        LOG(LM_INFO, element << " ist ein Vector " << s);
        memName += ".";
        memName += v->name();
        memName += "[";
        memName += std::to_string(s);
        memName += "]";
        v->resize(s+1);
        MemberBase *m = v->getMemInfo(s);
        ObjectBase *o = v->getObjInfo(s);
        if (o)
        {
          //          cerr << "Objekt" << endl;
          objekte.push(ObjectInserter::Objekt(o, memName));
          return true;
        }
        else if (m)
        {
          memName += m->name();
          memBase = m;
//          LOG(LM_INFO, "Member: " << memName)
          return true;
        }
        objekte.push(ObjectInserter::Objekt(nullptr, memName));
        
        return false;
      }
      MemberBase *m = objekte.top().obj->getMemInfo(element);
      if (m)
      {
        //        cerr << element << " ist ein Member" << endl;
        memName += "." + m->name();
        memBase = m;
//        LOG(LM_INFO, "Member: " << memName);
        return true;
      }
    }
    memName += ".";
    memName += element;
    objekte.push(ObjectInserter::Objekt(nullptr, memName));

//    LOG(LM_INFO, memName << " ist ein WeisNichtWas");
    return false;
  }
  void exit(const std::string &element = "") {
    TRACE(PARAM(element));
    if (memBase)
      memBase = 0;
    else if (objekte.empty())
      throw std::runtime_error(u8"Objektstack underflow");
    else
      objekte.pop();
    if (not element.empty() and path.top() != element)
      throw std::runtime_error(u8"exit Object expected " + path.top() + " got " + element);
    path.pop();
  }
  
  
  
private:
  class Objekt {
  public:
    Objekt(ObjectBase *o, const std::string &name) {
      obj = o;
      objName = name;
    };
    ~Objekt() {};
    
    std::string objName;
    ObjectBase *obj = 0;
  };
  std::stack<Objekt> objekte;
  std::stack<std::string> path;
  
  std::string memName;
  MemberBase *memBase = 0;
};
#endif



class XmlReadData : public ObjectInserter, public XmlParser  {
public:
  XmlReadData(const std::string &input) : XmlParser(input) {  };
  ~XmlReadData() { };
  
// stack<bool> inArray;
  
  std::string encoding;
    
  void NullTag(const std::string &element) {
    TRACE(PARAM(element));
    LOG(LM_INFO, "NULL " << element);

    if (member() and member()->nullAllowed())
    {
      member()->setNull(true);
    }
    EndTag(element);
  };
  void Attribut(const std::string &element, const std::string &attribut, const std::string &value) {
    TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
    LOG(LM_INFO, "Attribut " << element << " " << attribut << " " << value);

  };
  void Value(const std::string &value) {
    TRACE(PARAM(value));
    LOG(LM_INFO, "Value " << showName() << " = " << value);
    if (not member())
      LOG(LM_INFO, "Variable fehlt " << showName())
    else
      member()->fromStr(value);
  };
  void Cdata(const char *val, size_t len) {
    std::string value(val, len);
    TRACE(PARAM(value));
    LOG(LM_INFO, "Cdata " << value << " atadC");
    if (not member())
      LOG(LM_WARNING, "Variable fehlt " << showName())
    else
      member()->fromStr(value);
  };
  void StartTag(const std::string &element) {
    TRACE(PARAM(element));
    LOG(LM_INFO, "Start " << element << " " << tagPath().size());

    if (tagPath().size() <= 1)
      return;
    if (not enter(element))
      LOG(LM_INFO, element << " wurde nicht gefunden");


  }
  void EndTag(const std::string &element) {
    TRACE(PARAM(element));
    LOG(LM_INFO, "Ende " << element);
    if (tagPath().size() > 1)
      exit(element);
  }
  void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::string &value) {
    TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
    LOG(LM_INFO, "ProcInst " << element << " " << attribut << " " << value);
    if (element == "xml" and attribut == "encoding")
      encoding = value;
  }

  
  
  private:
    std::string prefix;;
  };
  
  
  
  void XmlRead::parse()
  {
    TRACE("");
    data->parse();
  }
  
  XmlRead::XmlRead(const std::string &input)
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
    } catch (std::exception &e) {
      LOG(LM_INFO, "Exception " << e.what());
      size_t pos;
      const std::string &xml = data->info(pos);
          LOG(LM_INFO, xml.substr(pos));
      throw std::runtime_error(std::string("Parsing failed ") + " at pos. " + std::to_string(pos));
    }
  }
  
  

#include "readjson.h"

#include<stack>
#include<limits.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/prettywriter.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"


using namespace std;
using namespace rapidjson;









class JsonReadData {
  public:
    JsonReadData(const string &input) : json(input), buffer(), writer(buffer), my_count(0), my_errors(0) { path.resize(1); };
    ~JsonReadData() { };
    const string &json;
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer;

    stack<bool> inArray;

    class Objekt {
      public:
      Objekt(ObjectBase *o, const string &name, bool init = false) {
        obj = o;
        objName = name;
        inProgress = not init;
        //cerr << "OBJ " << name << endl;
      };
      ~Objekt() {};

      string objName;
      ObjectBase *obj = 0;
      MemBaseVector *vec = 0;
      bool inProgress = false;
    };
    stack<Objekt> objekte;
    vector<string> path;
    string lastKey;



  MemberBase *getMem()
  {
    if (not objekte.empty() and not lastKey.empty() and objekte.top().obj and objekte.top().inProgress)
    {
      cerr << "GET MEM " << lastKey << endl;
      MemberBase *m = 0;
      if (objekte.top().vec)
      {
        cerr << "IN VEC" << endl;
        MemBaseVector *vec = objekte.top().vec;
        size_t s = vec->size();
        vec->resize(s+1);

        m = vec->getMemInfo(s);
        if (m)
          cerr << "FOUNF VEC " << m->name() << endl;
        else
          cerr << "VEC NOT FOUND " << lastKey << endl;
      }
      if (not m)
        m = objekte.top().obj->getMemInfo(lastKey);
      if (m)
        cerr << "FOUNF " << lastKey << " " << m->name() << endl;
      {
        string p;
        for (auto &i:path)
          p += i + "::";

        cerr << "UNKNOWN " << p << endl; 
      }
      return m;
    }
    return 0;
  };

  bool Null() {
    cout << "Null()" << endl;
    MemberBase *mem = getMem();
    if (mem and mem->nullAllowed())
    {
      mem->setNull(true);
      return true;
    }
    return true; }
  bool Bool(bool b) {
    cout << "Bool(" << boolalpha << b << ")" << endl;
    MemberBase *mem = getMem();
    Member<bool> *mbool = dynamic_cast<Member<bool> *>(mem);
    if (mbool)
    {
      mbool->operator()(b);
      return true;
    }
  return true; }
  bool Int(int i) {
    cout << "Int(" << i << ")" << endl;
    MemberBase *mem = getMem();
    Member<int> *mint = dynamic_cast<Member<int> *>(mem);
    Member<double> *mdouble = dynamic_cast<Member<double> *>(mem);
    Member<bool> *mbool = dynamic_cast<Member<bool> *>(mem);
    if (mint)
    {
      cerr << "MINT" << endl;
      mint->operator()(int(i));
      return true;
    }
  return true; }
  bool Uint(unsigned u) {
    cout << "Uint(" << u << ")" << endl;
    MemberBase *mem = getMem();
    Member<int> *mint = dynamic_cast<Member<int> *>(mem);
    Member<double> *mdouble = dynamic_cast<Member<double> *>(mem);
    Member<bool> *mbool = dynamic_cast<Member<bool> *>(mem);
    if (mint)
    {
      cerr << "MINT" << endl;
      if (u > INT_MAX)
        throw runtime_error("INT_Overflow");
      mint->operator()(int(u));
      return true;
    }
  return true; }
  bool Int64(int64_t i) {
    cout << "Int64(" << i << ")" << endl;
  return true; }
  bool Uint64(uint64_t u) {
    cout << "Uint64(" << u << ")" << endl;
  return true; }
  bool Double(double d) {
    cout << "Double(" << d << ")" << endl;
  return true; }
  bool RawNumber(const char* str, SizeType length, bool copy) {
    cout << "Number(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
    return true;
  }
  bool String(const char* str, SizeType length, bool copy) {
    cout << "String(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
    MemberBase *mem = getMem();
    Member<int> *mint = dynamic_cast<Member<int> *>(mem);
    Member<double> *mdouble = dynamic_cast<Member<double> *>(mem);
    Member<bool> *mbool = dynamic_cast<Member<bool> *>(mem);
    Member<string> *mstring = dynamic_cast<Member<string> *>(mem);
    cerr << "KJKKJKJ" << endl;
    if (mstring)
    {
      mstring->operator()(string(str, length));
      return true;
    }
    return true;
  }
  bool StartObject() {
    cout << "StartObject(" << ")" << endl;
    if (objekte.empty())
      throw runtime_error("JsonRead: Fatal: keine Objekt");

    if (not objekte.top().inProgress)
    {
      // allererster Aufruf
      objekte.top().inProgress = true;
      return true;
    }
    else if (objekte.top().vec)
    {
      cerr << "OBJ IN VEC" << endl;
      MemBaseVector *vec = objekte.top().vec;
      size_t s = vec->size();
      vec->resize(s+1);

      ObjectBase *o = vec->getObjInfo(s);
      if (o)
      {
        cerr << "FOUNF OBJ VEC " << endl;
        objekte.push(JsonReadData::Objekt(o, vec->name() + "[]"));
        path.resize(path.size()+1);
        return true;
      }
      else
        cerr << "OBJ VEC NOT FOUND " << endl;
        // ab hier alles weitere Ignrieren
    }
    else if (not lastKey.empty() and objekte.top().obj)
    {
      ObjectBase *o = objekte.top().obj->getObjInfo(lastKey);
      if (o)
      {
        objekte.push(JsonReadData::Objekt(o, o->name()));
        path.resize(path.size()+1);
        return true;
      }
    }
    // Dummy Objekt erzeugen
    objekte.push(JsonReadData::Objekt(0, lastKey));
    path.resize(path.size()+1);
    return true; }
  bool Key(const char* str, SizeType length, bool copy) {
    lastKey = string(str, length);
    cout << "Key(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
    path[path.size()-1] = lastKey;
    return true;
  }
  bool EndObject(SizeType memberCount) {
    cout << "EndObject(" << memberCount << ")" << endl;
    if (not objekte.empty() and objekte.top().inProgress)
      objekte.pop();
    path.resize(path.size()-1);
    if (objekte.empty())
      cerr << "OBJ STACK EMPTY" << endl;
      //throw runtime_error("Objektstack underflow");
    return true; }
  bool StartArray() {
    cout << "StartArray()" << endl;
    if (not objekte.empty() and not lastKey.empty() and objekte.top().obj and objekte.top().inProgress)
    {
      MemBaseVector *v = objekte.top().obj->getVecInfo(lastKey);
      if (v)
      {
        objekte.top().vec = v;
        v->resize(0);
      }
    }
    return true; }
  bool EndArray(SizeType elementCount) {
    cout << "EndArray(" << elementCount << ")" << endl;
    if (not objekte.empty())
      objekte.top().vec = 0;
    return true; }

  int my_count;
  int my_errors;
  private:
  string prefix;;
};



void JsonRead::parse()
{
  StringStream s(data->json.c_str());
  Reader reader;
  reader.Parse(s, *data);
  cerr << "PARSE FERTIG" << endl;
}

JsonRead::JsonRead(const string &input) //: os(stdout, writeBuffer, sizeof(writeBuffer), writer(os)
{
  data = new JsonReadData(input);
}

JsonRead::~JsonRead()
{
  delete data;
}

void JsonRead::fill(ObjectBase &obj)
{
  data->objekte.push(JsonReadData::Objekt(&obj, "", true));
  parse();
}



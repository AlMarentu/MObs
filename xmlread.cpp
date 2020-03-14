#include "xmlread.h"

#include<stack>
#include<limits.h>
#include<iostream>


using namespace std;




class XmlReadData  {
public:
  XmlReadData(const string &input) : Xml(input) {
    path.resize(1);
    pos1 = pos2 = posS = posE = 0;
  };
  ~XmlReadData() { };
  
  void parse2LT() {
    pos2 = Xml.find('<', pos1);
    //cerr << "PLT " << pos2 << endl;
  };
  void parse2GT() {
    pos2 = Xml.find_first_of("/ <>=\"?!", pos1);
    //cerr << "PGT " << pos2 << " " << pos1 << endl;
    if (pos2 == string::npos)
      throw runtime_error("Syntax");
  };
  void parse2Com() {
    pos2 = Xml.find(u8"--!>", pos1);
    //cerr << "P-- " << pos2 << " " << pos1 << endl;
    if (pos2 == string::npos)
      throw runtime_error("Syntax");
    pos1 = pos2 + 3;
  };
  void parse2CD() {
    pos2 = Xml.find(u8"]]>", pos1);
    //cerr << "PCD " << pos2 << " " << pos1 << endl;
    if (pos2 == string::npos)
      throw runtime_error("Syntax");
  };
  string getValue() {
      if (pos2 == string::npos)
        throw runtime_error("unexpected EOF");
    size_t p = pos1;
    pos1 = pos2;
    return Xml.substr(p, pos2 - p);
  };
  void clearValue() { posS = posE; }; // der Zwischenraum fand Verwendung
  void saveValue() {
    // Verwaltet den Zischenraum zwische <..Anweisungen..>
    // wenn nicht verwendet, darf es nur white space sein
    if (posS != posE)
    {
      //cerr << "White '" <<  Xml.substr(posS, posE - posS) << "'" << endl;
      size_t p = Xml.find_first_not_of(" \n\r\t", posS);
      if (p < posE)
      {
        pos1 = p;
        throw runtime_error("unexpected char");
      }
    }

    if (pos2 == string::npos)
      throw runtime_error("unexpected EOF");
    posS = pos1;
    posE = pos2;
    pos1 = pos2;
  };
  void eat(char c) {
    if (Xml[pos1] != c)
      throw runtime_error("Syntax eat " + to_string(c) + " at pos " + to_string(pos1));
    pos1++;
  };
  void eat() { //cerr << "EAT " << peek() << endl;
    pos1++; };
  char peek() {
    if (pos1 >= Xml.length())
      throw runtime_error("Syntax EOF");
    //cerr << "Peek " << Xml[pos1] << " " << pos1 << endl;
    return Xml[pos1];
  };
  
  const string &Xml;
  size_t pos1, pos2;  // current / search pointer for parsing
  size_t posS, posE;  // start / end pointer for last text
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
  string encoding;
  stack<Objekt> objekte;
  vector<string> path;
  string lastKey;
  
  
  
  MemberBase *getMem()
  {
    TRACE(PARAM(lastKey));
    if (not objekte.empty() and not lastKey.empty() and objekte.top().obj and objekte.top().inProgress)
    {
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
        cerr << "FOUND " << lastKey << " " << m->name() << endl;
      else
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
  
  void parse() {
    TRACE("");
    parse2LT();
    if (pos2 != 0)
      throw runtime_error(u8"Syntax Head");
    eat('<');
    if (peek() == '?')
    {
      // Parse priimäre Verarbeitungsanweisung
      eat();
      parse2GT();
      if (getValue() != "xml")
        throw runtime_error(u8"Syntax");
      for (;peek() != '?';)
      {
        eat(' ');
        parse2GT();
        string a = getValue();
        eat('=');
        eat('"');
        parse2GT();
        string v = getValue();
        eat('"');
        ProcInst("xml", a, v);
        if (a == u8"encoding")
          encoding = a;
      }
      eat('?');
      eat('>');
      parse2LT();
    }
    for (;pos2 != string::npos;)
    {
      saveValue();
      eat('<');

      if (peek() == '/')
      {
        // Parse End-Tag
        eat();
        parse2GT();
        string element = getValue();
        if (element.empty())
          throw runtime_error("missing tag");
        if (lastKey == element)
        {
          Value(string(&Xml[posS], posE-posS));
          clearValue();
          lastKey = "";
        }
        EndObject(element);
        parse2LT();
      }
      else if (peek() == '!')
      {
        eat();
        // Parse CDATA Element
        if (peek() == '[')
        {
          eat('[');
          eat('C');
          eat('D');
          eat('A');
          eat('T');
          eat('A');
          eat('[');
          parse2CD();
          Cdata(getValue());
          eat();
          eat();
        }
        else
        {
          // Parse Kommentar
          eat('-');
          eat('-');
          parse2Com();
        }
        parse2LT();
      }
      else if (peek() == '?')
      {
        // Parse Verarbeitungsanweisung
        eat();
        parse2GT();
        string element = getValue();
        for (;;)
        {
          if (peek() == '?')
          {
            eat();
            ProcInst(element, "", "");
            parse2LT();
            break;
          }
          eat(' ');
          parse2GT();
          string a = getValue();
          string v;
          if (peek() == '=')
          {
            eat('=');
            eat('"');
            parse2GT();
            v = getValue();
            eat('"');
          }
          ProcInst(element, a, v);
        }
        eat('>');

        continue;
      }
      // Parse Element-Beginn
      parse2GT();
      string element = getValue();
      if (element.empty())
        throw runtime_error("missing tag");
      for (;;)
      {
        if (peek() == '>')  // Ende eines Starttags
        {
          eat();
          StartObject(element);
          parse2LT();
          break;
        }
        else if (peek() == '/') // Leertag
        {
          eat();
          eat('>');
          Null(element);
          parse2LT();
          break;
        }
        eat(' ');
        parse2GT();
        string a = getValue();
        eat('=');
        eat('"');
        parse2GT();
        string v = getValue();
        eat('"');
        Attribut(element, a, v);
      }
      lastKey = element;
    }
    pos2 = Xml.length();
    saveValue();
    cerr << "DURCH" << endl;
      
  };
    
  void Null(const string &element) {
    TRACE(PARAM(element));
    cerr << "NULL " << element << endl;
    return;
    MemberBase *mem = getMem();
    if (mem and mem->nullAllowed())
    {
      mem->setNull(true);
      return;
    }
    
  };
  void Attribut(const string &element, const string &attribut, const string &value) {
    TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
    cerr << "Attribut " << element << " " << attribut << " " << value << endl;

  };
  void Value(const string &value) {
    TRACE(PARAM(value));
    cerr << "Value " << value << endl;

  };
  void Cdata(const string &value) {
    TRACE(PARAM(value));
    cerr << "Cdata " << value << "atadC" << endl;

  };
  void String(const char* str, int length, bool copy) {
    TRACE(PARAM(str) << PARAM(length) << PARAM(copy));
    MemberBase *mem = getMem();
    Member<int> *mint = dynamic_cast<Member<int> *>(mem);
    Member<double> *mdouble = dynamic_cast<Member<double> *>(mem);
    Member<bool> *mbool = dynamic_cast<Member<bool> *>(mem);
    Member<string> *mstring = dynamic_cast<Member<string> *>(mem);
    
    if (mstring)
    {
      mstring->operator()(string(str, length));
    }
    else if (mem)
      throw runtime_error(u8"Xml: string statt Zahl in Variable " + mem->name());
  };
  void StartObject(const string &element) {
    TRACE(PARAM(element));
    cerr << "Start " << element << endl;
    return;
    if (objekte.empty())
      throw runtime_error(u8"XmlRead: Fatal: keine Objekt");
    
    if (not objekte.top().inProgress)
    {
      // allererster Aufruf
      objekte.top().inProgress = true;
      return;
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
        objekte.push(XmlReadData::Objekt(o, vec->name() + "[]"));
        path.resize(path.size()+1);
        return;
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
        objekte.push(XmlReadData::Objekt(o, o->name()));
        path.resize(path.size()+1);
        return;
      }
    }
    // Dummy Objekt erzeugen
    objekte.push(XmlReadData::Objekt(0, lastKey));
    path.resize(path.size()+1);
  }
  //  bool Key(const char* str, int length, bool copy) {
  //    lastKey = string(str, length);
  //    path[path.size()-1] = lastKey;
  // }
  void EndObject(const string &element) {
    TRACE(PARAM(element));
    cerr << "Ende " << element << endl;
    return;
    if (not objekte.empty() and objekte.top().inProgress)
      objekte.pop();
    else
      throw runtime_error(u8"Objektstack underflow");
    path.resize(path.size()-1);
  }
  void ProcInst(const string &element, const string &attribut, const string &value) {
    TRACE(PARAM(element) << PARAM(attribut)<< PARAM(value));
    cerr << "ProcInst " << element << " " << attribut << " " << value << endl;

  }

  
  
    int my_count;
    int my_errors;
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
    data->objekte.push(XmlReadData::Objekt(&obj, "", true));
    try {
      parse();
    } catch (exception &e) {
      cerr << "Exception " << e.what() << endl;
      cerr << data->Xml.substr(data->pos1) << endl;
      throw runtime_error(string("Parsing failed ") + " at pos. " + to_string(data->pos1));
    }
  }
  
  

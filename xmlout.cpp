
#include "xmlout.h"
#include "logging.h"


#include <stack>
#include <sstream>
#include <iostream>


using namespace std;



class XmlOutData {
public:
  XmlOutData() { };
  ~XmlOutData() {};
  stringstream buffer;
  int level = 0;
  bool doIndent = true;
  string prefix;
  stack<string> elements;
  void indent()
  {
    if (doIndent)
    {
      string s;
      s.resize(level * 2, ' ');
      buffer << s;
    }
  };
};

XmlOut::XmlOut(bool indent)
{
  data = new XmlOutData;
  data->doIndent = indent;
}

XmlOut::~XmlOut()
{
  delete data;
}

void XmlOut::startList(string name)
{
  data->buffer << u8"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
  data->elements.push(u8"list");
  data->buffer << "<" << data->prefix << data->elements.top() << ">\n";
  data->level++;
  data->elements.push(u8"entry");
}



void XmlOut::clear()
{
   data->buffer.clear();
}

string  XmlOut::getString()
{
  if (data->level == 1 and data->elements.size() == 2)
  {
    data->level--;
    data->elements.pop();
    data->buffer << "</" << data->prefix << data->elements.top() << ">\n";
  }

  return data->buffer.str();
}



void XmlOut::doObjBeg(ObjTravConst &ot, const ObjectBase &obj)
{
  if (data->elements.empty())
  {
    data->buffer << u8"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    data->elements.push(u8"root");
  }
  
  string name;
  if (not data->elements.empty())
    name = data->elements.top();
  if (name.empty())
    name = obj.name();
  if (name.empty())
    name = u8"entry";

  data->indent();
  if (obj.isNull())
    data->buffer << '<' << data->prefix << name << u8"/>\n";
  else
    data->buffer << '<' << data->prefix << name << u8">\n";
  
  data->elements.push("");
  data->level++;

}

void XmlOut::doObjEnd(ObjTravConst &ot, const ObjectBase &obj)
{ 
  data->level--;
  data->elements.pop();

  if (not obj.isNull())
  {
    string name;
    if (not data->elements.empty())
      name = data->elements.top();
    if (name.empty())
      name = obj.name();
    
    data->indent();
    data->buffer << u8"</" << data->prefix << name << u8">\n";
  }
}

void XmlOut::doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec)
{
  data->elements.push(vec.name());
}

void XmlOut::doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec)
{
  data->elements.pop();
}

void XmlOut::doMem(ObjTravConst &ot, const MemberBase &mem)
{
  string name;
  if (not data->elements.empty())
    name = data->elements.top();
  if (name.empty())
    name = mem.name();
  data->indent();
  data->buffer << '<' << data->prefix << name;
  if (mem.isNull())
    data->buffer << "/>\n";
  else
  {
    data->buffer << '>';
    const string &value = mem.toStr();
    for (const auto c:value)
      switch (c)
      {
        case '<': data->buffer << u8"&lt;"; break;
        case '>': data->buffer << u8"&gt;"; break;
        default: data->buffer << c;
      }
    data->buffer << u8"</" << data->prefix << name << u8">\n";
  }
}

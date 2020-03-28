// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs https://github.com/AlMarentu/MObs.git
//
// MObs is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "xmlout.h"
#include "logging.h"


#include <stack>
#include <sstream>
#include <iostream>


using namespace std;

namespace mobs {


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

}

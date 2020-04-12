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
  XmlOutData(const ConvObjToString &c) : cth(c) { };
  ~XmlOutData() {};
  stringstream buffer;
  int level = 0;
  string prefix;
  stack<string> elements;
  const ConvObjToString cth;
  void indent()
  {
    if (cth.withIndentation())
    {
      string s;
      s.resize(level * 2, ' ');
      buffer << s;
    }
  };
};

XmlOut::XmlOut(ConvObjToString cth)
{
  data = new XmlOutData(cth);
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
  {
    size_t n = (obj.parent() and data->cth.useAltNames()) ? obj.cAltName() : SIZE_T_MAX;
    name = (n == SIZE_T_MAX) ? obj.name() : obj.parent()->getConf(n);
  }
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
    {
      size_t n = (obj.parent() and data->cth.useAltNames()) ? obj.cAltName() : SIZE_T_MAX;
      name = (n == SIZE_T_MAX) ? obj.name() : obj.parent()->getConf(n);
    }
    
    data->indent();
    data->buffer << u8"</" << data->prefix << name << u8">\n";
  }
}

void XmlOut::doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec)
{
  size_t n = (vec.parent() and data->cth.useAltNames()) ? vec.cAltName() : SIZE_T_MAX;
  data->elements.push((n == SIZE_T_MAX) ? vec.name() : vec.parent()->getConf(n));
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
  {
    size_t n = (mem.parent() and data->cth.useAltNames()) ? mem.cAltName() : SIZE_T_MAX;
    name = (n == SIZE_T_MAX) ? mem.name() : mem.parent()->getConf(n);
  }
  data->indent();
  data->buffer << '<' << data->prefix << name;
  if (mem.isNull())
    data->buffer << "/>\n";
  else
  {
    data->buffer << '>';
    const string &value = mem.toStr(data->cth);
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

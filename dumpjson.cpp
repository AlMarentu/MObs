// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs
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


#include "dumpjson.h"

#include<stack>

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/prettywriter.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"


using namespace std;
using namespace rapidjson;



class JsonOutData {
  public:
    JsonOutData() : buffer(), writer(buffer) { };
    ~JsonOutData() {};
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer;
    stack<bool> inArray;
};

JsonOut::JsonOut() //: os(stdout, writeBuffer, sizeof(writeBuffer), writer(os)
{
  data = new JsonOutData;
}

JsonOut::~JsonOut()
{
  delete data;
}


void JsonOut::clear()
{
   data->buffer.Clear();
}

string  JsonOut::getString()
{
   return string(data->buffer.GetString(), data->buffer.GetSize());
}

void JsonOut::doObjBeg(ObjTravConst &ot, const ObjectBase &obj)
{ 
  if (not data->inArray.empty() and not data->inArray.top())
    data->writer.String(obj.name());
  if (obj.isNull())
    data->writer.Null();
  else
  {
    data->writer.StartObject();
    data->inArray.push(false);
  }
}

void JsonOut::doObjEnd(ObjTravConst &ot, const ObjectBase &obj)
{ 
  if (not obj.isNull())
  {
    data->inArray.pop();
    data->writer.EndObject();
  }
}

void JsonOut::doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec)
{ 
  data->writer.String(vec.name());
  if (vec.isNull() and vec.size() == 0)
    data->writer.Null();
  else
  {
    data->writer.StartArray();
    data->inArray.push(true);
  }
}

void JsonOut::doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec)
{ 
  if (not vec.isNull() or vec.size() != 0)
  {
    data->inArray.pop();
    data->writer.EndArray();
  }
}

void JsonOut::doMem(ObjTravConst &ot, const MemberBase &mem)
{ 
  if (data->inArray.empty() or not data->inArray.top())
    data->writer.String(mem.name());
  if (mem.isNull())
    data->writer.Null();
  else
  {
    const Member<int> *mint = dynamic_cast<const Member<int> *>(&mem);
    const Member<double> *mdouble = dynamic_cast<const Member<double> *>(&mem);
    const Member<bool> *mbool = dynamic_cast<const Member<bool> *>(&mem);
    if (mbool)
      data->writer.Bool((*mbool)());
    else if (mint)
      data->writer.Int((*mint)());
    else if (mdouble)
      data->writer.Double((*mdouble)());
    else
      data->writer.String(mem.toStr());
  }
}

class JsonDumpData {
  public:
    JsonDumpData() : buf((char *)::operator new[](65536)), os(stdout, buf, 65536), writer(os) {};
    ~JsonDumpData() { delete[] buf;};
    char *buf;
    FileWriteStream os;
    PrettyWriter<FileWriteStream> writer;
    stack<bool> inArray;
};

JsonDump::JsonDump() //: os(stdout, writeBuffer, sizeof(writeBuffer), writer(os)
{
  data = new JsonDumpData;
}

JsonDump::~JsonDump()
{
  data->os.Put('\n');
  data->os.Flush();
  delete data;
}


void JsonDump::doObjBeg(ObjTravConst &ot, const ObjectBase &obj)
{ 
  if (not data->inArray.empty() and not data->inArray.top())
    data->writer.String(obj.name());
  if (obj.isNull())
    data->writer.Null();
  else
  {
    data->writer.StartObject();
    data->inArray.push(false);
  }
}

void JsonDump::doObjEnd(ObjTravConst &ot, const ObjectBase &obj)
{ 
  if (not obj.isNull())
  {
    data->inArray.pop();
    data->writer.EndObject();
  }
}

void JsonDump::doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec)
{ 
  data->writer.String(vec.name());
  if (vec.isNull() and vec.size() == 0)
    data->writer.Null();
  else
  {
    data->writer.StartArray();
    data->inArray.push(true);
  }
}

void JsonDump::doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec)
{ 
  if (not vec.isNull() or vec.size() != 0)
  {
    data->inArray.pop();
    data->writer.EndArray();
  }
}

void JsonDump::doMem(ObjTravConst &ot, const MemberBase &mem)
{ 
  if (data->inArray.empty() or not data->inArray.top())
    data->writer.String(mem.name());
  if (mem.isNull())
    data->writer.Null();
  else
  {
    const Member<int> *mint = dynamic_cast<const Member<int> *>(&mem);
    const Member<double> *mdouble = dynamic_cast<const Member<double> *>(&mem);
    const Member<bool> *mbool = dynamic_cast<const Member<bool> *>(&mem);
    if (mbool)
      data->writer.Bool((*mbool)());
    else if (mint)
      data->writer.Int((*mint)());
    else if (mdouble)
      data->writer.Double((*mdouble)());
    else
      data->writer.String(mem.toStr());
  }
}

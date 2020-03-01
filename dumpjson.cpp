
#include "dumpjson.h"

#include<stack>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/reader.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>


using namespace std;
using namespace rapidjson;


class JsonDumpData;
class JsonDump : virtual public ObjTrav {
  public:
    JsonDump();
    ~JsonDump();
    virtual void doObjBeg(ObjTrav &ot, ObjectBase &obj);
    virtual void doObjEnd(ObjTrav &ot, ObjectBase &obj);
    virtual void doArrayBeg(ObjTrav &ot, MemBaseVector &vec);
    virtual void doArrayEnd(ObjTrav &ot, MemBaseVector &vec);
    virtual void doMem(ObjTrav &ot, MemberBase &mem);
  private:
    JsonDumpData *data;

};

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

void JsonOut::doObjBeg(ObjTrav &ot, ObjectBase &obj)
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

void JsonOut::doObjEnd(ObjTrav &ot, ObjectBase &obj)
{ 
  if (not obj.isNull())
  {
    data->inArray.pop();
    data->writer.EndObject();
  }
}

void JsonOut::doArrayBeg(ObjTrav &ot, MemBaseVector &vec)
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

void JsonOut::doArrayEnd(ObjTrav &ot, MemBaseVector &vec)
{ 
  if (not vec.isNull() or vec.size() != 0)
  {
    data->inArray.pop();
    data->writer.EndArray();
  }
}

void JsonOut::doMem(ObjTrav &ot, MemberBase &mem)
{ 
  if (data->inArray.empty() or not data->inArray.top())
    data->writer.String(mem.name());
  if (mem.isNull())
    data->writer.Null();
  else
  {
    Member<int> *mint = dynamic_cast<Member<int> *>(&mem);
    Member<double> *mdouble = dynamic_cast<Member<double> *>(&mem);
    Member<bool> *mbool = dynamic_cast<Member<bool> *>(&mem);
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


void JsonDump::doObjBeg(ObjTrav &ot, ObjectBase &obj)
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

void JsonDump::doObjEnd(ObjTrav &ot, ObjectBase &obj)
{ 
  if (not obj.isNull())
  {
    data->inArray.pop();
    data->writer.EndObject();
  }
}

void JsonDump::doArrayBeg(ObjTrav &ot, MemBaseVector &vec)
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

void JsonDump::doArrayEnd(ObjTrav &ot, MemBaseVector &vec)
{ 
  if (not vec.isNull() or vec.size() != 0)
  {
    data->inArray.pop();
    data->writer.EndArray();
  }
}

void JsonDump::doMem(ObjTrav &ot, MemberBase &mem)
{ 
  if (data->inArray.empty() or not data->inArray.top())
    data->writer.String(mem.name());
  if (mem.isNull())
    data->writer.Null();
  else
  {
    Member<int> *mint = dynamic_cast<Member<int> *>(&mem);
    Member<double> *mdouble = dynamic_cast<Member<double> *>(&mem);
    Member<bool> *mbool = dynamic_cast<Member<bool> *>(&mem);
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

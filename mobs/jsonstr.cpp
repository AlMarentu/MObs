// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2025 Matthias Lautner
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


#include "jsonstr.h"
#include "objgen.h"

#include <utility>

namespace  {
class ObjDump : virtual public mobs::ObjTravConst {
public:
  using JS = mobs::JsonStream;
  explicit ObjDump(JS &js) : jstr(js) { };
  bool doObjBeg(const mobs::ObjectBase &obj) override
  {
    if (obj.isNull() and jstr.cts().omitNull())
      return false;
    if (not obj.isModified() and jstr.cts().modOnly())
      return false;
    if (not obj.getElementName().empty() and not jstr.isRoot())
      jstr << JS::Tag(obj.getName(jstr.cts()));
    if (obj.isNull())
    {
      jstr << nullptr;
      return false;
    }
    jstr.objectBegin();
    return true;
  };
  void doObjEnd(const mobs::ObjectBase &obj) override
  {
    if (obj.isNull() and jstr.cts().omitNull())
      return;
    jstr.objectEnd();
  };
  bool doArrayBeg(const mobs::MemBaseVector &vec) override
  {
    if (vec.isNull() and jstr.cts().omitNull())
      return false;
    if (not vec.isModified() and jstr.cts().modOnly())
      return false;
    if (not jstr.isRoot())
      jstr << JS::Tag(vec.getName(jstr.cts()));
    if (vec.isNull())
    {
      jstr << nullptr;
      return false;
    }
    jstr.arrayBegin();
    return true;
  };
  void doArrayEnd(const mobs::MemBaseVector &vec) override
  {
    jstr.arrayEnd();
  };
  void doMem(const mobs::MemberBase &mem) override
  {
    if (mem.isNull() and jstr.cts().omitNull())
      return;
    if (not mem.isModified() and jstr.cts().modOnly())
      return;
    if (not inArray())
      jstr << JS::Tag(mem.getName(jstr.cts()));
    jstr << mem;
  };
private:
  JS &jstr;
};

}
namespace mobs {


class JsonStr_Data {
public:
  class Info {
  public:
    explicit Info(bool a) : array(a) {};
    bool array;
    bool comma = false;
  };

  JsonStr_Data(std::ostream &s, mobs::ConvObjToString c) : ostr(s), cth(std::move(c)) {

  }

  void elementCheck() const {
    if (not lastTag and not inSimpleList)
      throw std::runtime_error("JSON syntax");
  }

  void delimElement(bool noComma = false) {
    bool wasTag = lastTag;
    lastTag = false;
    if (info.empty())
      return;
    if (info.top().comma and not noComma)
      ostr << ',';
    else
      info.top().comma = true;
    if (cth.withIndentation()) {
      // Damit simpl arrays schön in einer Zeile dargestellt werden hier etwas verkünsteln
      auto s = ostr.tellp();
      if (inSimpleList) {
        auto len = s - beginOfLine;
        if (len < 80)
          wasTag = true;
      }
      if (wasTag)
        ostr << ' ';
      else {
        beginOfLine = s + std::streamoff(1);
        ostr << "\n" << std::string(indentCnt, ' ');
      }
    }
  }

  void objectBegin() {
    inSimpleList = false;
    if (not info.empty() and info.top().array) {
      if (info.top().comma)
        ostr << ',';
      else
        info.top().comma = true;
    }
    info.emplace(false);
    delimElement();
    ostr << '{';
    info.top().comma = false;
    indentCnt += 2;
  }

  void objectEnd() {
    if (info.empty() or indentCnt < 2 or info.top().array)
      throw std::runtime_error("JSON syntax");
    indentCnt -= 2;
    delimElement(true);
    ostr << '}';
    info.pop();
  }

  void arrayBegin() {
    delimElement(true);
    info.emplace(true);
    ostr << '[';
    indentCnt += 2;
    inSimpleList = true;
  }

  void arrayEnd() {
    if (info.empty() or not info.top().array)
      throw std::runtime_error("JSON syntax");
    indentCnt -= 2;
    delimElement(true);
    ostr << ']';
    info.pop();
    inSimpleList = false;
  }

  std::ostream &ostr;
  const mobs::ConvObjToString cth;
  int indentCnt = 0;
  bool lastTag = false;
  bool inSimpleList = false;
  std::streampos beginOfLine{};
  std::stack<Info> info;
};


JsonStream::JsonStream(std::ostream &s, const mobs::ConvObjToString &cth) {
  data = new JsonStr_Data(s, cth);
}

JsonStream::~JsonStream() {
  delete data;
}

JsonStream &JsonStream::operator<<(int t) {
  data->elementCheck();
  data->delimElement(data->lastTag);
  data->ostr << t;
  return *this;
}

JsonStream &JsonStream::operator<<(uint64_t t) {
  data->elementCheck();
  data->delimElement(data->lastTag);
  data->ostr << t;
  return *this;
}

JsonStream &JsonStream::operator<<(int64_t t) {
  data->elementCheck();
  data->delimElement(data->lastTag);
  data->ostr << t;
  return *this;
}

JsonStream &JsonStream::operator<<(bool t) {
  data->elementCheck();
  data->delimElement(data->lastTag);
  data->ostr << std::boolalpha << t;
  return *this;
}

JsonStream &JsonStream::operator<<(const std::string &t) {
  data->elementCheck();
  data->delimElement(data->lastTag);
  data->ostr << mobs::to_quoteJson(t);
  return *this;
}

JsonStream &JsonStream::operator<<(const std::wstring &t) {
  data->elementCheck();
  data->delimElement(data->lastTag);
  data->ostr << mobs::to_quoteJson(mobs::to_string(t));
  return *this;
}

JsonStream &JsonStream::operator<<(nullptr_t) {
  data->elementCheck();
  data->delimElement(data->lastTag);
  data->ostr << "null";
  return *this;
}

JsonStream &JsonStream::operator<<(const mobs::ObjectBase &t) {
  ObjDump od(*this);
  t.traverse(od);
  return *this;
}

JsonStream &JsonStream::operator<<(const MemberBase &mem) {
  if (mem.isNull())
    return operator<<(nullptr);
  data->elementCheck();
  data->delimElement(data->lastTag);
  if (mem.is_chartype(cts()))
    data->ostr << mobs::to_quoteJson(mem.toStr(cts()));
  else
    data->ostr << mem.toStr(cts());
  return *this;
}

JsonStream &JsonStream::operator<<(const JsonStream::Tag &tag) {
  if (data->lastTag or data->inSimpleList)
    throw std::runtime_error("JSON syntax");
  data->delimElement();
  data->ostr << mobs::to_quoteJson(tag.t) << ':';
  data->lastTag = true;
  return *this;
}


JsonStream &JsonStream::operator<<(JsonStream::Operator o) {
  switch(o) {
    case JsonStream::ArrayBegin: arrayBegin(); break;
    case JsonStream::ArrayEnd: arrayEnd(); break;
    case JsonStream::ObjectBegin: objectBegin(); break;
    case JsonStream::ObjectEnd: objectEnd(); break;
  }
  return *this;
}


void JsonStream::objectBegin() {
  data->objectBegin();
}

void JsonStream::objectEnd() {
  data->objectEnd();
}

void JsonStream::arrayBegin() {
  data->arrayBegin();
}

void JsonStream::arrayEnd() {
  data->arrayEnd();
}

const ConvObjToString &JsonStream::cts() const {
  return data->cth;
}

bool JsonStream::isRoot() const {
  return data->info.empty();
}

JsonStream &JsonStream::operator<<(const char *t) {
  if (t)
    return operator<<(std::string(t));
  return operator<<(nullptr);
}

JsonStream &JsonStream::operator<<(const wchar_t *t) {
  if (t)
    return operator<<(std::wstring(t));
  return operator<<(nullptr);
}

JsonStream &JsonStream::operator<<(wchar_t t) {
  if (t)
    return operator<<(std::wstring() + t);
  return operator<<(nullptr);
}

JsonStream &JsonStream::operator<<(char t) {
  if (t)
    return operator<<(std::string() + t);
  return operator<<(nullptr);
}

JsonStream &JsonStream::operator<<(const MDate &t) {
  return operator<<(to_string(t));
}

JsonStream &JsonStream::operator<<(const MTime &t) {
  return operator<<(to_string(t));
}


}

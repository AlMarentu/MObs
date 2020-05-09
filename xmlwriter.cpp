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


#include "xmlwriter.h"
#include "xmlout.h"
#include "logging.h"
#include "converter.h"


#include <stack>
#include <sstream>
#include <iostream>


using namespace std;

namespace mobs {



class XmlWriterData {
public:
  XmlWriterData(std::wostream &str, XmlWriter::charset c, bool i) : buffer(str), cs(c), indent(i) { setConFun(); }
  XmlWriterData(XmlWriter::charset c, bool i) : cs(c), indent(i) { }
  std::wostream &buffer = wstrBuff;
  XmlWriter::charset cs;
  int level = 0;
  bool indent;
  bool openEnd = false;
  bool hasValue = false;
  std::wstring prefix;
  stack<wstring> elements;
  wstringstream wstrBuff; // buffer für u8-Ausgabe in std::string
  std::mbstate_t mbstate;
  wchar_t (*conFun)(wchar_t) = nullptr;
  

  void setConFun() {
    std::locale lo;
    lo = std::locale(buffer.getloc(), new codec_iso8859_1);
    buffer.imbue(lo);
    switch (cs) {
      case XmlWriter::CS_iso8859_1:
  //      lo = std::locale(data->buffer.getloc(), new codec_iso8859_1);
        break;
      case XmlWriter::CS_iso8859_9:
        lo = std::locale(buffer.getloc(), new codec_iso8859_9);
        break;
      case XmlWriter::CS_iso8859_15:
        lo = std::locale(buffer.getloc(), new codec_iso8859_15);
        break;
      case XmlWriter::CS_utf8_bom:
        if (buffer.tellp() == 0)  // writing BOM
          buffer << L'\u00ef' << L'\u00bb' << L'\u00bf';
        lo = std::locale(buffer.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
        break;
      case XmlWriter::CS_utf8:
        lo = std::locale(buffer.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
        break;
      case XmlWriter::CS_utf16_be:
        if (buffer.tellp() == 0)  // writing BOM
          buffer << L'\u00fe' << L'\u00ff';
        lo = std::locale(buffer.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, codecvt_mode(0)>);
        break;
      case XmlWriter::CS_utf16_le:
        if (buffer.tellp() == 0)  // writing BOM
          buffer << L'\u00ff' << L'\u00fe';
        lo = std::locale(buffer.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>);
        break;
    }
    buffer.imbue(lo);
  }
  
  void write(wchar_t c) {
//    if (conFun and (c & ~0x7f))
//      buffer << wchar_t(conFun(c));
//    else
      buffer << c;
  }
  void writeIndent() {
    if (indent)
    {
      wstring s;
      s.resize(level * 2, ' ');
      buffer << s;
    }
  }
  void closeTag() {
    if (openEnd)
    {
      buffer << '>';
      openEnd = false;
    }
  }

};






XmlWriter::XmlWriter(std::wostream &str, charset c, bool indent) {
  data = new XmlWriterData(str, c, indent);
}

XmlWriter::XmlWriter(charset c, bool indent) {
  data = new XmlWriterData(c, indent);
}

XmlWriter::~XmlWriter() {
  delete data;
}

int XmlWriter::level() const { return data->level; }
bool XmlWriter::attributeAllowed() const { return data->openEnd; }


void XmlWriter::writeHead() {
  wstring encoding;
  switch (data->cs) {
    case CS_iso8859_1: encoding = L"ISO-8859-1"; break;
    case CS_iso8859_9: encoding = L"ISO-8859-9"; break;
    case CS_iso8859_15: encoding = L"ISO-8859-15"; break;
    case CS_utf8_bom: encoding = L"UTF-8"; break;
    case CS_utf8: encoding = L"UTF-8"; break;
    case CS_utf16_be: encoding = L"UTF-16"; break;
    case CS_utf16_le: encoding = L"UTF-16"; break;
  }

  data->buffer << "<?xml";
  data->openEnd = true;
  writeAttribute(L"version", version);
  writeAttribute(L"encoding", encoding);
  writeAttribute(L"standalone", standalone ? L"yes":L"no");
  data->buffer << "?>";
  data->openEnd = false;
}

void XmlWriter::writeTagBegin(const std::wstring &tag) {
  data->closeTag();
  if (data->indent)
    data->buffer << '\n';
  data->writeIndent();
  data->buffer << '<' << data->prefix << tag;
  data->openEnd = true;
  data->elements.push(tag);
  data->level++;
}

void XmlWriter::writeAttribute(const std::wstring &attribute, const std::wstring &value) {
  if (not data->openEnd)
    LOG(LM_WARNING, "XmlWriter::writeAttribute error");
  data->buffer << ' ' << attribute << '=' << '"';
  for (const auto c:value)
    switch (c)
    {
      case '<': data->buffer << L"&lt;"; break;
      case '"': data->buffer << L"&quot;"; break;
      case '&': data->buffer << L"&amp;"; break;
      case 0 ... 0x1f: data->buffer << L"&#x" << hex << int(c) << L';'; break;
      default: data->write(c);
    }
  data->buffer << '"';
}

void XmlWriter::writeValue(const std::wstring &value) {
  data->closeTag();
  for (const auto c:value)
    switch (c)
    {
      case '<': data->buffer << L"&lt;"; break;
      case '>': data->buffer << L"&gt;"; break;
      case '&': data->buffer << L"&amp;"; break;
      case 0 ... 0x1f: if (escapeControll) { data->buffer << L"&#x" << hex << int(c) << L';'; break; }
      default: data->write(c);
    }
  data->hasValue = true;
}

void XmlWriter::writeCdata(const std::wstring &value) {
  data->closeTag();
  data->buffer << L"<![CDATA[" << value << L"]]>";
  data->hasValue = true;
}

void XmlWriter::writeBase64(const std::vector<u_char> &value) {
  data->closeTag();
  data->buffer << L"<![CDATA[";
  to_wostream_base64(data->buffer, value);
  data->buffer << L"]]>";
  data->hasValue = true;
}

void XmlWriter::writeTagEnd(bool forceNoNulltag) {
  if (data->elements.empty())
    throw runtime_error("XmlWriter::writeTagEnd unbalanced");
  data->level--;
  if (data->openEnd and not forceNoNulltag)
    data->buffer << L"/>";
  else {
    if (data->indent and not data->hasValue) {
      data->buffer << '\n';
      data->writeIndent();
    }
    data->buffer << L"</" << data->prefix << data->elements.top() << '>';
  }
  if (data->indent and level() == 0)
    data->buffer << '\n';
  data->elements.pop();
  data->hasValue = false;
  data->openEnd = false;
}

void XmlWriter::writeComment(const std::wstring &value, bool inNewLine) {
  data->closeTag();
  if (data->indent and inNewLine) {
    data->buffer << '\n';
    data->writeIndent();
  }
  data->buffer << L"<!-- ";
  for (const auto c:value)
    switch (c)
    {
      case '<': data->buffer << L"&lt;"; break;
      case '>': data->buffer << L"&gt;"; break;
      default: data->write(c);
    }
  data->buffer << L" -->";
}

void XmlWriter::setPrefix(const std::wstring &pf) {
  data->prefix = pf;
}

void XmlWriter::clearString()
{
   data->wstrBuff.clear();
}

string XmlWriter::getString() const
{
  string result;
// ist leer wenn filebuffer
  switch (data->cs) {
    case CS_iso8859_1: {
         const wstring &t = data->wstrBuff.str();
         std::transform(t.begin(), t.cend(), std::back_inserter(result),
                        [](wchar_t c) -> char{ return to_iso_8859_1(c); });
         return result;
       }
    case CS_iso8859_9: {
         const wstring &t = data->wstrBuff.str();
         std::transform(t.begin(), t.cend(), std::back_inserter(result),
                        [](wchar_t c) -> char{ return to_iso_8859_9(c); });
         return result;
       }
    case CS_iso8859_15: {
      const wstring &t = data->wstrBuff.str();
      std::transform(t.begin(), t.cend(), std::back_inserter(result),
                     [](wchar_t c) -> char{ return to_iso_8859_15(c); });
      return result;
    }
    case CS_utf8_bom:
    case CS_utf8:
      return to_string(data->wstrBuff.str());
    case CS_utf16_le:
    case CS_utf16_be:
      return "";
  }
  return "";
}




}

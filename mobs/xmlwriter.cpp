// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2024 Matthias Lautner
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
#include "logging.h"
#include "converter.h"


#include <stack>
#include <sstream>


using namespace std;

namespace mobs {



class XmlWriterData {
public:
  XmlWriterData(std::wostream &str, XmlWriter::charset c, bool i) : buffer(str), wostr(&str), cs(c), indent(i) { setConFun(); }
  XmlWriterData(XmlWriter::charset c, bool i) : cs(c), indent(i) { }
  std::wostream &buffer = wstrBuff;
  std::wostream *wostr = &wstrBuff;
  XmlWriter::charset cs;
  int level = 0;
  int cryptLevel = 0;
  bool indent;
  bool indentSave = false;
  bool openEnd = false;
  bool inHeader = false;
  bool hasValue = false;
  std::wstring prefix;
  stack<wstring> elements;
  std::unique_ptr<CryptOstrBuf> cryptBufp;
  std::unique_ptr<CryptBufBase> cryptSwap;
  stringstream cryptss;
  wstringstream wstrBuff; // buffer für u8-Ausgabe in std::string
  std::unique_ptr<std::ostream> binaryStream;
  std::ostream::pos_type binaryStart = 0;

  void setConFun() {
    std::locale lo;
    switch (cs) {
      case XmlWriter::CS_iso8859_1:
        lo = std::locale(buffer.getloc(), new codec_iso8859_1);
        break;
      case XmlWriter::CS_iso8859_9:
        lo = std::locale(buffer.getloc(), new codec_iso8859_9);
        break;
      case XmlWriter::CS_iso8859_15:
        lo = std::locale(buffer.getloc(), new codec_iso8859_15);
        break;
      case XmlWriter::CS_utf8_bom:
      case XmlWriter::CS_utf8:
        lo = std::locale(buffer.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
        break;
      case XmlWriter::CS_utf16_be:
        lo = std::locale(buffer.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, codecvt_mode(0)>);
        break;
      case XmlWriter::CS_utf16_le:
        lo = std::locale(buffer.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>);
        break;
      default:
        lo = std::locale(buffer.getloc(), new codec_iso8859_1);

    }
    buffer.imbue(lo);
    if (buffer.tellp() == 0)  // writing BOM
      switch (cs) {
        case XmlWriter::CS_utf16_be:
        case XmlWriter::CS_utf16_le:
        case XmlWriter::CS_utf8_bom:
          buffer << L'\ufeff';
          break;
        default:
          break;
      }
  }
  
  void write(wchar_t c) {
//    if (conFun and (c & ~0x7f))
//      buffer << wchar_t(conFun(c));
//    else
      *wostr << c;
  }
  void writeIndent() {
    if (indent)
    {
      wstring s;
      s.resize(level * 2, L' ');
      *wostr << s;
    }
  }
  void closeTag() {
    if (openEnd)
    {
      if (inHeader) {
        *wostr << L'?';
        inHeader = false;
      }
      *wostr << L'>';
      openEnd = false;
    }
  }

  std::ostream &byteStream(const char *delimiter = nullptr, CryptBufBase *cbbp = nullptr) {
    auto wbufp = dynamic_cast<CryptOstrBuf*>(wostr->rdbuf());
    if (not wbufp)
      throw std::runtime_error("no mobs::CryptOstrBuf");
    *wostr << flush;
    if (cbbp)
    {
      binaryStream = std::unique_ptr<std::ostream>(new std::ostream(cbbp));
      if (delimiter)
        wbufp->getOstream() << delimiter;
      binaryStart = binaryStream->tellp();
      cbbp->setOstr(wbufp->getOstream());
      return *binaryStream;
    } else {
      if (delimiter)
        wbufp->getOstream() << delimiter;
      binaryStart = wbufp->getOstream().tellp();
      return wbufp->getOstream();
    }
  }

  std::streamsize closeByteStream() {
    if (binaryStream) {
      binaryStream->flush();
      std::streamsize size = binaryStream->tellp();
      if (size >= 0 and binaryStart >= 0)
        size -= binaryStart;
      else
        size = -1;
      if (auto sp = dynamic_cast<CryptBufBase *>(binaryStream->rdbuf())) {
        sp->finalize();
      }
      binaryStream = nullptr;
      return size;
    }
    return 0;
  }


};






XmlWriter::XmlWriter(std::wostream &str, charset c, bool indent) {
  data = std::unique_ptr<XmlWriterData>(new XmlWriterData(str, c, indent));
}

XmlWriter::XmlWriter(charset c, bool indent) {
  data = std::unique_ptr<XmlWriterData>(new XmlWriterData(c, indent));
}

XmlWriter::~XmlWriter() = default;

int XmlWriter::level() const { return data->level; }
int XmlWriter::cryptingLevel() const { return data->cryptLevel; }
bool XmlWriter::attributeAllowed() const { return data->openEnd; }


void XmlWriter::writeHead() {
  wstring encoding;
  switch (data->cs) {
    case CS_iso8859_1: encoding = L"ISO-8859-1"; break;
    case CS_iso8859_9: encoding = L"ISO-8859-9"; break;
    case CS_iso8859_15: encoding = L"ISO-8859-15"; break;
    case CS_utf8_bom:
    case CS_utf8: encoding = L"UTF-8"; break;
    case CS_utf16_be:
    case CS_utf16_le: encoding = L"UTF-16"; break;
  }

  data->buffer << "<?xml";
  data->openEnd = true;
  data->inHeader = true;
  data->level = 0;
  data->elements = stack<wstring>();
  writeAttribute(L"version", version);
  writeAttribute(L"encoding", encoding);
  if (standalone)
    writeAttribute(L"standalone", L"yes");
}

void XmlWriter::pushTag(const std::wstring &tag) {
  data->elements.push(tag);
  data->level++;
}

void XmlWriter::writeTagBegin(const std::wstring &tag) {
  data->closeTag();
  if (data->indent)
    *data->wostr << L'\n';
  data->writeIndent();
  *data->wostr << L'<' << data->prefix << tag;
  data->openEnd = true;
  data->elements.push(tag);
  data->level++;
}

void XmlWriter::writeAttribute(const std::wstring &attribute, const std::wstring &value) {
  if (not data->openEnd)
    LOG(LM_WARNING, "XmlWriter::writeAttribute error");
  *data->wostr << L' ' << attribute << L'=' << L'"';
  for (const auto c:value)
    switch (c)
    {
      case '<': *data->wostr << L"&lt;"; break;
      case '>': *data->wostr << L"&gt;"; break;
      case '"': *data->wostr << L"&quot;"; break;
      case '&': *data->wostr << L"&amp;"; break;
      case 0xD800 ... 0xDFFF:
      case 0xFFFE ... 0xFFFF:
      case 0x110000 ... 0x7FFFFFFF:
      case 0 ... 0x1f: *data->wostr << L"&#x" << hex << int(c) << L';'; break;
      default: data->write(c);
    }
  *data->wostr << L'"';
}

void XmlWriter::writeValue(const std::wstring &value) {
  data->closeTag();
  for (const auto c:value)
    switch (c)
    {
      case '<': *data->wostr << L"&lt;"; break;
      case '>': *data->wostr << L"&gt;"; break;
      case '&': *data->wostr << L"&amp;"; break;
      case 0 ... 8:
      case 11:
      case 12:
      case 14 ... 0x1f:
      case 0xD800 ... 0xDFFF:
      case 0xFFFE ... 0xFFFF:
      case 0x110000 ... 0x7FFFFFFF:
        *data->wostr << L"&#x" << hex << int(c) << L';';
        break;
      case '\r':
      case '\n':
      case '\t':
        if (escapeControl) { *data->wostr << L"&#x" << hex << int(c) << L';'; break; }
      default: data->write(c);
    }
  data->hasValue = true;
}

void XmlWriter::writeCdata(const std::wstring &value) {
  data->closeTag();
  for (size_t pos1 = 0;;) {
    size_t pos2 = value.find(L"]]>", pos1);
    if (pos2 == string::npos)
      pos2 = value.length();
    else
      pos2 += 1;
    *data->wostr << L"<![CDATA[" << value.substr(pos1, pos2-pos1) << L"]]>";
    if (pos2 == value.length())
      break;
    pos1 = pos2;
  }
  data->hasValue = true;
}

void XmlWriter::writeBase64(const u_char *value, uint64_t size) {
  data->closeTag();
  *data->wostr << L"<![CDATA[";
  string lBreak;
  if (data->indent) {
    lBreak = "\n";
    lBreak.resize(data->level * 2 +1, ' ');
  }
  copy_base64(&value[0], &value[size], std::ostreambuf_iterator<wchar_t>(*data->wostr), lBreak);
  *data->wostr << L"]]>";
  data->hasValue = true;
}

void XmlWriter::writeBase64(const std::vector<u_char> &value) {
  data->closeTag();
  *data->wostr << L"<![CDATA[";
  string lBreak;
  if (data->indent) {
    lBreak = "\n";
    lBreak.resize(data->level * 2 +1, ' ');
  }
  copy_base64(value.cbegin(), value.cend(), std::ostreambuf_iterator<wchar_t>(*data->wostr), lBreak);
  *data->wostr << L"]]>";
  data->hasValue = true;
}

void XmlWriter::writeTagEnd(bool forceNoNulltag) {
  if (data->elements.empty())
    throw runtime_error("XmlWriter::writeTagEnd unbalanced");
  data->level--;
  if (data->openEnd and not forceNoNulltag)
    *data->wostr << L"/>";
  else {
    if (data->indent and not data->hasValue) {
      *data->wostr << L'\n';
      data->writeIndent();
    }
    *data->wostr << L"</" << data->prefix << data->elements.top() << L'>';
  }
  if (data->indent and level() == 0)
    *data->wostr << L'\n' << flush;
  data->elements.pop();
  data->hasValue = false;
  data->openEnd = false;
}

void XmlWriter::writeComment(const std::wstring &value, bool inNewLine) {
  data->closeTag();
  if (data->indent and inNewLine) {
    *data->wostr << L'\n';
    data->writeIndent();
  }
  *data->wostr << L"<!-- ";
  for (const auto c:value)
    switch (c)
    {
      case '<': *data->wostr << L"&lt;"; break;
      case '>': *data->wostr << L"&gt;"; break;
      default: data->write(c);
    }
  *data->wostr << L" -->";
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
// ist leer, wenn filebuffer
  switch (data->cs) {
    case CS_iso8859_1: {
         const wstring &t = data->wstrBuff.str();
         std::transform(t.begin(), t.cend(), std::back_inserter(result),
                        [](wchar_t c) -> char{ return u_char(to_iso_8859_1(c)); });
         return result;
       }
    case CS_iso8859_9: {
         const wstring &t = data->wstrBuff.str();
         std::transform(t.begin(), t.cend(), std::back_inserter(result),
                        [](wchar_t c) -> char{ return u_char(to_iso_8859_9(c)); });
         return result;
       }
    case CS_iso8859_15: {
      const wstring &t = data->wstrBuff.str();
      std::transform(t.begin(), t.cend(), std::back_inserter(result),
                     [](wchar_t c) -> char{ return u_char(to_iso_8859_15(c)); });
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


void XmlWriter::startEncrypt(CryptBufBase *cbbp) {
  if (data->cryptBufp or data->cryptSwap or not cbbp or data->level == 0)
    THROW("invalid state encryption");
  data->cryptLevel = data->level;
  std::wstring pfx;
  data->prefix.swap(pfx);
  writeTagBegin(L"EncryptedData");
  writeAttribute(L"Type", L"https://www.w3.org/2001/04/xmlenc#Element");
  writeAttribute(L"xmlns", L"https://www.w3.org/2001/04/xmlenc#");
  writeTagBegin(L"EncryptionMethod");
  writeAttribute(L"Algorithm", L"https://www.w3.org/2001/04/xmlenc#" + to_wstring(cbbp->name()));
  writeTagEnd();
  size_t rcpt = cbbp->recipients();
  for (auto i = 0; i < rcpt; i++) {
    writeTagBegin(L"KeyInfo");
    writeAttribute(L"xmlns", L"https://www.w3.org/2000/09/xmldsig#");
    writeTagBegin(L"KeyName");
    string k = cbbp->getRecipientId(i);
    if (not k.empty())
      writeValue(to_wstring(k));
    writeTagEnd();
    string c = cbbp->getRecipientKeyBase64(i);
    if (not c.empty()) {
      writeTagBegin(L"CipherData");
      writeTagBegin(L"CipherValue");
      writeValue(to_wstring(c));
      writeTagEnd();
      writeTagEnd();
    }
    writeTagEnd();
  }
  writeTagBegin(L"CipherData");
  writeTagBegin(L"CipherValue");
  data->prefix.swap(pfx);
  data->indentSave = data->indent;
  data->indent = false;
  data->closeTag();
  data->cryptss.str("");
  data->cryptss.clear();
  // Wenn Ziel-Stream bereits ein mobs::CryptOstrBuf ist, dann statt in temporärem stream direkt in den Ziel-stream schreiben
  // spart hier einen Zwischenpuffer und die Latenz dessen Ein-/Ausgabe
  auto r = dynamic_cast<mobs::CryptOstrBuf *>(data->buffer.rdbuf());
  if (r) {
    data->buffer.flush();
    data->cryptSwap = std::unique_ptr<CryptBufBase>(cbbp);
    r->swapBuffer(data->cryptSwap);
    //data->cryptBufp = std::unique_ptr<CryptOstrBuf>(new CryptOstrBuf(r->getOstream(), cbbp));
    //data->wostr = new std::wostream(data->cryptBufp.get());
  } else {
    auto lo = data->buffer.getloc();
    data->cryptBufp = std::unique_ptr<CryptOstrBuf>(new CryptOstrBuf(data->cryptss, cbbp));
    data->wostr = new std::wostream(data->cryptBufp.get());
    data->wostr->imbue(lo);
  }
  *data->wostr << mobs::CryptBufBase::base64(true);
}

void XmlWriter::stopEncrypt() {
  if (data->cryptSwap) {
    if (auto r = dynamic_cast<mobs::CryptOstrBuf *>(data->buffer.rdbuf()))
      r->swapBuffer(data->cryptSwap);
    data->cryptSwap = nullptr;
  } else if (data->cryptBufp) {
    data->cryptBufp->finalize();
    const string &buf = data->cryptss.str();
    // Wenn auf den Ziel-Buffer des mobs::CryptOstrBuf geschrieben wurde, ist buf hier leer
    for (auto c: buf)
      data->buffer.put(c);
    delete data->wostr;
    data->cryptBufp = nullptr;
    data->cryptss.clear();
    data->wostr = &data->buffer;
  } else
    return;

  std::wstring pfx;
  data->prefix.swap(pfx);
  writeTagEnd();
  data->indent = data->indentSave;
  writeTagEnd();
  writeTagEnd();
  data->prefix.swap(pfx);
  data->cryptLevel = 0;
}

void XmlWriter::sync() {
  *data->wostr << flush;
}

void XmlWriter::putc(wchar_t c) {
  data->buffer.put(c);
}

std::ostream &XmlWriter::byteStream(const char *delimiter, CryptBufBase *cbbp)
{
  return data->byteStream(delimiter, cbbp);
}

std::streamsize XmlWriter::closeByteStream()
{
  return data->closeByteStream();
}


}

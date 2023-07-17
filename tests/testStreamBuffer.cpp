// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2021 Matthias Lautner
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


#include "csb.h"
#include "csb.h"

#include "objtypes.h"
#include "nbuf.h"
#include "objgen.h"
#include "xmlout.h"
#include "xmlwriter.h"
#include "xmlread.h"
#include "xmlparser.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>
#include <codecvt>

using namespace std;


namespace {

TEST(streamBufferTest, base) {
  stringstream ss("Gut");

  mobs::CryptIstrBuf streambufI(ss);
  std::wistream xin(&streambufI);

  wchar_t c;
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'G', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'u', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L't', c);
  EXPECT_TRUE(xin.get(c).eof());


}

TEST(streamBufferTest, char0) {
  stringstream ss;
  ss << u8"Gut" << '\0' << "ABCTest";

  mobs::CryptIstrBuf streambufI(ss);
  std::wistream xin(&streambufI);

  wchar_t c;
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'G', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'u', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L't', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'\0', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'A', c);


}

TEST(streamBufferTest, delimiter) {
  stringstream ss;
  ss.imbue(std::locale("de_DE.ISO8859-1"));
  ss.unsetf(ios::skipws);
  ss << u8"Gut" << '\0' << "A" << char(0x91) << char(0xaf) << char(0xff) << char(0xff) << "ABCTest";

  mobs::CryptIstrBuf streambufI(ss);
  streambufI.getCbb()->setReadDelimiter('\0');
  std::wistream xin(&streambufI);


  std::locale lo = std::locale(xin.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
  xin.imbue(lo);

  wchar_t c;
  char c8;
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'G', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'u', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L't', c);
  // Delimiter
  EXPECT_FALSE(ss.get(c8).eof());
  EXPECT_EQ('\0', c8);
  // 5 Zeichen binär
  istream::sentry sentry(ss, true);
  char buf[128];
  ASSERT_TRUE(sentry);
  ss.read(buf, 5);
  EXPECT_EQ('A', buf[0]);
  EXPECT_EQ(char(0x91), buf[1]);
  EXPECT_EQ(char(0xaf), buf[2]);

  // Rest der Streams
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ('A', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ('B', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ('C', c);


}


TEST(streamBufferTest, sizelimit) {
  stringstream ss(u8"GutABCTest");

  mobs::CryptIstrBuf streambufI(ss);
  // genau 3 Zeichen lesen
  streambufI.getCbb()->setReadLimit(3);
  std::wistream xin(&streambufI);


  wchar_t c;
  char c8;
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'G', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L'u', c);
  EXPECT_FALSE(xin.get(c).eof());
  EXPECT_EQ(L't', c);
  EXPECT_TRUE(xin.get(c).eof());

  EXPECT_FALSE(ss.get(c8).eof());
  EXPECT_EQ('A', c8);
  EXPECT_FALSE(ss.get(c8).eof());
  EXPECT_EQ('B', c8);
  EXPECT_FALSE(ss.get(c8).eof());
  EXPECT_EQ('C', c8);

}

TEST(streamBufferTest, B64BufferTest) {
  wstringstream ss(L"QUJDCg==>");
  mobs::Base64IstBuf b64buff(ss);
  istream istr(&b64buff);

  EXPECT_EQ(9, b64buff.in_avail());
  EXPECT_EQ('Q', istr.get());
  EXPECT_EQ(8, b64buff.in_avail());
  EXPECT_EQ('U', istr.get());
  EXPECT_EQ(7, b64buff.in_avail());
  EXPECT_EQ('J', istr.get());
  EXPECT_EQ(6, b64buff.in_avail());
  EXPECT_EQ('D', istr.get());
  EXPECT_EQ(5, b64buff.in_avail());
  EXPECT_EQ('C', istr.get());
  EXPECT_EQ(4, b64buff.in_avail());
  EXPECT_EQ('g', istr.get());
  istr.unget();
  EXPECT_EQ(1, b64buff.in_avail());
  EXPECT_EQ('g', istr.get());

  EXPECT_EQ(3, b64buff.in_avail());
  EXPECT_EQ('=', istr.get());
  EXPECT_EQ(2, b64buff.in_avail());
  EXPECT_EQ('=', istr.get());
  EXPECT_EQ(1, b64buff.in_avail());
  EXPECT_EQ(-1, istr.get());
  EXPECT_EQ(-1, b64buff.in_avail());
}

TEST(streamBufferTest, NBufferTest) {
  wstringstream ss(L"QUJDCg==>");
  mobs::Base64IstBuf b64buff(ss);
  istream b64str(&b64buff);

  mobs::CryptIstrBuf cbuf(b64str, new mobs::CryptBufNone);
  wistream istr(&cbuf);
  cbuf.getCbb()->setBase64(false);

  EXPECT_EQ(8, cbuf.in_avail());
  EXPECT_EQ(L'Q', istr.get());
  EXPECT_EQ(7, cbuf.in_avail());
  EXPECT_EQ('U', istr.get());
  EXPECT_EQ(6, cbuf.in_avail());
  EXPECT_EQ('J', istr.get());
  EXPECT_EQ(5, cbuf.in_avail());
  EXPECT_EQ('D', istr.get());
  EXPECT_EQ(4, cbuf.in_avail());
  EXPECT_EQ('C', istr.get());
  istr.unget();
  EXPECT_EQ(4, cbuf.in_avail());
  EXPECT_EQ('C', istr.get());
  EXPECT_EQ(3, cbuf.in_avail());
  EXPECT_EQ('g', istr.get());
  EXPECT_EQ(2, cbuf.in_avail());
  EXPECT_EQ('=', istr.get());
  EXPECT_EQ(1, cbuf.in_avail());
  EXPECT_EQ('=', istr.get());
  EXPECT_EQ(-1, cbuf.in_avail());
}

TEST(streamBufferTest, NBufferTest64) {
  wstringstream ss(L"QUJDCg==>");
  mobs::Base64IstBuf b64buff(ss);
  istream b64str(&b64buff);

  mobs::CryptIstrBuf cbuf(b64str, new mobs::CryptBufNone);
  wistream istr(&cbuf);
  cbuf.getCbb()->setBase64(true);

  EXPECT_EQ(4, cbuf.in_avail());
  EXPECT_EQ(L'A', istr.get());
  EXPECT_EQ(3, cbuf.in_avail());
  EXPECT_EQ(L'B', istr.get());
  EXPECT_EQ(2, cbuf.in_avail());
  EXPECT_EQ(L'C', istr.get());
  EXPECT_EQ(1, cbuf.in_avail());
  EXPECT_EQ(L'\n', istr.get());
  EXPECT_EQ(-1, cbuf.in_avail());
  EXPECT_EQ(-1, istr.get());
  EXPECT_EQ(-1, cbuf.in_avail());
}

namespace {


class Person : virtual public mobs::ObjectBase {
public:
  ObjInit(Person);
  MemVar(std::string, name);
};

static Person person;

class XmlInput : public mobs::XmlReader {
public:
  XmlInput(wistream &str) : mobs::XmlReader(str) {}

  XmlInput(const std::string &str, bool charsetUnknown = false) : XmlReader(str, mobs::ConvObjFromStr(),
                                                                            charsetUnknown) {}

//  virtual void NullTag(const std::string &element) { EndTag(element); }
  virtual void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value) override {
    LOG(LM_INFO, "attribute " << element << ":" << attribut << " = " << mobs::to_string(value));
  }

  virtual void Value(const std::wstring &value) override {
    LOG(LM_INFO, "value " << mobs::to_string(value));
  }

  virtual void StartTag(const std::string &element) override {
    LOG(LM_INFO, "start " << element);
    if (element == "Person") {
        person.name("XXX");
        fill(&person);
    }
  }

  virtual void EndTag(const std::string &element) override {
    LOG(LM_INFO, "end " << element);
  }

  void filled(mobs::ObjectBase *obj, const string &error) override {
    LOG(LM_INFO, "filled " << obj->to_string() << (error.empty() ? " OK" : " ERROR = ") << error);
  }

  void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, mobs::CryptBufBase *&cryptBufp) override {
      cryptBufp = new mobs::CryptBufNone();
  }

  void EncryptionFinished() override {

  }
};


}


TEST(streamBufferTest, charsetStrUtf8) {
  Person p;
  p.name("€Mähr");
  stringstream strOut;
  mobs::CryptOstrBuf streambufO(strOut, new mobs::CryptBufNone);
  std::wostream x2out(&streambufO);
  x2out.exceptions(std::wostream::failbit | std::wostream::badbit);

  mobs::XmlWriter xf(x2out, mobs::XmlWriter::CS_utf8, false);

  mobs::XmlOut xo(&xf, mobs::ConvObjToString().exportXml());

  xf.writeHead();
  xf.writeTagBegin(L"root");
  p.traverse(xo);
  xf.writeTagEnd();
  xf.sync();
  EXPECT_EQ(u8"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><root><Person><name>€Mähr</name></Person></root>",
            strOut.str());

  stringstream strIn(strOut.str());
  mobs::CryptIstrBuf streambufI(strIn, new mobs::CryptBufNone);
  std::wistream x2in(&streambufI);
  XmlInput xr(x2in);
  EXPECT_NO_THROW(xr.parse());
  EXPECT_EQ(u8"€Mähr", person.name());

}

TEST(streamBufferTest, charsetStrIso) {
  Person p;
  p.name("€Mähr");
  stringstream strOut;
  mobs::CryptOstrBuf streambufO(strOut, new mobs::CryptBufNone);
  //std::locale lo0 = std::locale(socket.getloc(), new mobs::codec_iso8859_1);
  //socket.imbue(lo0);

  std::wostream x2out(&streambufO);
  x2out.exceptions(std::wostream::failbit | std::wostream::badbit);

  mobs::XmlWriter xf(x2out, mobs::XmlWriter::CS_iso8859_15, false);

  mobs::XmlOut xo(&xf, mobs::ConvObjToString().exportXml());

  xf.writeHead();
  xf.writeTagBegin(L"root");
  p.traverse(xo);
  xf.writeTagEnd();
  xf.sync();
  EXPECT_EQ(
          u8"<?xml version=\"1.0\" encoding=\"ISO-8859-15\" standalone=\"yes\"?><root><Person><name>\xa4M\xE4hr</name></Person></root>",
          strOut.str());

  stringstream strIn(strOut.str());
  mobs::CryptIstrBuf streambufI(strIn, new mobs::CryptBufNone);
  std::wistream x2in(&streambufI);
  XmlInput xr(x2in);
  EXPECT_NO_THROW(xr.parse());
  EXPECT_EQ(u8"€Mähr", person.name());


}


TEST(streamBufferTest, charsetStrLang) {
  auto ltx = R"(1. Auf de schwäbsche Eisebahne
gibt´s gar viele Haltstatione,
Schtuegert, Ulm und Biberach, Meckebeure, Durlesbach.
Rulla, rulla, rulllala, rulla, rulla, rulllala,
Schtuegert, Ulm und Biberach, Meckebeure, Durlesbach.

2. Auf de schwäbsche Eisebahne
gibt es viele Restauratione,
wo ma esse, trinke ka,
alles, was de Magen ma...

3. Auf de schwäbsche Eisebahne
braucht ma keine Postillione,
was uns sonst das Posthorn blies,
pfeifet jetzt die Lokomotiv...

4. Auf de schwäbsche Eisebahne
wollt´ amal a Bäurle fahre,
geht an Schalter lupft de Hut:
"Oi Billetle, seid so gut !"...

5. Eine Geiß hat er sich kaufet
und dass sie ihm nit entlaufet,
bindet sie de gute Ma hinte an de Wage a...

6. "Böckli, tu nur woidle springe,
`s Futter wird i dir schon bringe."
Setzt sich zu seimn Weibl na
und brennts Tabackspfeifle a...

7. Auf de nächste Statione,
wo er will sein Böckle hole,
findt er nur noch Kopf und Soil
an dem hintre Wagentoil...

8. Da kriegt er en große Zorne,
nimmt den Kopf mitsamt dem Horne,
schmeißt en, was er schmeiße ka,
dem Konduktör an Schädel na...

9. "So, du kannst den Schade zahle,
warum bischt so schnell gefahre!
Du allein bischt Schuld daran,
dass i d´Gois verlaure ha !"

10. So, jetzt wär das Lied gesunge,
hot´s euch in de Ohre klunge?
Wer´s noch net begreife ka,
fangs nomal von vorne a...
)";

  Person p;
  p.name(ltx);
  stringstream strOut;
  mobs::CryptOstrBuf streambufO(strOut);
  //std::locale lo0 = std::locale(socket.getloc(), new mobs::codec_iso8859_1);
  //socket.imbue(lo0);

  std::wostream x2out(&streambufO);
  x2out.exceptions(std::wostream::failbit | std::wostream::badbit);

  mobs::XmlWriter xf(x2out, mobs::XmlWriter::CS_utf8, true);

  xf.writeHead();
  xf.writeTagBegin(L"methodCall");

  xf.startEncrypt(new mobs::CryptBufNone());

  mobs::XmlOut xo(&xf, mobs::ConvObjToString().exportXml());
  ASSERT_NO_THROW(p.traverse(xo));
  xf.stopEncrypt();
  xf.writeTagEnd();
  cerr << strOut.str() << endl;
  cerr << u8"DONE äöü" << endl;

  stringstream strIn(strOut.str());
  mobs::CryptIstrBuf streambufI(strIn);
  std::wistream x2in(&streambufI);
  XmlInput xr(x2in);
  EXPECT_NO_THROW(xr.parse());


  EXPECT_STREQ(ltx, person.name().c_str());

}

TEST(streamBufferTest, codecKill) {
  std::locale lo = std::locale(std::locale(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
  auto input = "Mümmelmännchen€\200\01\02\03€Otto";
  auto inputEnd = input + strlen(input);
  std::mbstate_t state{};
  const char *bp2;
  std::array<wchar_t, 256> result;
  wchar_t *bit2;
  std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(lo).in(state, input, inputEnd, bp2,
                     &result[0], &result[result.size()], bit2);
  //EXPECT_EQ(bp2, input + strlen(input));
  EXPECT_EQ('\200', *bp2);
  EXPECT_EQ(wstring(L"Mümmelmännchen€"), wstring(&result[0], std::distance(&result[0], bit2)));
  auto s2 = bp2 +4;
  std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(lo).in(state, s2, inputEnd, bp2,
                                                                     &result[0], &result[result.size()], bit2);
  EXPECT_EQ(bp2, inputEnd);
  EXPECT_EQ(wstring(L"€Otto"), wstring(&result[0], std::distance(&result[0], bit2)));

}


TEST(streamBufferTest, dataInUTF8) {
  std::locale lo = std::locale(std::locale(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
  std::locale lo1 = std::locale(std::locale(), new mobs::codec_iso8859_1);

  stringstream strOut("Mümmelmännchen€\200\01\02\03€Otto\200\04\05\06€ßß");

  stringstream strIn(strOut.str());
  mobs::CryptIstrBuf streambufI(strIn, new mobs::CryptBufNone);
  std::wistream x2in(&streambufI);
  x2in.imbue(lo);

  std::array<wchar_t, 1024> buf;
  auto sz = x2in.readsome(&buf[0], buf.size());
  ASSERT_GT(sz, 0);
  EXPECT_EQ(std::wstring(L"Mümmelmännchen€"), std::wstring(&buf[0], sz));
  EXPECT_TRUE(x2in.eof());

  x2in.clear();
  EXPECT_TRUE(x2in.good());
  x2in.imbue(lo1);
  wchar_t ch;
  ASSERT_FALSE(x2in.get(ch).bad());
  EXPECT_EQ(L'\200', ch);
  ASSERT_FALSE(x2in.get(ch).bad());
  EXPECT_EQ(L'\01', ch);
  ASSERT_FALSE(x2in.get(ch).bad());
  EXPECT_EQ(L'\02', ch);
  ASSERT_FALSE(x2in.get(ch).bad());
  EXPECT_EQ(L'\03', ch);
  EXPECT_TRUE(x2in.good());

  x2in.imbue(lo);
  EXPECT_TRUE(x2in.good());

  sz = x2in.readsome(&buf[0], buf.size());
  ASSERT_GT(sz, 0);
  EXPECT_EQ(std::wstring(L"€Otto"), std::wstring(&buf[0], sz));
  EXPECT_FALSE(x2in.eof());

  x2in.clear();
  EXPECT_TRUE(x2in.good());
  x2in.imbue(lo1);
  ASSERT_FALSE(x2in.get(ch).bad());
  EXPECT_EQ(L'\200', ch);
  ASSERT_FALSE(x2in.get(ch).bad());
  EXPECT_EQ(L'\04', ch);
  ASSERT_FALSE(x2in.get(ch).bad());
  EXPECT_EQ(L'\05', ch);
  ASSERT_FALSE(x2in.get(ch).bad());
  EXPECT_EQ(L'\06', ch);
  EXPECT_TRUE(x2in.good());
  x2in.imbue(lo);
  EXPECT_TRUE(x2in.good());

  sz = x2in.readsome(&buf[0], buf.size());
  ASSERT_GT(sz, 0);
  EXPECT_EQ(std::wstring(L"€ßß"), std::wstring(&buf[0], sz));
  EXPECT_FALSE(x2in.eof());

  EXPECT_TRUE(x2in.get(ch).eof());

}


TEST(streamBufferTest, dataStreamInUTF8) {
  stringstream strOut("Mümmelmännchen€\200\01\02\03€Otto\200\04\05\06€ßß");

  stringstream strIn(strOut.str());
  mobs::CryptIstrBuf streambufI(strIn, new mobs::CryptBufNone);
  std::wistream x2in(&streambufI);
  std::locale lo = std::locale(std::locale(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
  x2in.imbue(lo);

  std::array<wchar_t, 1024> buf;
  auto sz = x2in.readsome(&buf[0], buf.size());
  ASSERT_GT(sz, 0);
  EXPECT_EQ(std::wstring(L"Mümmelmännchen€"), std::wstring(&buf[0], sz));
  EXPECT_TRUE(x2in.eof());

  x2in.clear();
  EXPECT_TRUE(x2in.good());

  mobs::BinaryIstBuf binBuf1(streambufI, 4);
  istream bin1(&binBuf1);
  EXPECT_EQ(4, bin1.rdbuf()->in_avail());

  char ch;
  {
    ASSERT_FALSE(bin1.get(ch).bad());
    EXPECT_EQ('\200', ch);
    ASSERT_FALSE(bin1.get(ch).bad());
    EXPECT_EQ('\01', ch);
    ASSERT_FALSE(bin1.get(ch).bad());
    EXPECT_EQ('\02', ch);
    ASSERT_FALSE(bin1.get(ch).bad());
    EXPECT_EQ('\03', ch);
    EXPECT_TRUE(bin1.good());
    EXPECT_TRUE(bin1.get(ch).eof());
  }
  EXPECT_TRUE(x2in.good());

  sz = x2in.readsome(&buf[0], buf.size());
  ASSERT_GT(sz, 0);
  EXPECT_EQ(std::wstring(L"€Otto"), std::wstring(&buf[0], sz));
  EXPECT_FALSE(x2in.eof());

  x2in.clear();
  EXPECT_TRUE(x2in.good());

  mobs::BinaryIstBuf binBuf2(streambufI, 4);
  istream bin2(&binBuf2);
  EXPECT_LT(0, bin2.rdbuf()->in_avail());

  ASSERT_FALSE(bin2.get(ch).bad());
  EXPECT_EQ('\200', ch);
  ASSERT_FALSE(bin2.get(ch).bad());
  EXPECT_EQ(L'\04', ch);
  ASSERT_FALSE(bin2.get(ch).bad());
  EXPECT_EQ(L'\05', ch);
  ASSERT_FALSE(bin2.get(ch).bad());
  EXPECT_EQ(L'\06', ch);
  EXPECT_TRUE(bin2.good());
  EXPECT_TRUE(bin2.get(ch).eof());


  EXPECT_TRUE(x2in.good());

  sz = x2in.readsome(&buf[0], buf.size());
  ASSERT_GT(sz, 0);
  EXPECT_EQ(std::wstring(L"€ßß"), std::wstring(&buf[0], sz));
  wchar_t wch;
  EXPECT_TRUE(x2in.get(wch).eof());





}

}
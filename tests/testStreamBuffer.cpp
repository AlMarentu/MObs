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

TEST(IstreamBufferTest, B64BufferTest) {
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

TEST(IstreamBufferTest, NBufferTest) {
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

TEST(IstreamBufferTest, NBufferTest64) {
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


}
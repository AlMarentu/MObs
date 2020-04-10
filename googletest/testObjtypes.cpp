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


#include "objtypes.h"
#include "objtypes.h"


#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>


namespace {

TEST(objtypeTest, to_string32) {
  EXPECT_EQ(L"Test \n öäü xx\u20186tt", mobs::to_wstring(u8"Test \n öäü xx\u20186tt"));
  EXPECT_EQ(U"Test \n öäü xx\U000020186tt", mobs::to_u32string(u8"Test \n öäü xx\u20186tt"));
  EXPECT_EQ(U"Test \n öäü xx\U000020186tt", mobs::to_u32string(u8"Test \n öäü xx\u20186tt"));
}

TEST(objtypeTest, to_string) {
  EXPECT_EQ(u8"Test \n öäü xx\u20186tt", mobs::to_string(L"Test \n öäü xx\u20186tt"));
  EXPECT_EQ(u8"Test \n öäü xx\u20186tt", mobs::to_string(U"Test \n öäü xx\U000020186tt"));
  EXPECT_EQ(u8"Test \n öäü xx\u20186tt", mobs::to_string(u"Test \n öäü xx\u20186tt"));
  EXPECT_EQ(u8"Test \n öäü xx\u20186tt", mobs::to_string(u8"Test \n öäü xx\u20186tt"));
  EXPECT_EQ(u8"true", mobs::to_string(true));
  EXPECT_EQ(u8"false", mobs::to_string(false));
  EXPECT_EQ(u8"123", mobs::to_string(123));
  EXPECT_EQ(u8"12.6", mobs::to_string(12.6));
  EXPECT_EQ(u8"Z", mobs::to_string('Z'));
  EXPECT_EQ(u8"ä", mobs::to_string(u'\u00e4'));
  EXPECT_EQ(u8"ä", mobs::to_string(U'\U000000e4'));

}

TEST(objtypeTest, string2x) {

  char32_t c32;
  ASSERT_TRUE(mobs::string2x(u8"a", c32));
  EXPECT_EQ(U'a', c32);
  EXPECT_FALSE(mobs::string2x(u8"ab", c32));
  ASSERT_TRUE(mobs::string2x(u8"", c32));
  EXPECT_EQ(U'\U00000000', c32);

  char c;
  ASSERT_FALSE(mobs::string2x(u8"\u2018", c));
  ASSERT_TRUE(mobs::string2x(u8"a", c));
  EXPECT_EQ('a', c);
  ASSERT_TRUE(mobs::string2x(u8"ö", c));
  EXPECT_EQ(char(246), c);

  wchar_t w;
  ASSERT_TRUE(mobs::string2x(u8"ü", w));
  EXPECT_EQ(wchar_t(252), w);

  ASSERT_TRUE(mobs::string2x(u8"\u2018", w));
  EXPECT_EQ(L'\u2018', w);
  
  int i;
  ASSERT_TRUE(mobs::string2x(u8"12345", i));
  EXPECT_EQ(12345, i);
  ASSERT_TRUE(mobs::string2x(u8"-998", i));
  EXPECT_EQ(-998, i);
  EXPECT_EQ(-998, i);
  ASSERT_TRUE(mobs::string2x(u8" 30000", i));
  EXPECT_EQ(30000, i);
  ASSERT_TRUE(mobs::string2x(u8"+99", i));
  EXPECT_EQ(99, i);

  EXPECT_FALSE(mobs::string2x(u8"998 ", i));
  EXPECT_FALSE(mobs::string2x(u8"- 998", i));
  EXPECT_FALSE(mobs::string2x(u8"+ 998", i));
  EXPECT_FALSE(mobs::string2x(u8"9a98", i));
  EXPECT_FALSE(mobs::string2x(u8"3.14", i));
  EXPECT_FALSE(mobs::string2x(u8"30000000000", i));
  EXPECT_FALSE(mobs::string2x(u8"", i));
  
  unsigned short int usi;
  ASSERT_TRUE(mobs::string2x(u8"12345", usi));
  EXPECT_EQ(12345, usi);
  EXPECT_FALSE(mobs::string2x(u8"-998 ", usi));
  
  float f;
  ASSERT_TRUE(mobs::string2x(u8"12.345", f));
  EXPECT_FLOAT_EQ(12.345, f);
  EXPECT_FALSE(mobs::string2x(u8"9,98", f));

  double d;
  ASSERT_TRUE(mobs::string2x(u8"-12.345", d));
  EXPECT_DOUBLE_EQ(-12.345, d);
  EXPECT_FALSE(mobs::string2x(u8"9,98", d));
  
  bool b;
  ASSERT_TRUE(mobs::string2x(u8"true", b));
  EXPECT_TRUE(b);
  ASSERT_TRUE(mobs::string2x(u8"false", b));
  EXPECT_FALSE(b);

  EXPECT_FALSE(mobs::string2x(u8"1", b));

  unsigned char uc;
  ASSERT_FALSE(mobs::string2x(u8"\u2018", uc));
  ASSERT_TRUE(mobs::string2x(u8"a", uc));
  EXPECT_EQ('a', uc);
  ASSERT_TRUE(mobs::string2x(u8"ö", uc));
  EXPECT_EQ((unsigned char)(246), uc);

  signed char sc;
  ASSERT_FALSE(mobs::string2x(u8"\u2018", sc));
  ASSERT_TRUE(mobs::string2x(u8"a", sc));
  EXPECT_EQ('a', sc);
  ASSERT_TRUE(mobs::string2x(u8"ö", sc));
  EXPECT_EQ((signed char)(246), sc);

  std::string s;
  ASSERT_TRUE(mobs::string2x(u8" Hallo ä0ß \u2020 +", s));
  EXPECT_EQ(u8" Hallo ä0ß \u2020 +", s);
  ASSERT_TRUE(mobs::string2x(u8"", s));
  EXPECT_EQ(u8"", s);

  std::u32string s32;
  ASSERT_TRUE(mobs::string2x(u8" Hallo ä0ß \u2020 +", s32));
  EXPECT_EQ(U" Hallo ä0ß \U00002020 +", s32);
  ASSERT_TRUE(mobs::string2x(u8"", s32));
  EXPECT_EQ(U"", s32);

  std::u16string s16;
  ASSERT_TRUE(mobs::string2x(u8" Hallo ä0ß \u2020 +", s16));
  EXPECT_EQ(u" Hallo ä0ß \u2020 +", s16);
  ASSERT_TRUE(mobs::string2x(u8"", s16));
  EXPECT_EQ(u"", s16);

  std::wstring ws;
  ASSERT_TRUE(mobs::string2x(u8" Hallo ä0ß \u2020 +", ws));
  EXPECT_EQ(L" Hallo ä0ß \u2020 +", ws);
  ASSERT_TRUE(mobs::string2x(u8"", ws));
  EXPECT_EQ(L"", ws);


}

MOBS_ENUM_DEF(direction, Dleft, Dright, Dup, Ddown)
MOBS_ENUM_VAL(direction, "left", "right", "up", "down")

mobs::StrdirectionConv c;

TEST(objtypeTest, mobsenum) {
  EXPECT_EQ("left", mobs::direction_to_string(Dleft));
  EXPECT_EQ("up", mobs::direction_to_string(Dup));
  enum direction d;
  ASSERT_TRUE(mobs::string_to_direction("right", d));
  EXPECT_EQ(Dright, d);
  ASSERT_TRUE(mobs::string_to_direction("down", d));
  EXPECT_EQ(Ddown, d);
  EXPECT_FALSE(mobs::string_to_direction("middle", d));
}

TEST(objtypeTest, mobsconv) {
  ::mobs::ConvToStrHint cthf = ::mobs::ConvToStrHint(false);
  ::mobs::ConvToStrHint ctht = ::mobs::ConvToStrHint(true);
  EXPECT_EQ("left", c.c_to_string(Dleft,cthf));
  EXPECT_EQ("up", c.c_to_string(Dup, cthf));
  EXPECT_EQ(std::to_string(int(Dleft)), c.c_to_string(Dleft,ctht));
  EXPECT_EQ(std::to_string(int(Dup)), c.c_to_string(Dup, ctht));
  enum direction d;

  ASSERT_TRUE(c.c_string2x("right", d, ::mobs::ConvFromStrHint::convFromStrHintDflt));
  EXPECT_EQ(Dright, d);
  ASSERT_TRUE(c.c_string2x("down", d, ::mobs::ConvFromStrHint::convFromStrHintDflt));
  EXPECT_EQ(Ddown, d);
  EXPECT_FALSE(c.c_string2x("middle", d, ::mobs::ConvFromStrHint::convFromStrHintDflt));
}


MOBS_ENUM_DEF(direction2, D2left, D2right, D2up, D2down, D2void)
MOBS_ENUM_VAL(direction2, "left", "right", "up", "down")

TEST(objtypeTest, mobsenum2) {
  EXPECT_ANY_THROW(mobs::direction2_to_string(D2void));
}

MOBS_ENUM_DEF(direction3, D3left, D3right, D3up, D3down)
MOBS_ENUM_VAL(direction3, "left", "right", "up", "down", "void")

TEST(objtypeTest, mobsenum3) {
  enum direction3 d;
  EXPECT_ANY_THROW(mobs::string_to_direction3("void", d));
  EXPECT_FALSE(mobs::string_to_direction3("middle", d));
}

}


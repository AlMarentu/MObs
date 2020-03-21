//
//  testObjgen.cpp
//  mobs
//
//  Created by Matthias Lautner on 19.03.20.
//  Copyright © 2020 Matthias Lautner. All rights reserved.
//

#include "objtypes.h"
#include "objtypes.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;
using namespace mobs;


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
  ASSERT_TRUE(string2x(u8"a", c32));
  EXPECT_EQ(U'a', c32);
  EXPECT_FALSE(string2x(u8"ab", c32));
  ASSERT_TRUE(string2x(u8"", c32));
  EXPECT_EQ(U'\U00000000', c32);

  char c;
  ASSERT_FALSE(string2x(u8"\u2018", c));
  ASSERT_TRUE(string2x(u8"a", c));
  EXPECT_EQ('a', c);
  ASSERT_TRUE(string2x(u8"ö", c));
  EXPECT_EQ(char(246), c);

  wchar_t w;
  ASSERT_TRUE(string2x(u8"ü", w));
  EXPECT_EQ(wchar_t(252), w);

  ASSERT_TRUE(string2x(u8"\u2018", w));
  EXPECT_EQ(L'\u2018', w);
  
  int i;
  ASSERT_TRUE(string2x(u8"12345", i));
  EXPECT_EQ(12345, i);
  ASSERT_TRUE(string2x(u8"-998", i));
  EXPECT_EQ(-998, i);
  EXPECT_EQ(-998, i);
  ASSERT_TRUE(string2x(u8" 30000", i));
  EXPECT_EQ(30000, i);
  ASSERT_TRUE(string2x(u8"+99", i));
  EXPECT_EQ(99, i);

  EXPECT_FALSE(string2x(u8"998 ", i));
  EXPECT_FALSE(string2x(u8"- 998", i));
  EXPECT_FALSE(string2x(u8"+ 998", i));
  EXPECT_FALSE(string2x(u8"9a98", i));
  EXPECT_FALSE(string2x(u8"3.14", i));
  EXPECT_FALSE(string2x(u8"30000000000", i));
  EXPECT_FALSE(string2x(u8"", i));
  
  unsigned short int usi;
  ASSERT_TRUE(string2x(u8"12345", usi));
  EXPECT_EQ(12345, usi);
  EXPECT_FALSE(string2x(u8"-998 ", usi));
  
  float f;
  ASSERT_TRUE(string2x(u8"12.345", f));
  EXPECT_FLOAT_EQ(12.345, f);
  EXPECT_FALSE(string2x(u8"9,98", f));

  double d;
  ASSERT_TRUE(string2x(u8"-12.345", d));
  EXPECT_DOUBLE_EQ(-12.345, d);
  EXPECT_FALSE(string2x(u8"9,98", d));
  
  bool b;
  ASSERT_TRUE(string2x(u8"true", b));
  EXPECT_TRUE(b);
  ASSERT_TRUE(string2x(u8"false", b));
  EXPECT_FALSE(b);

  EXPECT_FALSE(string2x(u8"1", b));

  unsigned char uc;
  ASSERT_FALSE(string2x(u8"\u2018", uc));
  ASSERT_TRUE(string2x(u8"a", uc));
  EXPECT_EQ('a', uc);
  ASSERT_TRUE(string2x(u8"ö", uc));
  EXPECT_EQ((unsigned char)(246), uc);

  signed char sc;
  ASSERT_FALSE(string2x(u8"\u2018", sc));
  ASSERT_TRUE(string2x(u8"a", sc));
  EXPECT_EQ('a', sc);
  ASSERT_TRUE(string2x(u8"ö", sc));
  EXPECT_EQ((signed char)(246), sc);

  string s;
  ASSERT_TRUE(string2x(u8" Hallo ä0ß \u2020 +", s));
  EXPECT_EQ(u8" Hallo ä0ß \u2020 +", s);
  ASSERT_TRUE(string2x(u8"", s));
  EXPECT_EQ(u8"", s);

  u32string s32;
  ASSERT_TRUE(string2x(u8" Hallo ä0ß \u2020 +", s32));
  EXPECT_EQ(U" Hallo ä0ß \U00002020 +", s32);
  ASSERT_TRUE(string2x(u8"", s32));
  EXPECT_EQ(U"", s32);

  u16string s16;
  ASSERT_TRUE(string2x(u8" Hallo ä0ß \u2020 +", s16));
  EXPECT_EQ(u" Hallo ä0ß \u2020 +", s16);
  ASSERT_TRUE(string2x(u8"", s16));
  EXPECT_EQ(u"", s16);

  wstring ws;
  ASSERT_TRUE(string2x(u8" Hallo ä0ß \u2020 +", ws));
  EXPECT_EQ(L" Hallo ä0ß \u2020 +", ws);
  ASSERT_TRUE(string2x(u8"", ws));
  EXPECT_EQ(L"", ws);


}

}


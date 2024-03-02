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


#include "mchrono.h"
#include "mchrono.h"

#include "objgen.h"
#include "objgen.h"


#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>



using namespace std;
using namespace mobs;


namespace {



TEST(mchronoTest, mtime) {
  MTime t;
  EXPECT_EQ("1970-01-01T01:00:00+01:00", to_string(t));
  EXPECT_EQ("1970-01-01T01:00:00.000000+01:00", to_string_iso8601(t));
  EXPECT_EQ("1970-01-01 01:00:00.000000", to_string_ansi(t));
  EXPECT_EQ("1970-01-01T00:00:00.000000Z", to_string_gmt(t));
  EXPECT_TRUE(string2x("2020-03-30T15:30:55.123456Z", t));
  EXPECT_EQ("2020-03-30T15:30:55.123456Z", to_string_gmt(t));
  EXPECT_TRUE(string2x("2020-03-30T15:30:55.123456+00:00", t));
  EXPECT_EQ("2020-03-30T15:30:55.123456Z", to_string_gmt(t));
  EXPECT_TRUE(string2x("2020-03-30T15:30:55.123456-0000", t));
  EXPECT_EQ("2020-03-30T15:30:55.123456Z", to_string_gmt(t));
  EXPECT_TRUE(string2x("2020-03-30T15:30:55.123456+00", t));
  EXPECT_EQ("2020-03-30T15:30:55.123456Z", to_string_gmt(t));

  EXPECT_TRUE(string2x("2020-12-31T15:30:55.123Z", t));
  EXPECT_EQ("2020-12-31T15:30:55.123000Z", to_string_gmt(t));
  EXPECT_EQ("2020-12-31T16:30:55.123+01:00", to_string(t));
  EXPECT_TRUE(string2x("2020-12-31T15:30:55Z", t));
  EXPECT_EQ("2020-12-31T15:30:55.000000Z", to_string_gmt(t));
  EXPECT_EQ("2020-12-31T16:30:55+01:00", to_string(t));
  EXPECT_TRUE(string2x("2020-12-31T15:30:55Z", t));
  EXPECT_EQ("2020-12-31T15:30:55.000000Z", to_string_gmt(t));
  EXPECT_EQ("2020-12-31T16:30:55+01:00", to_string(t));

  EXPECT_TRUE(string2x("2020-12-31T15:30Z", t));
  EXPECT_EQ("2020-12-31T15:30:00.000000Z", to_string_gmt(t));
  EXPECT_TRUE(string2x("2020-1-1T1:3:5Z", t));
  EXPECT_EQ("2020-01-01T01:03:05.000000Z", to_string_gmt(t));
  EXPECT_EQ("2020-01-01T02:03:05.000000+01:00", to_string_iso8601(t));
  // tz = europe/berlin
  EXPECT_TRUE(string2x("1999-12-31T15:30", t));
  EXPECT_EQ("1999-12-31T14:30:00.000000Z", to_string_gmt(t));

  EXPECT_TRUE(string2x("1900-12-31T15:30:00.654321Z", t));
  EXPECT_EQ("1900-12-31T15:30:00.654321Z", to_string_gmt(t));
  EXPECT_EQ("1900-12-31T15:30:00.654321Z", to_string_gmt(t, mobs::MF6));
  EXPECT_EQ("1900-12-31T15:30:00.654Z", to_string_gmt(t, mobs::MF3));
  EXPECT_EQ("1900-12-31T15:30:00.6Z", to_string_gmt(t, mobs::MF1));
  EXPECT_EQ("1900-12-31T15:30:00Z", to_string_gmt(t, mobs::MSecond));
  EXPECT_EQ("1900-12-31T15:30Z", to_string_gmt(t, mobs::MMinute));
  EXPECT_EQ("1900-12-31T15Z", to_string_gmt(t, mobs::MHour));
  EXPECT_EQ("1900-12-31", to_string_gmt(t, mobs::MDay));
  EXPECT_EQ("1900-12", to_string_gmt(t, mobs::MMonth));
  EXPECT_EQ("1900", to_string_gmt(t, mobs::MYear));

//  EXPECT_TRUE(string2x("1583-01-01T15:30:00Z", t));
//  EXPECT_EQ("1583-01-01T15:30:00Z", to_string_gmt(t));


}

TEST(mchronoTest, mtime_int) {
  MTime t;

  EXPECT_TRUE(mobs::from_number(int64_t (9223372036854775), t));
  EXPECT_EQ("2262-04-11T23:47:16.854775Z", to_string_gmt(t));
  EXPECT_TRUE(mobs::from_number(int64_t (-12521191039804), t));
  EXPECT_EQ("1969-08-09T01:53:28.960196Z", to_string_gmt(t));
  EXPECT_TRUE(mobs::from_number(int64_t (-2521191039804000), t));
  EXPECT_EQ("1890-02-08T13:09:20.196000Z", to_string_gmt(t));

  EXPECT_NO_THROW(t = mobs::MTimeNow());
  LOG(LM_INFO, "NOW " << mobs::to_string(t));

}

TEST(mchronoTest, mdate) {
  mobs::MDate d{};
  int64_t i{};

  EXPECT_TRUE(mobs::string2x("1970-01-05", d));
  EXPECT_TRUE(mobs::to_int64(d, i));
  EXPECT_EQ(4, i);
  i = 31;
  EXPECT_TRUE(mobs::from_number(i, d));
  EXPECT_EQ("1970-02-01T00Z", to_string_gmt(d, MTimeFract::MHour));
  EXPECT_EQ("1970-02-01", mobs::to_string(d));

  EXPECT_TRUE(mobs::from_number(int64_t (-25000), d));
  EXPECT_EQ("1901-07-22", to_string(d));


  EXPECT_NO_THROW(d = mobs::MDateNow());
  LOG(LM_INFO, "NOW " << mobs::to_string(d));
}

TEST(mchronoTest, delta) {
  MTime t1, t2;
  auto vorher = MTimeNow();
  EXPECT_TRUE(string2x("2020-03-30T15:30:55.123456Z", t1));
  EXPECT_TRUE(string2x("2020-03-30T15:30:55.123456Z", t2));
  EXPECT_EQ(0, (t1 - t2).count());
  EXPECT_TRUE(string2x("2020-03-30T15:30:55.123457Z", t2));
  EXPECT_EQ(-1, (t1 - t2).count());
  EXPECT_TRUE(string2x("2020-03-30T15:30:55.123455Z", t2));
  EXPECT_EQ(1, (t1 - t2).count());
  EXPECT_TRUE(string2x("2020-03-30T15:30:54.123456Z", t2));
  EXPECT_EQ(1000000, (t1 - t2).count());
  EXPECT_TRUE(string2x("2020-03-30T15:29:55.123456Z", t2));
  EXPECT_EQ(60000000, (t1 - t2).count());
  EXPECT_EQ(60000, std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t2).count());
  EXPECT_EQ(60, std::chrono::duration_cast<std::chrono::seconds>(t1 - t2).count());
  EXPECT_LT(5, (MTimeNow() - vorher).count()); // Sollte immer mindestens 5 microsekunden dauern
}

}


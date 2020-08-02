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

#include "unixtime.h"
#include "unixtime.h"

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


TEST(dateTimeTest, einAusgabe) {
  UxTime t(1095379198);
  EXPECT_EQ("2004-09-17T01:59:58+02:00", t.toISO8601());
  UxTime t2("1999-01-01T00:00:01+00:00");
  EXPECT_EQ(915148801, t2.toUxTime());
  UxTime t3("1999-01-01T00:00:01-01:00");
  EXPECT_EQ(915152401, t3.toUxTime());
  UxTime t4(2004, 9, 17, 1, 59, 58);
  EXPECT_EQ(1095379198, t4.toUxTime());

  EXPECT_ANY_THROW(UxTime("AAA"));

  UxTime t6(3402, 9, 17, 1, 59, 58);
  EXPECT_EQ(45211910398, t6.toUxTime());
  EXPECT_EQ("3402-09-17T01:59:58+02:00", t6.toISO8601());

  EXPECT_NO_THROW(t = UxTime("2020-07-03T14:12:55+02:00"));
  EXPECT_EQ("2020-07-03T14:12:55+02:00", t.toISO8601());

}

TEST(dateTimeTest, string2x) {
    UxTime t;
    EXPECT_TRUE(string2x("2020-02-02T14:22:02+02:00", t));
    EXPECT_FALSE(string2x("2ZZZ2T14:22:02+02:00", t));
}

TEST(dateTimeTest, timeBeforeEpochIsInvalid) {
  UxTime t5(1802, 9, 17, 1, 59, 58);
  EXPECT_EQ(-1, t5.toUxTime());
}

class TimeStamp : virtual public mobs::ObjectBase {
public:
  ObjInit(TimeStamp);
  
  MemVar(time_t, time);
  MemVar(mobs::UxTime, dtime);
  MemVar(std::string, name);
};
ObjRegister(TimeStamp);


TEST(dateTimeTest, mobs) {
  TimeStamp t;
  t.time(mobs::UxTime("2019-12-24T16:01:00+01:00").toUxTime());
  t.dtime(mobs::UxTime("2019-12-24T16:01:00+01:00"));
  t.name(u8"me");
  EXPECT_EQ(u8"{time:1577199660,dtime:\"2019-12-24T16:01:00+01:00\",name:\"me\"}", to_string(t));
  
}


TEST(dateTimeTest, mtime) {
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


  cerr << to_string_ansi(std::chrono::system_clock::now());

}

TEST(dateTimeTest, mtime_int) {
  MTime t;
  EXPECT_TRUE(mobs::from_number(int64_t (45211910398000345), t));
  EXPECT_EQ("3402-09-16T23:59:58.000345Z", to_string_gmt(t));
  EXPECT_TRUE(mobs::from_number(int64_t (-12521191039804), t));
  EXPECT_EQ("1969-08-09T01:53:28.960196Z", to_string_gmt(t));
  EXPECT_TRUE(mobs::from_number(int64_t (-2521191039804000), t));
  EXPECT_EQ("1890-02-08T13:09:20.196000Z", to_string_gmt(t));

}


}


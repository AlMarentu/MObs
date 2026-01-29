// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2026 Matthias Lautner
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

#include "objgen.h"
#include "objgen.h"


#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>



using namespace std;
using namespace mobs;

// Die Tests benötigen die Timezone Europe/Berlin

namespace {


TEST(dateTimeTest, basics) {
  EXPECT_TRUE(UxTime(5) < UxTime(7));
  EXPECT_TRUE(UxTime(8) > UxTime(7));
  EXPECT_TRUE(UxTime(5) != UxTime(7));
  EXPECT_TRUE(UxTime(5) == UxTime(5));
}

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

  UxTime t6(2988, 9, 17, 1, 59, 58);
  EXPECT_EQ(32147452798, t6.toUxTime());
  EXPECT_EQ("2988-09-17T01:59:58+02:00", t6.toISO8601());

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
#if GTEST_OS_LINUX
  EXPECT_EQ(-5279208810, t5.toUxTime());
#else
  EXPECT_EQ(-1, t5.toUxTime());
#endif
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





}


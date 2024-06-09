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


#include "objcache.h"
#include "objcache.h"
#include "objgen.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;


namespace {

class Person : virtual public mobs::ObjectBase {
public:
  ObjInit(Person);

  MemVar(int, kundennr, KEYELEMENT1);
  MemVar(std::string, name);
  MemVar(std::string, vorname);
};

class KFZ : virtual public mobs::ObjectBase {
public:
  ObjInit(KFZ);

  MemVar(std::string, kennzeichen, KEYELEMENT1);
  MemVar(std::string, hersteller);
  MemVar(std::string, typ);
};

TEST(cacheTest, simple) {
  Person a, b, c;
  KFZ x, y;
  a.kundennr(333);
  a.name("Müller");
  a.vorname("Peter");
  b.kundennr(444);
  b.name("Huber");
  b.vorname("Anneliese");
  x.kennzeichen("X-12345");
  x.hersteller("VW");
  x.typ("Käfer");
  mobs::ObjCache cache;
  EXPECT_NO_THROW(cache.save(a));
  EXPECT_NO_THROW(cache.save(b));
  EXPECT_NO_THROW(cache.save(x));
  c.kundennr(222);
  EXPECT_FALSE(cache.exists(c));
  c.kundennr(333);
  EXPECT_TRUE(cache.exists(c));
  c.kundennr(444);
  EXPECT_TRUE(cache.exists(c));
  EXPECT_EQ(3, cache.reduce(INT_MAX));


  c.kundennr(222);
  EXPECT_FALSE(cache.load(c));
  c.kundennr(333);
  EXPECT_TRUE(cache.load(c));
  EXPECT_EQ("Peter", c.vorname());
  c.kundennr(444);
  EXPECT_TRUE(cache.load(c));
  EXPECT_EQ("Anneliese", c.vorname());
  y.kennzeichen("X-12345");
  EXPECT_TRUE(cache.load(y));
  EXPECT_EQ("Käfer", y.typ());

  EXPECT_EQ(3, cache.reduce(INT_MAX));
  auto k = std::unique_ptr<KFZ>(new KFZ);
  k->kennzeichen("A-345");
  k->hersteller("Ford");
  k->typ("Modell T");
  std::shared_ptr<const KFZ> f, z;
  std::shared_ptr<const mobs::ObjectBase> o;

  EXPECT_NO_THROW(f = cache.save(k));
  EXPECT_EQ("Ford", f->hersteller());
  EXPECT_NO_THROW(o = cache.searchObj("KFZ:A-345"));
  ASSERT_TRUE(o);
  EXPECT_EQ("Ford", dynamic_pointer_cast<const KFZ>(o)->hersteller());
  EXPECT_NO_THROW(z = cache.search<KFZ>(cache.escapeKey("X-12345")));
  ASSERT_TRUE(z);
  EXPECT_EQ("VW", z->hersteller());
  EXPECT_NO_THROW(z = cache.search<KFZ>("blah"));
  EXPECT_FALSE(z);


  EXPECT_EQ(1, cache.reduce(1));
  EXPECT_TRUE(cache.exists(x));
  EXPECT_FALSE(cache.exists(*o));

  EXPECT_EQ(0, cache.reduce(0));
}

}


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

#include "objpool.h"
#include "objpool.h"

#include "objgen.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;


namespace {

class Berlin : virtual public mobs::NamedObject
{
public:
  int a;
};

class Tokio : virtual public mobs::NamedObject
{
public:
  int a;
};

class Fahrzeug : virtual public mobs::NamedObject, virtual public mobs::ObjectBase
{
  public:
    ObjInit(Fahrzeug);
    MemVar(int, id);
    MemVar(string, typ);

//    void init() { keylist << id; };
    // ist das nötig=?
//    string objName() const { return typName() + "." + std::to_string(id()); };
};
ObjRegister(Fahrzeug);

TEST(objpoolTest, simple) {

  // Einen Pool erzeugen
  shared_ptr<mobs::NamedObjPool> pool = make_shared<mobs::NamedObjPool>();

  {
    mobs::NamedObjRef<Berlin> ref1(pool, "B.1");
    mobs::NamedObjRef<Tokio > ref2(pool, "T.2");
    ref1 = new Berlin;
    ref2 = new Tokio;
    ref1->a = 42;
    ref2->a = 666;
  }
  // retrueve Objects via Name
  {
    mobs::NamedObjRef<Berlin> r1(pool, "B.1");
    mobs::NamedObjRef<Tokio > r2(pool, "T.2");
    ASSERT_TRUE(r1.exists());
    ASSERT_TRUE(r2.exists());
    EXPECT_EQ(42, r1->a);
    EXPECT_EQ(666, r2->a);
  }
  {
    mobs::NamedObjRef<Berlin> xx(pool, "T.2");
    mobs::NamedObjRef<Tokio > yy(pool, "T.4");
    // wrong object
    ASSERT_FALSE(xx.exists());
    // unknown id
    ASSERT_FALSE(yy.exists());
  }
  pool->garbageCollect();
  
}

TEST(objpoolTest, create) {
  
  // Einen Pool erzeugen
  shared_ptr<mobs::NamedObjPool> pool = make_shared<mobs::NamedObjPool>();
  
  // pointer auf ein Objekt erzeugen (pool, ID)
  mobs::NamedObjRef<Fahrzeug> f1(pool, "1");
  // Objekt nicht vorhanden
  ASSERT_FALSE(f1.lock());
  // Objekt erzeugen
  EXPECT_NO_THROW(f1.create());
  // Objekt verwenden
  EXPECT_NO_THROW(f1->id(1));
  ASSERT_TRUE(f1.lock());
  
}

TEST(objpoolTest, reuse) {
  
  // Einen Pool erzeugen
  shared_ptr<mobs::NamedObjPool> pool = make_shared<mobs::NamedObjPool>();
  
  // pointer auf ein Objekt erzeugen (pool, ID)
  mobs::NamedObjRef<Fahrzeug> f1(pool, "1");
  // Objekt erzeugen
  EXPECT_NO_THROW(f1.create());
  // Objekt verwenden
  EXPECT_NO_THROW(f1->id(1));
  // Objekt vorhabedn
  ASSERT_TRUE(f1.lock());
  
  mobs::NamedObjRef<Fahrzeug> f2(pool, "1");
  // Objekt vorhabedn
  ASSERT_TRUE(f2.exists());
  // beide Objekte zeigen auf den selben Speicherbereich
  ASSERT_EQ(&*f1.lock(), &*f2.lock());
  
  const mobs::NamedObjRef<Fahrzeug> f3(pool, "1");
  
  //Lesen des Const-Objektes
  EXPECT_EQ(1, f3->id());


  
}




}


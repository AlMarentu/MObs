#include <iostream>


#include "objpool.h"

using namespace std;

class Hummel : virtual public NamedObject
{
  public:
  Hummel() { a = 7; };
  ~Hummel() {};
  string objName() const { return string("Hummel.") + std::to_string(a); };
    int a;
};

int main(int argc, char* argv[])
{
cerr << "HHHH" << endl;
  shared_ptr<NamedObjPool> pool = make_shared<NamedObjPool>();

  NamedObjRef<Hummel> fritz(pool, "Fritz");
  fritz.create();
  NamedObjRef<Hummel> ref1(pool, "Peter");
  ref1 = new Hummel();

  cerr << "HH 1 " << ref1->a << endl;
  
  NamedObjRef<Hummel> ref2(pool, "Peter");
  cerr << "HH 2 " << ref2->a << endl;

  ref1->a = 99;
  cerr << "HH 1 " << ref1->a << endl;
  cerr << "HH 2 " << ref2->a << endl;

  auto r1 = ref1.lock();
  auto r2 = ref2.lock();
  pool->garbageCollect();
  cerr << "HHR 1 " << r1->a << endl;
  cerr << "HHR 2 " << r2->a << endl;
  r1->a = 12;
  cerr << "HH 1 " << ref1->a << endl;
  cerr << "HH 2 " << ref2->a << endl;
  cerr << "HHR 1 " << r1->a << endl;
  cerr << "HHR 2 " << r2->a << endl;

  ref1 = new Hummel();
  auto r3 = ref1.lock();
  auto r4 = ref2.lock();
  cerr << "HH 1 " << ref1->a << endl;
  cerr << "HH 2 " << ref2->a << endl;
  cerr << "HHR 1 " << r1->a << endl;
  cerr << "HHR 2 " << r2->a << endl;
  cerr << "HHR 3 " << r3->a << endl;
  cerr << "HHR 4 " << r4->a << endl;
}


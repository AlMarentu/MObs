
#include "objgen.h"

#include <iostream>
#include <string>


class NOObjInterface()
{
  NOObjInterface() {};
  virtual ~NOObjInterface() = 0;
  // lädt object anhand vorausgefüllter Schlüsselfelder (true, wenn vorhanden)
  bool load(ObjectBase &obj);
};



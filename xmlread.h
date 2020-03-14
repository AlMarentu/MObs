#include "objgen.h"

class XmlReadData;
class XmlRead {
  public:
    XmlRead(const std::string &input);
    ~XmlRead();
    void fill(ObjectBase &obj);
  private:
    XmlReadData *data;
    void parse();

};


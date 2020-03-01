#include "objgen.h"

class JsonReadData;
class JsonRead {
  public:
    JsonRead(const std::string &input);
    ~JsonRead();
    void fill(ObjectBase &obj);
  private:
    JsonReadData *data;
    void parse();

};


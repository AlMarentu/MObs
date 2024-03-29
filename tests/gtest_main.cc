#include <cstdio>

#include "gtest/gtest.h"


GTEST_API_ int main(int argc, char **argv) {
  printf("Running main() from %s\n", __FILE__);
#ifdef __WIN32__
  //setlocale(LC_CTYPE, ".utf8");
#endif
  setlocale(LC_CTYPE, "");
  std::cerr << "LC_ALL=" << setlocale(LC_CTYPE, nullptr) << '\n';

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


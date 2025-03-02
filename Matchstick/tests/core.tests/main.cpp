#include "stdafx.h"

// this snippet ensures that the location of memory leaks is reported correctly
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)

int main(int argc, char **argv) {
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

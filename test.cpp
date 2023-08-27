#include <iostream>

#include "CAE.h"

void test() {
  cae_try {
    int* ptr = nullptr;
    ptr[0] = 0;
  } cae_catch(access_violation, ex) {
    std::cout << ex << std::endl;
  }
  cae_end
}

int main() {
  test();

  return 0;
}
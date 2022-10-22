#include <stdio.h>

#include "CAE.hpp"

void test_cae(int* p) {
   try {
      cae_scope;

      printf("Trying to get value of pointer 0x%X\n", p);
      int v = *p;
      printf("Value: %i\n", v);

      printf("Deleting 0x%X\n", p);
      delete p;
   } catch(cae::ExceptionAccessViolation& e) {
      (void)e;

      printf("Pointer 0x%X was invalid!\n", p);
      printf("Trying with new int...\n");
      test_cae(new int);
   }

   printf("Try for pointer 0x%X ended\n", p);
}

int main() {
   test_cae(nullptr);
}
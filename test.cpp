#include <stdio.h>

#include "CAE.hpp"

void test_cae(int* p) {
   cae::cae([p]() {
      printf("Trying to get value of pointer 0x%p\n", p);
      int v = *p;
      printf("Value: %i\n", v);

      printf("Deleting 0x%p\n", p);
      delete p;
   }).except<cae::ExceptionAccessViolation>(
      [p](cae::ExceptionAccessViolation& e) {
         printf("Pointer 0x%p was invalid!\n", p);
         printf("Trying with new int...\n");
         test_cae(new int);
      }
   );
   printf("Try for pointer 0x%p ended\n", p);
}

int main() {
   test_cae(nullptr);
}
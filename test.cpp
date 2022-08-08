#include "CAE.hpp"

#include <stdio.h>

void test_cae(int* ptr) {
   cae([ptr]() {
      printf("Trying to set int at 0x%p to zero...\n", ptr);
      *ptr = 0;
   }).except([ptr](CaeBaseException& e) {
      // if access violation catched, create new int and try again
      if (e.GetID() == CaeExceptionAccessViolation::ID) {
         auto& e2 = static_cast<CaeExceptionAccessViolation&>(e);

         // print info
         printf("Access Violation at 0x%p. Tryied to %s 0x%p\n",
            (void*)e2.getExceptionAddress(),
            cae_tostring(e2.getAccessType()),
            (void*)e2.getAccessedAddress()
         );

         // try again with created int
         test_cae(new int);
      }
   }).success([ptr]() {
      printf("No exceptions!\n");

      // if pointer is valid, dont forget to delete it
      delete ptr;
   }).onend([ptr]() {
      // just mark when execution is ended
      printf("End %p\n", ptr);
   });
}

int main() {
   test_cae(nullptr);
}
#include "CAE.hpp"

#include <string.h>
#include <stdlib.h>

#include <signal.h>

void on_sigaction(int signum, siginfo_t* si, void* arg) {
   switch (signum) {
   case SIGSEGV:
      throw cae::ExceptionAccessViolation(
         (uintptr_t)si->si_addr,
         0,
         cae::MemoryAccessType_UNKNOWN
      );
   }
   exit(1);
}

namespace cae {

   void install_catchers() {
      struct sigaction sa;
      memset(&sa, 0, sizeof(struct sigaction));

      sigemptyset(&sa.sa_mask);
      sa.sa_sigaction = &on_sigaction;
      sa.sa_flags = SA_SIGINFO;

      sigaction(SIGSEGV, &sa, NULL);
   }

   void remove_catchers() {}

}
#include "CAE.hpp"

#include <Windows.h>

static LONG CALLBACK _cae_ExceptionCatcher(_EXCEPTION_POINTERS* pointers) {
   PEXCEPTION_RECORD exception_info = pointers->ExceptionRecord;

   switch (exception_info->ExceptionCode) {
   case EXCEPTION_ACCESS_VIOLATION: {
      uintptr_t rawAccessType = exception_info->ExceptionInformation[0];
      
      throw cae::ExceptionAccessViolation(
         (uintptr_t)exception_info->ExceptionAddress,
         exception_info->ExceptionInformation[1],
         rawAccessType == 0 ? cae::MemoryAccessType_READ :
         rawAccessType & 1 ? cae::MemoryAccessType_WRITE :
         rawAccessType & 8 ? cae::MemoryAccessType_EXECUTE :
         cae::MemoryAccessType_UNKNOWN
      );
   } break;
   }

   return EXCEPTION_CONTINUE_SEARCH;
}

namespace cae {

   void install_catchers() {
      AddVectoredExceptionHandler(TRUE, _cae_ExceptionCatcher);
   }

   void remove_catchers() {
      RemoveVectoredExceptionHandler(_cae_ExceptionCatcher);
   }

}
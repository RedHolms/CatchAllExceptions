#include "CAE.hpp"

#include <string.h>
#include <assert.h>

#include <windows.h>

extern "C" DWORD _Win32GlobalTlsIndex = -1;

struct ProtectedCallState {
   struct {
      uintptr_t reg_b;
      uintptr_t reg_c;
      uintptr_t reg_d;
      uintptr_t reg_sp;
      uintptr_t reg_bp;
      uintptr_t reg_si;
      uintptr_t reg_di;
   } _context;
   cae::Corountime_t target_corountime;
   cae::BaseException* catched_exception;
};

// defined in assembly
extern "C" void _cae_pcall_save_state();

__declspec(noreturn)
extern "C" void _cae_pcall_restore_state();

// called when windows exception raised
static long __stdcall _cae_pcall_exceptions_handler(struct _EXCEPTION_POINTERS* ep) {
   { // scope to destroy all objects before returning
      PVOID ex_addr = ep->ExceptionRecord->ExceptionAddress;
      DWORD ex_code = ep->ExceptionRecord->ExceptionCode;

      ProtectedCallState* s = reinterpret_cast<ProtectedCallState*>(TlsGetValue(_Win32GlobalTlsIndex));

      switch (ex_code) {
      case EXCEPTION_ACCESS_VIOLATION: {
         ULONG_PTR access_addr = ep->ExceptionRecord->ExceptionInformation[1];
         ULONG_PTR access_type_raw = ep->ExceptionRecord->ExceptionInformation[0];
         cae::MemoryAccessType access_type =
            access_type_raw == 0 ? cae::MemoryAccessType_READ :
            access_type_raw & 1 ? cae::MemoryAccessType_WRITE :
            access_type_raw & 8 ? cae::MemoryAccessType_EXECUTE :
            cae::MemoryAccessType_UNKNOWN;

         s->catched_exception = new cae::ExceptionAccessViolation(
            reinterpret_cast<uintptr_t>(ex_addr), access_addr, access_type);
      } break;
      }
   }

   _cae_pcall_restore_state();
}

// init pcall state
static void _cae_pcall_init(ProtectedCallState& s) {
   if (_Win32GlobalTlsIndex == -1) {
      _Win32GlobalTlsIndex = TlsAlloc();
   }

   TlsSetValue(_Win32GlobalTlsIndex, &s);
   AddVectoredExceptionHandler(TRUE, &_cae_pcall_exceptions_handler);
}

// perform protected call
static void _cae_pcall_perform(ProtectedCallState& s) {
   _cae_pcall_save_state();
   s.target_corountime();
}

static void _cae_pcall_cleanup(ProtectedCallState& s) {
   RemoveVectoredExceptionHandler(&_cae_pcall_exceptions_handler);
   TlsSetValue(_Win32GlobalTlsIndex, NULL);
}

namespace cae {
   namespace intrnl {

      BaseException* ProtectedCall(std::function<void()> const& corountime) {
         if (!corountime) return nullptr;

         ProtectedCallState pcall_state;

         pcall_state.target_corountime = corountime;
         pcall_state.catched_exception = nullptr;
         memset(&pcall_state._context, 0, sizeof(pcall_state._context));

         _cae_pcall_init(pcall_state);
         _cae_pcall_perform(pcall_state);
         _cae_pcall_cleanup(pcall_state);

         return pcall_state.catched_exception;
      }

   }
}
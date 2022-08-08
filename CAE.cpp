#include "CAE.hpp"

#include <Windows.h>

// TODO: make it multi-platformed (maybe)
// now it's accepts only windows with msvc

/* determine arch */
#define _ARCH_TYPE_X86 1
#define _ARCH_TYPE_X64 2
#if \
   defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64) || defined(_M_X64) ||\
   defined(__ia64__) || defined(_IA64) || defined(__IA64__) || defined(__ia64) || defined(_M_IA64) || defined(__itanium__)
#define _ARCH_TYPE _ARCH_TYPE_X64
#else
#define _ARCH_TYPE _ARCH_TYPE_X86
#endif
#define _ARCH_IS_X86 (_ARCH_TYPE == _ARCH_TYPE_X86)
#define _ARCH_IS_X64 (_ARCH_TYPE == _ARCH_TYPE_X64)

#if _ARCH_IS_X64
#error "x64 Is Not Acceptable (MSVC Issue)"
#endif

/* some utils for asm */
#if _ARCH_IS_X64
#define CPU_REG(n) r ## n
#define PTR_SIZE 8
#else
#define CPU_REG(n) e ## n
#define PTR_SIZE 4
#endif
#define PTR_OFFSET(n) (PTR_SIZE*n)

constexpr DWORD TLS_STATE_IDX = 42;

struct CaeProtectedCallState {
   struct {
      uintptr_t reg_bx;
      uintptr_t reg_cx;
      uintptr_t reg_dx;
      uintptr_t reg_si;
      uintptr_t reg_di;
      uintptr_t reg_sp;
      uintptr_t reg_bp;
   } context;
   CaeBaseException* exception;
};

static void __cae_pcall_make_access_violation(EXCEPTION_RECORD*);
static void __cae_pcall_make_unknown(EXCEPTION_RECORD*);

static void __cae_pcall_save_context();
static void __cae_pcall_restore_context_and_return();

static inline CaeProtectedCallState* __cae_pcall_get_state() {
   return reinterpret_cast<CaeProtectedCallState*>(TlsGetValue(TLS_STATE_IDX));;
}

/* Handle exception and return */
static long __stdcall __cae_pcall_on_exception(_EXCEPTION_POINTERS* ptrs) {
   CaeProtectedCallState* pcallState = __cae_pcall_get_state();

   auto ex_record = ptrs->ExceptionRecord;

   // Make exception object
   switch (ex_record->ExceptionCode) {
      case EXCEPTION_ACCESS_VIOLATION: __cae_pcall_make_access_violation(ex_record); break;
      default: __cae_pcall_make_unknown(ex_record); break;
   }

   // Return to `cae_protectedcall`
   __cae_pcall_restore_context_and_return();

   return -1;
}

/* Process protected call */
static void cae_pcall_process(std::function<void(void)>& fn) {
   // Save context
   __cae_pcall_save_context();

   // Call function
   fn.operator()();
}

/* Save context to state */
__declspec(naked)
void __cae_pcall_save_context() {
   // get state pointer
   __asm push TLS_STATE_IDX
   __asm call TlsGetValue
   // state now stored in register A

   // save all registers except A
   __asm mov[CPU_REG(ax)], CPU_REG(bx)
   __asm mov[CPU_REG(ax) + PTR_OFFSET(1)], CPU_REG(cx)
   __asm mov[CPU_REG(ax) + PTR_OFFSET(2)], CPU_REG(dx)
   __asm mov[CPU_REG(ax) + PTR_OFFSET(3)], CPU_REG(si)
   __asm mov[CPU_REG(ax) + PTR_OFFSET(4)], CPU_REG(di)
   __asm mov[CPU_REG(ax) + PTR_OFFSET(5)], CPU_REG(sp)
   __asm mov[CPU_REG(ax) + PTR_OFFSET(6)], CPU_REG(bp)

   // return
   __asm ret
}

/* Restore context from state and return to `cae_protectedcall` */
__declspec(naked)
void __cae_pcall_restore_context_and_return() {
   // get state pointer
   __asm push TLS_STATE_IDX
   __asm call TlsGetValue
   // state now stored in register A

   // restore all registers except A
   __asm mov CPU_REG(bx), [CPU_REG(ax)]
   __asm mov CPU_REG(cx), [CPU_REG(ax) + PTR_OFFSET(1)]
   __asm mov CPU_REG(dx), [CPU_REG(ax) + PTR_OFFSET(2)]
   __asm mov CPU_REG(si), [CPU_REG(ax) + PTR_OFFSET(3)]
   __asm mov CPU_REG(di), [CPU_REG(ax) + PTR_OFFSET(4)]
   __asm mov CPU_REG(sp), [CPU_REG(ax) + PTR_OFFSET(5)]
   __asm mov CPU_REG(bp), [CPU_REG(ax) + PTR_OFFSET(6)]

   // Stack pointer now restored, and after return we will be returned to `cae_protectedcall`
   __asm ret
}

/* === FUNCTIONS TO MAKE EXCEPTIONS FROM OS-DATA === */
static void __cae_pcall_make_access_violation(EXCEPTION_RECORD* record) {
   CaeProtectedCallState* pcallState = __cae_pcall_get_state();

   CaeExceptionAccessViolation::AccessType accessType;
   switch (record->ExceptionInformation[0]) {
   case 0: accessType = CaeExceptionAccessViolation::AccessType::READ; break;
   case 1: accessType = CaeExceptionAccessViolation::AccessType::WRITE; break;
   case 8: accessType = CaeExceptionAccessViolation::AccessType::EXECUTE; break;
   default: accessType = CaeExceptionAccessViolation::AccessType::UNKNOWN; break;
   }

   pcallState->exception = new CaeExceptionAccessViolation(
      reinterpret_cast<uintptr_t>(record->ExceptionAddress),
      record->ExceptionInformation[1],
      accessType
   );
}

static void __cae_pcall_make_unknown(EXCEPTION_RECORD* record) {
   CaeProtectedCallState* pcallState = __cae_pcall_get_state();

   pcallState->exception = new CaeExceptionUnknown(
      reinterpret_cast<uintptr_t>(record->ExceptionAddress)
   );
}

/* === USER-API FUNCTIONS === */
__declspec(noinline)
CaeBaseException* cae_protectedcall(std::function<void(void)>& fn) {
   // Init state
   auto protectedCallState = new CaeProtectedCallState;
   memset(protectedCallState, 0, sizeof(CaeProtectedCallState));
   TlsSetValue(TLS_STATE_IDX, protectedCallState);

   // Process
   AddVectoredExceptionHandler(TRUE, __cae_pcall_on_exception);
   cae_pcall_process(fn);
   RemoveVectoredExceptionHandler(__cae_pcall_on_exception);

   CaeBaseException* exception = protectedCallState->exception;

   // Cleanup
   delete protectedCallState;
   TlsSetValue(TLS_STATE_IDX, nullptr);

   // Return
   return exception;
}

const char* cae_tostring(CaeExceptionAccessViolation::AccessType val) {
   switch (val) {
   case CaeExceptionAccessViolation::AccessType::READ: return "READ";
   case CaeExceptionAccessViolation::AccessType::WRITE: return "WRITE";
   case CaeExceptionAccessViolation::AccessType::EXECUTE: return "EXECUTE";
   default: return "UNKNOWN";
   }
}
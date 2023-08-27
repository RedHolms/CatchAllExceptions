#include "CAE.h"

#include <stack>

#include <Windows.h>

extern "C" {

  static bool LowLevelContextTlsIndexAllocated = false;
  static DWORD LowLevelContextTlsIndex = 0;

  struct LowLevelContext {
    uintptr_t stack_pointer; // rsp / esp
    uintptr_t return_address;
    uintptr_t exception_pointer; // pointer to system_exception
  };

  // Functions defined in assembly
  __declspec(noreturn)
  void __cdecl CAE_Raise(const system_exception* exception_pointer);
  system_exception* __cdecl CAE_ProtedctedCallImpl(const void* worker);

  // Functions called from assembly
  void __cdecl CAE_CallStdFunction(std::function<void()> const& fn) {
    fn();
  }

  LowLevelContext* __cdecl CAE_PushLowLevelContext() {
    std::stack<LowLevelContext>* low_level_context_stack = reinterpret_cast<std::stack<LowLevelContext>*>(TlsGetValue(LowLevelContextTlsIndex));

    low_level_context_stack->push(LowLevelContext{});
    return &low_level_context_stack->top();
  }

  LowLevelContext* __cdecl CAE_GetLowLevelContext() {
    std::stack<LowLevelContext>* low_level_context_stack = reinterpret_cast<std::stack<LowLevelContext>*>(TlsGetValue(LowLevelContextTlsIndex));

    return &low_level_context_stack->top();
  }

  void __cdecl CAE_PopLowLevelContext() {
    std::stack<LowLevelContext>* low_level_context_stack = reinterpret_cast<std::stack<LowLevelContext>*>(TlsGetValue(LowLevelContextTlsIndex));

    low_level_context_stack->pop();
  }

}

void __declspec(noreturn) CAE_RaiseAccessViolation(EXCEPTION_RECORD* exception) {
  ULONG_PTR win32_memory_operation = exception->ExceptionInformation[0];
  ULONG_PTR accessed_address = exception->ExceptionInformation[1];
  memory_operation operation;

  if (win32_memory_operation == 0)
    operation = memory_operation::read;
  else if (win32_memory_operation == 1)
    operation = memory_operation::write;
  else if (win32_memory_operation == 8)
    operation = memory_operation::execute;
  else
    operation = memory_operation::unknown;

  CAE_Raise(new access_violation(
    reinterpret_cast<uintptr_t>(exception->ExceptionAddress),
    operation, accessed_address,
    EXCEPTION_ACCESS_VIOLATION
  ));
}

void __declspec(noreturn) CAE_RaiseUnknown(EXCEPTION_RECORD* exception) {
  CAE_Raise(new unknown_system_exception(
    reinterpret_cast<uintptr_t>(exception->ExceptionAddress),
    exception->ExceptionCode
  ));
}

LONG WINAPI CAE_ExceptionFitler(_EXCEPTION_POINTERS* pointers) {
  EXCEPTION_RECORD* exception = pointers->ExceptionRecord;

  if (!exception)
    return EXCEPTION_CONTINUE_SEARCH;

  // CXX exception
  if (exception->ExceptionCode == 0xE06D7363)
    return EXCEPTION_CONTINUE_SEARCH;

  switch (exception->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
      CAE_RaiseAccessViolation(exception);
      return EXCEPTION_CONTINUE_SEARCH;
  }

  CAE_RaiseUnknown(exception);
  return EXCEPTION_CONTINUE_SEARCH;
}

void CAE::ProtectedCall(CAE::context& ctx) {
  if (!ctx.m_worker)
    return;

  if (!LowLevelContextTlsIndexAllocated) {
    LowLevelContextTlsIndexAllocated = true;
    LowLevelContextTlsIndex = TlsAlloc();
  }

  if (TlsGetValue(LowLevelContextTlsIndex) == NULL) {
    TlsSetValue(LowLevelContextTlsIndex, new std::stack<LowLevelContext>());
  }

  PVOID exception_fitler_handle;

  exception_fitler_handle = AddVectoredExceptionHandler(TRUE, CAE_ExceptionFitler);

  system_exception* exception = CAE_ProtedctedCallImpl(&ctx.m_worker);

  RemoveVectoredExceptionHandler(exception_fitler_handle);

  if (exception) {
    uint32_t exception_type_id = exception->get_id();

    bool handler_called = false;
    for (auto& handler : ctx.m_exceptions_handlers) {
      if (handler.exception_type_id == exception_type_id) {
        handler.handler(*exception);
        handler_called = true;
        break;
      }
    }

    if (!handler_called) {
      for (auto& handler : ctx.m_exceptions_handlers) {
        if (handler.exception_type_id == system_exception::id) {
          handler.handler(*exception);
          break;
        }
      }
    }
  }
  else {
    if (ctx.m_else)
      ctx.m_else();
  }

  if (ctx.m_finally)
    ctx.m_finally();

  ctx.m_worker = {};
}

#include "CAE.h"

#include <stack>
#include <mutex>

#include <Windows.h>

static std::mutex ProtectedCallContextStackTlsIndexMutex;
static bool ProtectedCallContextStackTlsIndexAllocated = false;
static DWORD ProtectedCallContextStackTlsIndex = 0;

static inline void CheckProtectedCallContextStackTlsIndex() {
  // Very thread-safe
  ProtectedCallContextStackTlsIndexMutex.lock();
  if (!ProtectedCallContextStackTlsIndexAllocated) {
    ProtectedCallContextStackTlsIndexAllocated = true;
    ProtectedCallContextStackTlsIndex = TlsAlloc();
  }
  ProtectedCallContextStackTlsIndexMutex.unlock();
}

static inline std::stack<CAE_ProtectedCallContext>* GetProtectedCallContextStack() {
  CheckProtectedCallContextStackTlsIndex();

  std::stack<CAE_ProtectedCallContext>* stack = reinterpret_cast<std::stack<CAE_ProtectedCallContext>*>(TlsGetValue(ProtectedCallContextStackTlsIndex));

  if (!stack) {
    stack = new std::stack<CAE_ProtectedCallContext>;
    TlsSetValue(ProtectedCallContextStackTlsIndex, stack);
  }

  return stack;
}

extern "C" {

  void __cdecl CAE_CallStdFunction(std::function<void()> const& function) {
    function();
  }

  CAE_ProtectedCallContext* __cdecl CAE_PushProtectedCallContext() {
    auto* stack = GetProtectedCallContextStack();

    stack->push({});
    return &stack->top();
  }

  CAE_ProtectedCallContext* __cdecl CAE_GetProtectedCallContext() {
    auto* stack = GetProtectedCallContextStack();

    return &stack->top();
  }

  void __cdecl CAE_PopProtectedCallContext() {
    auto* stack = GetProtectedCallContextStack();

    stack->pop();
  }

}

void __declspec(noreturn) CAE_ThrowAccessViolation(EXCEPTION_RECORD* exception) {
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

  CAE_Throw(new access_violation(
    reinterpret_cast<uintptr_t>(exception->ExceptionAddress),
    operation, accessed_address,
    EXCEPTION_ACCESS_VIOLATION
  ));
}

void __declspec(noreturn) CAE_ThrowUnknown(EXCEPTION_RECORD* exception) {
  CAE_Throw(new unknown_system_exception(
    reinterpret_cast<uintptr_t>(exception->ExceptionAddress),
    exception->ExceptionCode
  ));
}

// Exception filter called by Windows, when exception is thrown
// We'll handle it, and pass to the our catchers
LONG WINAPI CAE_ExceptionFitler(_EXCEPTION_POINTERS* pointers) {
  EXCEPTION_RECORD* exception = pointers->ExceptionRecord;

  // No exception???
  if (!exception)
    return EXCEPTION_CONTINUE_SEARCH;

  // CXX exception
  if (exception->ExceptionCode == 0xE06D7363)
    return EXCEPTION_CONTINUE_SEARCH;

  switch (exception->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
      CAE_ThrowAccessViolation(exception);
      return EXCEPTION_CONTINUE_SEARCH;
  }

  CAE_ThrowUnknown(exception);
  return EXCEPTION_CONTINUE_SEARCH;
}

// Invoke context worker in protected enviroment
void CAE::context::invoke() {
  if (!m_worker)
    return;
  
  // Push our vectored exception handler
  PVOID exception_fitler_handle = AddVectoredExceptionHandler(TRUE, CAE_ExceptionFitler);

  // Actually do protected call
  system_exception* exception = CAE_PerformProtectedCall(m_worker);

  // Pop our vectored exception handler
  RemoveVectoredExceptionHandler(exception_fitler_handle);

  if (exception) {
    // If we cathed an exception

    do {

      uint32_t exception_type_id = exception->get_id();
      bool handler_called = false;

      // Find exact catcher
      for (CAE::context::exception_catcher& catcher : m_exceptions_catchers) {
        if (catcher.exception_type_id == exception_type_id) {
          catcher.handler(*exception);
          handler_called = true;
          break;
        }
      }

      if (handler_called)
        break;

      // There's no exact catcher, find who catches all system exceptions
      for (CAE::context::exception_catcher& catcher : m_exceptions_catchers) {
        if (catcher.exception_type_id == -1) {
          catcher.handler(*exception);
          break;
        }
      }

    } while (0);

    // Dont forget to clean-up a memory
    delete exception;
  }
  else {
    // No exception

    if (m_else)
      m_else();
  }

  if (m_finally)
    m_finally();

  // Delete worker, so if we're called `invoke` again, worker wont be called again
  m_worker = {};
}

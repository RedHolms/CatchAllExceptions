.code

extern CAE_CallStdFunction : near
extern CAE_PushProtectedCallContext : near
extern CAE_GetProtectedCallContext : near
extern CAE_PopProtectedCallContext : near

; Throw exception
CAE_Throw proc
  ; RCX = thrown exception pointer

  ; Save exception pointer
  push rcx

  ; Get our context
  call CAE_GetProtectedCallContext

  ; Restore exception pointer
  pop rcx

  ; Save exception pointer to context
  mov qword ptr [rax + 16], rcx

  ; Restore stack pointer from context
  mov rsp, qword ptr [rax]

  ; Jump to catcher
  jmp qword ptr [rax + 8]

CAE_Throw endp

; Perform protected call
CAE_PerformProtectedCall proc
  ; RCX = worker

  ; Save all registers to stack
  push rax
  push rbx
  push rcx
  push rdx
  push rsi
  push rbp
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  ; Save worker
  push rcx

  ; Create new context
  call CAE_PushProtectedCallContext

  ; Restore worker
  pop rcx

  ; We have context in RAX (return value of CAE_PushProtectedCallContext)

  ; Put stack pointer
  mov qword ptr [rax], rsp

  ; Put catcher
  mov rbx, CatchException
  mov qword ptr [rax + 8], rbx

  ; Put zero to place for thrown exception
  xor rbx, rbx
  mov qword ptr [rax + 16], rbx

  ; We still have worker in RCX, so just pass it
  ;  to the CAE_CallStdFunction, and call it
  call CAE_CallStdFunction

CatchException: ; <<= CAE_Throw will jump here with stack pointer restored

  ; Restore registers from stack
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop rbp
  pop rsi
  pop rdx
  pop rcx
  pop rbx
  pop rax
  
  ; Get current context
  call CAE_GetProtectedCallContext

  ; Put thrown (or not) exception as return value
  mov rax, qword ptr [rax + 16]

  ; Save exception pointer
  push rax

  ; Delete context
  call CAE_PopProtectedCallContext

  ; Restore exception pointer
  pop rax

  ; Finally return
  ret

CAE_PerformProtectedCall endp

end

.model flat
.code

extern _CAE_CallStdFunction : near
extern _CAE_PushProtectedCallContext : near
extern _CAE_GetProtectedCallContext : near
extern _CAE_PopProtectedCallContext : near

; Throw exception
_CAE_Throw proc
  ; [esp + 4] = thrown exception pointer

  ; Get our context
  call _CAE_GetProtectedCallContext

  ; Save exception pointer to context
  mov ebx, dword ptr [esp + 4]
  mov dword ptr [eax + 8], ebx

  ; Restore stack pointer from context
  mov esp, dword ptr [eax]

  ; Jump to catcher
  jmp dword ptr [eax + 4]

_CAE_Throw endp

; Perform protected call
_CAE_PerformProtectedCall proc
  ; [esp + 4] = worker

  ; Save all registers to stack
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push ebp

  ; Now worker at [esp + 24 + 4] (because we've pushed registers)

  ; Create new context
  call _CAE_PushProtectedCallContext

  ; We have context in EAX (return value of CAE_PushProtectedCallContext)

  ; Put stack pointer
  mov dword ptr [eax], esp

  ; Put catcher
  mov ebx, CatchException
  mov dword ptr [eax + 4], ebx

  ; Put zero to place for thrown exception
  xor ebx, ebx
  mov dword ptr [eax + 8], ebx

  ; Push worker as argument, and then call it
  mov eax, dword ptr [esp + 24 + 4]
  push eax
  call _CAE_CallStdFunction

  ; We're need to clean up stack, because it's cdecl
  add esp, 4

CatchException: ; <<= CAE_Throw will jump here with stack pointer restored

  ; Restore registers from stack
  pop ebp
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  
  ; Get current context
  call _CAE_GetProtectedCallContext

  ; Put thrown (or not) exception as return value
  mov eax, dword ptr [eax + 8]

  ; Save exception pointer
  push eax

  ; Delete context
  call _CAE_PopProtectedCallContext

  ; Restore exception pointer
  pop eax

  ; Finally return
  ret

_CAE_PerformProtectedCall endp

end

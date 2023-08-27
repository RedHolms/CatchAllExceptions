.code

extern CAE_CallStdFunction : near
extern CAE_PushLowLevelContext : near
extern CAE_GetLowLevelContext : near
extern CAE_PopLowLevelContext : near

CAE_Raise proc
  push rcx

  call CAE_GetLowLevelContext

  pop rcx

  mov qword ptr [rax + 16], rcx

  mov rsp, qword ptr [rax]

  jmp qword ptr [rax + 8]

CAE_Raise endp

CAE_ProtedctedCallImpl proc
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

  push rcx

  call CAE_PushLowLevelContext

  pop rcx

  mov qword ptr [rax], rsp
  mov rbx, CatchException
  mov qword ptr [rax + 8], rbx
  xor rbx, rbx
  mov qword ptr [rax + 16], rbx

  call CAE_CallStdFunction

CatchException:
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
  
  call CAE_GetLowLevelContext
  mov rax, qword ptr [rax + 16]

  push rax
  call CAE_PopLowLevelContext
  pop rax

  ret

CAE_ProtedctedCallImpl endp

end

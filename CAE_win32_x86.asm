.model flat
.code

extern _CAE_CallStdFunction : near
extern _CAE_PushLowLevelContext : near
extern _CAE_GetLowLevelContext : near
extern _CAE_PopLowLevelContext : near

_CAE_Raise proc
  call _CAE_GetLowLevelContext

  mov ebx, dword ptr [esp + 4]
  mov dword ptr [eax + 8], ebx

  mov esp, dword ptr [eax]

  jmp dword ptr [eax + 4]

_CAE_Raise endp

_CAE_ProtedctedCallImpl proc
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push ebp

  call _CAE_PushLowLevelContext

  mov dword ptr [eax], esp
  mov ebx, CatchException
  mov dword ptr [eax + 4], ebx
  xor ebx, ebx
  mov dword ptr [eax + 8], ebx

  push dword ptr [esp + 24 + 4]
  call _CAE_CallStdFunction
  add esp, 4

CatchException:
  pop ebp
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax

  call _CAE_GetLowLevelContext
  mov eax, dword ptr [eax + 8]

  push eax
  call _CAE_PopLowLevelContext
  pop eax

  ret

_CAE_ProtedctedCallImpl endp

end

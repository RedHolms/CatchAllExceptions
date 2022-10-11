.386
.MODEL flat

extern __Win32GlobalTlsIndex : near
extern __imp__TlsGetValue@4 : near

public __cae_pcall_save_state
public __cae_pcall_restore_state

.code

__get_pcall_state:
   push dword ptr [__Win32GlobalTlsIndex]
   call dword ptr [__imp__TlsGetValue@4]
   ret

__cae_pcall_save_state:
	push eax
   
   call __get_pcall_state

   mov [eax   ], ebx
   mov [eax+ 4], ecx
   mov [eax+ 8], edx
   mov [eax+12], esp
   mov [eax+16], ebp
   mov [eax+20], esi
   mov [eax+24], edi

   pop eax
   ret

__cae_pcall_restore_state:
   call __get_pcall_state

   mov ebx, [eax   ]
   mov ecx, [eax+ 4]
   mov edx, [eax+ 8]
   mov esp, [eax+12]
   mov ebp, [eax+16]
   mov esi, [eax+20]
   mov edi, [eax+24]

   pop eax
   ret
   
END
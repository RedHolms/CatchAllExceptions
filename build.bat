@echo off
setlocal

set _cl=cl /nologo /c /EHsc /Zi
set _ml=ml /nologo /c
set _link=link /nologo /DEBUG:FULL

%_cl% CAE.cpp /Fo:CAE.obj
%_cl% test.cpp /Fo:test.obj
%_ml% CAE_x86.asm /Fo CAE_x86.obj
%_link% CAE.obj CAE_x86.obj test.obj Kernel32.lib /out:CAE_Example.exe

endlocal
@echo off
setlocal

set _cl=cl /nologo /c /EHsc
set _link=link /nologo

%_cl% CAE_win32.cpp /Fo:CAE.obj
%_cl% test.cpp /Fo:test.obj
%_link% CAE.obj test.obj Kernel32.lib /out:CAE_test.exe

endlocal
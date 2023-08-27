@echo off
..\..\nasm\nasm.exe -f win64 -o out.obj out.asm
link out.obj /subsystem:console /entry:main /DYNAMICBASE "kernel32.lib" /out:out.exe 

ECHO  Running out.exe
out.exe
ECHO RESULT: %ERRORLEVEL%

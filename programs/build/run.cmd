@echo off
..\..\nasm\nasm.exe -f win64 -o out.obj out.asm
link out.obj /subsystem:console /entry:__main /DYNAMICBASE "kernel32.lib" /out:out.exe 

out.exe
ECHO RESULT: %ERRORLEVEL%

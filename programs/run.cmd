@echo off

link build\out.obj /subsystem:console /entry:__main /DYNAMICBASE "kernel32.lib" /out:build\out.exe 

build\out.exe
ECHO RESULT: %ERRORLEVEL%

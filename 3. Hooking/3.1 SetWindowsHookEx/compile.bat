@ECHO OFF

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

cl.exe /nologo /Ox /MT /W0 /GS- /DNDEBUG /LD /Tp implant_dll.cpp /link /OUT:implant.dll /SUBSYSTEM:CONSOLE

del *.obj
del *.lib
del *.exp

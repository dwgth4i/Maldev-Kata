@ECHO OFF

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

cl.exe /nologo /Ox /MT /W0 /GS- /DNDEBUG /Tp implant_injector.cpp /link /OUT:implant_injector.exe /SUBSYSTEM:CONSOLE

del *.obj

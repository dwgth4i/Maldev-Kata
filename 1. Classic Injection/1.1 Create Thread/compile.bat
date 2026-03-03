rem call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
@ECHO OFF

cl.exe /nologo /Ox /MT /W0 /GS- /DNDEBUG /Tp 1_classic_injection.cpp /link /OUT:1_classic_injection.exe /SUBSYSTEM:CONSOLE

del *.obj

@ECHO OFF

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

cl.exe /nologo /W0 implant.cpp /MT /link /DLL detours\lib.X64\detours.lib /OUT:implant.dll

cl.exe /nologo /Ox /MT /W0 /GS- /DNDEBUG /Tp messagebox.cpp /link /OUT:messagebox.exe /SUBSYSTEM:CONSOLE
del *.obj *.lib *.exp
@ECHO OFF

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

cl.exe /nologo /Ox /MT /W0 /GS- /DNDEBUG /Tp *.cpp /link /OUT:module_enum.exe /SUBSYSTEM:CONSOLE

del *.obj

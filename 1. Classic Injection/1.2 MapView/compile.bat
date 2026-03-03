@ECHO OFF

cl.exe /nologo /Ox /MT /W0 /GS- /DNDEBUG /Tp *.cpp /link /OUT:2_section_view.exe /SUBSYSTEM:CONSOLE

del *.obj

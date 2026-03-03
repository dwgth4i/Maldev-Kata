@ECHO OFF

cl.exe /DUNICODE /D_UNICODE /EHsc /W3 implant.cpp /link pdh.lib /out:implant.exe

del *.obj

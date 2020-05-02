cd C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\
C:
call "vcvars64.bat"
cd D:\Benutzer\Victor\Textdateien\C++\HiraethEngine3\HiraethEngine3\meta\commands
D:

CL.exe commands_meta.cpp /DEBUG:FULL /WX- /Zc:forScope /Gd /W3 /GS /ZI /Gm- /Od /MD /std:c++17 /FC /EHsc /Fo."/bin-int/" /link user32.lib /out:commands_meta.exe

echo Done Building
echo. 
echo. 

commands_meta.exe ..\..\HiraethEngine3\src\meta\commands.meta

echo.
echo.
echo Successfully ran commands_meta
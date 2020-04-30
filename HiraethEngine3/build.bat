D:\Benutzer\Victor\Programme\VisualStudio\19\MSBuild\Current\Bin\msbuild.exe HiraethEngine3.sln /t:Build /p:Configuration=Debug /p:Platform=x64


IF %ERRORLEVEL% == 0 (
	cd bin/HiraethEngine3/
	.\HiraethEngine3-Debug-x64.exe
	cd ..\..
)

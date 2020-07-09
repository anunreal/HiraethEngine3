.\meta\commands\commands_meta.exe HiraethEngine3/src/meta/commands.meta
D:\Benutzer\Victor\Programme\VisualStudio\19\MSBuild\Current\Bin\msbuild.exe HiraethEngine3.sln /t:Build /p:Configuration=DebugEngine /p:Platform=x64

IF %ERRORLEVEL% == 0 (
	cd out/bin/HiraethEngine3/
	.\HiraethEngine3-DebugEngine-x64.exe
	cd ..\..\..
)

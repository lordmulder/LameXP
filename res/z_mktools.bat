@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

for %%i in (tools\*.exe) do (
	echo %%~nxi
	set "OUTNAME=%%~ni"
	set "OUTNAME=!OUTNAME:.=-!"
	echo ^<!DOCTYPE RCC^> > "Tools.!OUTNAME!.qrc"
	echo ^<RCC version="1.0"^>^<qresource^>^<file^>tools/%%~nxi^</file^>^</qresource^>^</RCC^> >> "Tools.!OUTNAME!.qrc"
)

pause

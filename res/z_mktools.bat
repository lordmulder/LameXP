@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

for %%k in (exe,gpg) do (
	for %%i in (tools\*.%%k) do (
		echo %%~i
		set "OUTNAME=%%~ni"
		set "OUTNAME=!OUTNAME:.=-!"
		echo ^<^^!DOCTYPE RCC^>> "Tools.!OUTNAME!.qrc"
		echo ^<RCC version="1.0"^>^<qresource^>^<file^>tools/%%~nxi^</file^>^</qresource^>^</RCC^>>> "Tools.!OUTNAME!.qrc"
	)
)

pause

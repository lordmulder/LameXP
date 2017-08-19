@echo off

:: ---------------------------------------------------------------------------
:: COPY FILE
:: ---------------------------------------------------------------------------

set "CP_SRC=%~1"
set "CP_DST=%~2"

"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" white "copy \"%CP_SRC:\=\\%\" to \"%CP_DST:\=\\%\""

if not exist "%~1" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nCopy failed: Source file \"%CP_SRC:\=\\%\" not found!\n"
	pause && exit
)

copy "%CP_SRC%" "%CP_DST%"
if not "%ERRORLEVEL%" == "0" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nCopy failed: Operation faild with error code %ERRORLEVEL%!\n"
	pause && exit
)

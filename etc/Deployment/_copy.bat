@echo off

:: ---------------------------------------------------------------------------
:: COPY FILE
:: ---------------------------------------------------------------------------

set "CP_SRC=%~f1"
set "CP_DST=%~f2"

"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" white "copy \"%CP_SRC:\=\\%\" to \"%CP_DST:\=\\%\""

if not exist "%CP_SRC%" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nCopy failed: Source file \"%CP_SRC:\=\\%\" not found!\n"
	pause && exit
)

copy /B /Y /V "%CP_SRC%" "%CP_DST%"

if not "%ERRORLEVEL%" == "0" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nCopy failed: Operation faild with error code %ERRORLEVEL%!\n"
	pause && exit
)

if exist "%CP_DST%\*" (
	fc /B "%CP_SRC%" "%CP_DST%\%~nx1"
) else (
	fc /B "%CP_SRC%" "%CP_DST%"
)

if not "%ERRORLEVEL%" == "0" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nCopy failed: File content does not match!\n"
	pause && exit
)

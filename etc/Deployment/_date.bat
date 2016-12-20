@echo off

:: ---------------------------------------------------------------------------
:: SETUP BUILD DATE
:: ---------------------------------------------------------------------------

set "ISO_DATE="

if exist "%~dp0\..\..\..\Prerequisites\GnuWin32\date.exe" (
	for /F "tokens=1,2 delims=:" %%a in ('"%~dp0\..\..\..\Prerequisites\GnuWin32\date.exe" +ISODATE:%%Y-%%m-%%d') do (
		if "%%a"=="ISODATE" set "ISO_DATE=%%b"
	)
)

if "%ISO_DATE%"=="" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nFailed to set up build date!\n"
	pause && exit
)

echo.
echo Build Date: %ISO_DATE%
echo.

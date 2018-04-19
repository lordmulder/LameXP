@echo off

:: ---------------------------------------------------------------------------
:: SETUP BUILD DATE
:: ---------------------------------------------------------------------------

set "ISO_DATE="
set "ISO_TIME="

if exist "%~dp0\..\..\..\Prerequisites\GnuWin32\date.exe" (
	for /F "usebackq tokens=1,2" %%a in (`start /B "date" "%~dp0\..\..\..\Prerequisites\GnuWin32\date.exe" +"%%Y-%%m-%%d %%H:%%M:%%S"`) do (
		set "ISO_DATE=%%a"
		set "ISO_TIME=%%b"
	)
)

if "%ISO_DATE%"=="" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nFailed to set up build date!\n"
	pause && exit
)

if "%ISO_TIME%"=="" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nFailed to set up build time!\n"
	pause && exit
)

echo.
echo Build Date/Time: %ISO_DATE% %ISO_TIME%
echo.

@echo off

:: ---------------------------------------------------------------------------
:: CLEAR
:: ---------------------------------------------------------------------------

set "VER_LAMEXP_MAJOR="
set "VER_LAMEXP_MINOR_HI="
set "VER_LAMEXP_MINOR_LO="
set "VER_LAMEXP_BUILD="
set "VER_LAMEXP_TYPE="
set "VER_LAMEXP_PATCH="
set "VER_LAMEXP_BASENAME="

:: ---------------------------------------------------------------------------
:: PARSE CONFIG FILE
:: ---------------------------------------------------------------------------

for /f "tokens=2,*" %%s in (%~dp0\..\..\src\Config.h) do (
	if "%%s"=="VER_LAMEXP_MAJOR"    set "VER_LAMEXP_MAJOR=%%~t"
	if "%%s"=="VER_LAMEXP_MINOR_HI" set "VER_LAMEXP_MINOR_HI=%%~t"
	if "%%s"=="VER_LAMEXP_MINOR_LO" set "VER_LAMEXP_MINOR_LO=%%~t"
	if "%%s"=="VER_LAMEXP_BUILD"    set "VER_LAMEXP_BUILD=%%~t"
	if "%%s"=="VER_LAMEXP_TYPE"     set "VER_LAMEXP_TYPE=%%~t"
	if "%%s"=="VER_LAMEXP_PATCH"    set "VER_LAMEXP_PATCH=%%~t"
)

:: ---------------------------------------------------------------------------
:: CHECK RESULT
:: ---------------------------------------------------------------------------

if "%VER_LAMEXP_MAJOR%"==""    goto:version_failure
if "%VER_LAMEXP_MINOR_HI%"=="" goto:version_failure
if "%VER_LAMEXP_MINOR_LO%"=="" goto:version_failure
if "%VER_LAMEXP_BUILD%"==""    goto:version_failure
if "%VER_LAMEXP_TYPE%"==""     goto:version_failure
if "%VER_LAMEXP_PATCH%"==""    goto:version_failure

goto:version_success

:version_failure
echo. && echo "Failed to set up build date!"
echo. && pause && exit

:: ---------------------------------------------------------------------------
:: GET RELEASE TYPE
:: ---------------------------------------------------------------------------

:version_success

set "VER_LAMEXP_BASENAME=LameXP"
if "%VER_LAMEXP_TYPE%" == "Alpha" set "VER_LAMEXP_BASENAME=LameXP-ALPHA"
if "%VER_LAMEXP_TYPE%" == "Beta"  set "VER_LAMEXP_BASENAME=LameXP-BETA"
if "%VER_LAMEXP_TYPE%" == "RC"    set "VER_LAMEXP_BASENAME=LameXP-RC%VER_LAMEXP_PATCH%"

:: ---------------------------------------------------------------------------
:: OUTPUT RESULT
:: ---------------------------------------------------------------------------

echo.
echo LameXP Version: %VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO%, Build #%VER_LAMEXP_BUILD% (%VER_LAMEXP_TYPE%-%VER_LAMEXP_PATCH%)
echo.

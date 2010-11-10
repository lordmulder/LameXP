@echo off
set "LAMEXP_ERROR=1"
REM ------------------------------------------
set "VER_LAMEXP_MAJOR="
set "VER_LAMEXP_MINOR_HI="
set "VER_LAMEXP_MINOR_LO="
set "VER_LAMEXP_BUILD="
set "VER_LAMEXP_SUFFIX="
REM ------------------------------------------
FOR /F "tokens=2,3" %%s IN (..\..\src\Resource.h) DO (
	if "%%s"=="VER_LAMEXP_MAJOR" set "VER_LAMEXP_MAJOR=%%t"
	if "%%s"=="VER_LAMEXP_MINOR_HI" set "VER_LAMEXP_MINOR_HI=%%t"
	if "%%s"=="VER_LAMEXP_MINOR_LO" set "VER_LAMEXP_MINOR_LO=%%t"
	if "%%s"=="VER_LAMEXP_BUILD" set "VER_LAMEXP_BUILD=%%t"
	if "%%s"=="VER_LAMEXP_SUFFIX" set "VER_LAMEXP_SUFFIX=%%t"
)
REM ------------------------------------------
set "LAMEXP_ERROR=1"
if "%VER_LAMEXP_MAJOR%"=="" GOTO:EOF
if "%VER_LAMEXP_MINOR_HI%"=="" GOTO:EOF
if "%VER_LAMEXP_MINOR_LO%"=="" GOTO:EOF
if "%VER_LAMEXP_BUILD%"=="" GOTO:EOF
if "%VER_LAMEXP_SUFFIX%"=="" GOTO:EOF
REM ------------------------------------------
echo LameXP Version:
echo %VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO%, Build #%VER_LAMEXP_BUILD% (%VER_LAMEXP_SUFFIX%)
echo.
REM ------------------------------------------
set "LAMEXP_ERROR=0"

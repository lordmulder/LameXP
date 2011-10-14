@echo off
set "ISO_DATE="
set "LAMEXP_ERROR=1"
REM ------------------------------------------
if not exist "%~dp0\..\Utilities\Date.exe" GOTO:EOF
for /F "tokens=1,2 delims=:" %%a in ('"%~dp0\..\Utilities\Date.exe" +ISODATE:%%Y-%%m-%%d') do (
	if "%%a"=="ISODATE" set "ISO_DATE=%%b"
)
if "%ISO_DATE%"=="" GOTO:EOF
REM ------------------------------------------
echo %ISO_DATE%
set "LAMEXP_ERROR=0"

@echo off
set "ISO_DATE="
set "LAMEXP_ERROR=1"
REM ------------------------------------------
if not exist "%~dp0\_date.exe" GOTO:EOF
set "DATE_TEMP_FILE=%TEMP%\~date.%RANDOM%.tmp"
"%~dp0\_date.exe" +%%Y-%%m-%%d > "%DATE_TEMP_FILE%"
set /p "ISO_DATE=" < "%DATE_TEMP_FILE%"
del "%DATE_TEMP_FILE%"
set "DATE_TEMP_FILE="
echo BUILD DATE: %ISO_DATE%
REM ------------------------------------------
set "LAMEXP_ERROR=0"

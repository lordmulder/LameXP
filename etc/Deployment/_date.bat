@echo off
set "DATE_TMP=%TEMP%\~date.%RANDOM%.tmp"
"%~dp0\_date.exe" +%%Y-%%m-%%d > "%DATE_TMP%"
set /p OUT_DATE= < "%DATE_TMP%"
del "%DATE_TMP%"
set "DATE_TMP="

@echo off
REM ---------------------------
set "QTVARS=E:\Qt\MSVC\4.7.1\bin\qtvars.bat"
REM ---------------------------
call "%QTVARS%"
REM ---------------------------
for %%f in (LameXP_??.ts) do (
	lrelease.exe %%f -qm ..\..\res\localization\%%~nf.qm
)
REM ---------------------------
pause

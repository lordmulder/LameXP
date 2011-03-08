@echo off
REM ---------------------------
set "QTVARS=E:\Qt\4.7.2\bin\qtvars.bat"
REM ---------------------------
call "%QTVARS%"
REM ---------------------------
for %%f in (LameXP_??.ts) do (
	lrelease.exe %%f -qm ..\..\res\localization\%%~nf.qm
)
REM ---------------------------
pause

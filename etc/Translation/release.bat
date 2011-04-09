@echo off
REM ---------------------------
call "..\Deployment\_paths.bat" "..\Deployment\buildenv.txt"
call "%PATH_QTMSVC%\bin\qtvars.bat"
REM ---------------------------
for %%f in (LameXP_??.ts) do (
	lrelease.exe %%f -qm ..\..\res\localization\%%~nf.qm
)
REM ---------------------------
pause

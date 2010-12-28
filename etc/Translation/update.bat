@echo off
REM ---------------------------
set "QTVARS=E:\Qt\MSVC\4.7.1\bin\qtvars.bat"
REM ---------------------------
call "%QTVARS%"
del update.lst > NUL
REM ---------------------------
if exist update.lst (
	echo "Failed to delete old 'update.lst' file!"
	pause
	exit
)
REM ---------------------------
for %%f in (..\..\gui\*.ui) do (
	echo %%f >> update.lst
)
for %%f in (..\..\src\*.cpp) do (
	echo %%f >> update.lst
)
REM ---------------------------
for %%f in (*.ts) do (
	lupdate.exe @update.lst -ts %%f
)
REM ---------------------------
pause

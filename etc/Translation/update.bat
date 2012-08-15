@echo off
REM ---------------------------
call "..\Deployment\_paths.bat"
if exist "%PATH_QTMSVC%\bin\qtenv2.bat" call "%PATH_QTMSVC%\bin\qtenv2.bat"
if exist "%PATH_QTMSVC%\bin\qtvars.bat" call "%PATH_QTMSVC%\bin\qtvars.bat"
REM ---------------------------
del update.lst > NUL
REM ---------------------------
if exist update.lst (
	echo "Failed to delete old 'update.lst' file!"
	pause
	exit
)
echo ^<TS version="2.0" sourcelanguage="en"^>^</TS^> > Blank.ts
REM ---------------------------
for %%f in (..\..\gui\*.ui) do (
	echo %%f >> update.lst
)
for %%f in (..\..\src\*.cpp) do (
	echo %%f >> update.lst
)
for %%f in (..\..\src\*.h) do (
	echo %%f >> update.lst
)
REM ---------------------------
for %%f in (*.ts) do (
	del %%f.bak 2> NUL
	copy %%f %%f.bak
	lupdate.exe @update.lst -no-obsolete -ts %%f
)
REM ---------------------------
pause

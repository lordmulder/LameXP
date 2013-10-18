@echo off
set "LAMEXP_ERROR=1"
echo ----------------------------------------------------------------
echo Updating Translation files
echo ----------------------------------------------------------------
call "%~dp0\_paths.bat"
if not "%LAMEXP_ERROR%"=="0" GOTO:EOF
REM -----------------------------------------------------------------
set "LAMEXP_ERROR=1"
REM -----------------------------------------------------------------
call "%PATH_MSCDIR%\VC\bin\vcvars32.bat" x86
if exist "%PATH_QTMSVC%\bin\qtenv2.bat" call "%PATH_QTMSVC%\bin\qtenv2.bat"
if exist "%PATH_QTMSVC%\bin\qtvars.bat" call "%PATH_QTMSVC%\bin\qtvars.bat"
REM -----------------------------------------------------------------
set "LAMEXP_ERROR=1"
set "LST_FILE=%TEMP%\~list.%RANDOM%%RANDOM%.tmp"
echo %LST_FILE%
REM -----------------------------------------------------------------
del "%LST_FILE%" 2> NUL
for %%f in ("%~dp0\..\..\gui\*.ui") do (
	echo %%f >> "%LST_FILE%"
)
for %%f in ("%~dp0\..\..\src\*.cpp") do (
	echo %%f >> "%LST_FILE%"
)
for %%f in ("%~dp0\..\..\src\*.h") do (
	echo %%f >> "%LST_FILE%"
)
REM -----------------------------------------------------------------
for %%f in ("%~dp0\..\Translation\*.ts") do (
	del %%f.bak 2> NUL
	copy %%f %%f.bak
	lupdate.exe "@%LST_FILE%" -no-obsolete -ts %%f
)
lupdate.exe "@%LST_FILE%" -no-obsolete -pluralonly -ts "%~dp0\..\Translation\LameXP_EN.ts"
del "%LST_FILE%"
echo ----------------------------------------------------------------
for %%f in ("%~dp0\..\Translation\LameXP_??.ts") do (
	lrelease.exe %%f -qm "%~dp0\..\..\res\localization\%%~nf.qm"
)
echo ----------------------------------------------------------------
set "LST_FILE="
set "LAMEXP_ERROR=0"

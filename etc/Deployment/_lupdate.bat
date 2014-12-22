@echo off
setlocal ENABLEDELAYEDEXPANSION

echo.
echo ----------------------------------------------------------------
echo Updating Translation Files
echo ----------------------------------------------------------------
echo.

:: ---------------------------------------------------------------------------
:: SETUP PATHS
:: ---------------------------------------------------------------------------

call "%~dp0\_paths.bat"
call "%PATH_MSCDIR%\VC\bin\vcvars32.bat" x86

if exist "%PATH_QTMSVC%\bin\qtenv2.bat" (
	call "%PATH_QTMSVC%\bin\qtenv2.bat"
)

if exist "%PATH_QTMSVC%\bin\qtvars.bat" (
	call "%PATH_QTMSVC%\bin\qtvars.bat"
)

:: ---------------------------------------------------------------------------
:: GENERATE THE FILE LIST
:: ---------------------------------------------------------------------------

set "LST_FILE=%TEMP%\~list.%RANDOM%%RANDOM%.tmp"
echo %LST_FILE%
del "%LST_FILE%" 2> NUL

for %%f in ("%~dp0\..\..\gui\*.ui") do (
	echo %%f >> "%LST_FILE%"
)
for %%e in (cpp,h) do (
	for %%f in ("%~dp0\..\..\src\*.%%e") do (
		echo %%f >> "%LST_FILE%"
	)
)

:: ---------------------------------------------------------------------------
:: UPDATE TS FILES
:: ---------------------------------------------------------------------------

for %%f in ("%~dp0\..\Translation\*.ts") do (
	del %%f.bak 2> NUL
	copy %%f %%f.bak
	lupdate.exe "@%LST_FILE%" -no-obsolete -ts %%f
	if not "!ERRORLEVEL!"=="0" (
		echo. && echo Something went wrong^^!
		echo. && pause && exit
	)
)

lupdate.exe "@%LST_FILE%" -no-obsolete -pluralonly -ts "%~dp0\..\Translation\LameXP_EN.ts"
del "%LST_FILE%"

:: ---------------------------------------------------------------------------
:: GENERATE QM FILES
:: ---------------------------------------------------------------------------

for %%f in ("%~dp0\..\Translation\LameXP_??.ts") do (
	lrelease.exe %%f -qm "%~dp0\..\..\res\localization\%%~nf.qm"
	if not "!ERRORLEVEL!"=="0" (
		echo. && echo Something went wrong^^!
		echo. && pause && exit
	)
)

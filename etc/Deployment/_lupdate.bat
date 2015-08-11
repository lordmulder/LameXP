@echo off
setlocal ENABLEDELAYEDEXPANSION

"%~dp0\..\Utilities\CEcho.exe" cyan "\n==========================================================================="
"%~dp0\..\Utilities\CEcho.exe" cyan "Updating translation files..."
"%~dp0\..\Utilities\CEcho.exe" cyan "===========================================================================\n"

:: ---------------------------------------------------------------------------
:: SETUP PATHS
:: ---------------------------------------------------------------------------

call "%~dp0\_paths.bat"

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
	lupdate.exe "@%LST_FILE%" -no-obsolete -locations absolute -ts %%f
	if not "!ERRORLEVEL!"=="0" (
		"%~dp0\..\Utilities\CEcho.exe" red "\nSomething went wrong^^!\n"
		pause && exit
	)
)

lupdate.exe "@%LST_FILE%" -no-obsolete -locations absolute -pluralonly -ts "%~dp0\..\Translation\LameXP_EN.ts"
del "%LST_FILE%"

:: ---------------------------------------------------------------------------
:: GENERATE QM FILES
:: ---------------------------------------------------------------------------

for %%f in ("%~dp0\..\Translation\LameXP_??.ts") do (
	lrelease.exe %%f -qm "%~dp0\..\..\res\localization\%%~nf.qm"
	if not "!ERRORLEVEL!"=="0" (
		"%~dp0\..\Utilities\CEcho.exe" red "\nSomething went wrong^^!\n"
		pause && exit
	)
)

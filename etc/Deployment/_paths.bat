@echo off

:: ------------------------------------------
:: Paths already initialized?
:: ------------------------------------------

if "%_LAMEXP_PATHS_INITIALIZED_%"=="%DATE%" (
	goto:eof
)

:: ------------------------------------------
:: Clear Paths
:: ------------------------------------------

set "PATH_MSCDIR="
set "PATH_QTMSVC="
set "PATH_VCTOOL="
set "PATH_VCPROJ="

:: ------------------------------------------
:: Setup BUILDENV_TXT
:: ------------------------------------------

set "BUILDENV_TXT=%~dp0\buildenv.txt"
if not "%~1"=="" (
	set "BUILDENV_TXT=%~1"
)

if not exist "%BUILDENV_TXT%" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nCould not find \"buildenv.txt\" in current directory!\n\nPlease create your \"buildenv.txt\" file from 'buildenv.template.txt' first...\n"
	pause && exit
)

:: ------------------------------------------
:: Parse paths from BUILDENV_TXT
:: ------------------------------------------

for /f "tokens=2,*" %%s in (%BUILDENV_TXT%) do (
	if "%%s"=="PATH_MSCDIR" set "PATH_MSCDIR=%%~t"
	if "%%s"=="PATH_QTMSVC" set "PATH_QTMSVC=%%~t"
	if "%%s"=="PATH_VCTOOL" set "PATH_VCTOOL=%%~t"
	if "%%s"=="PATH_VCPROJ" set "PATH_VCPROJ=%%~t"
)

set "BUILDENV_TXT="

:: ------------------------------------------
:: Print all paths
:: ------------------------------------------

"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" yellow "\n========== BEGIN PATHS =========="
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" yellow "PATH_MSCDIR = \"%PATH_MSCDIR:\=\\%\""
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" yellow "PATH_QTMSVC = \"%PATH_QTMSVC:\=\\%\""
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" yellow "PATH_VCTOOL = \"%PATH_VCTOOL:\=\\%\""
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" yellow "PATH_VCPROJ = \"%PATH_VCPROJ:\=\\%\""
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" yellow "=========== END PATHS ===========\n"

:: ------------------------------------------
:: Validate Paths
:: ------------------------------------------

call:validate_path PATH_MSCDIR "%PATH_MSCDIR%\vcvarsall.bat"
call:validate_path PATH_QTMSVC "%PATH_QTMSVC%\bin\uic.exe"
call:validate_path PATH_QTMSVC "%PATH_QTMSVC%\bin\moc.exe"
call:validate_path PATH_QTMSVC "%PATH_QTMSVC%\bin\rcc.exe"
call:validate_path PATH_VCPROJ "%~dp0\..\..\%PATH_VCPROJ%"

:: ------------------------------------------
:: Locate Qt Path
:: ------------------------------------------

if exist "%PATH_QTMSVC%\bin\qtvars.bat" goto:exit_success
if exist "%PATH_QTMSVC%\bin\qtenv2.bat" goto:exit_success

"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nCould not find \"qtvars.bat\" or \"qtenv2.bat\" in your Qt path!\n\nPlease check your PATH_QTMSVC path variable and try again...\n"
pause && exit

:: ------------------------------------------
:: Validate Path
:: ------------------------------------------

:validate_path
if not exist "%~2" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nPath %1 could not be found!\n\nPlease check your %1 path variable and try again...\n"
	pause && exit
)
goto:eof

:: ------------------------------------------
:: Completed
:: ------------------------------------------

:exit_success
set "_LAMEXP_PATHS_INITIALIZED_=%DATE%"

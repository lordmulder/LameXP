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

set "PATH_UPXBIN="
set "PATH_MKNSIS="
set "PATH_MSCDIR="
set "PATH_WINSDK="
set "PATH_QTMSVC="
set "PATH_GNUPG1="
set "PATH_PANDOC="
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
	"%~dp0\..\Utilities\CEcho.exe" red "\nCould not find \"buildenv.txt\" in current directory!\n\nPlease create your \"buildenv.txt\" file from 'buildenv.template.txt' first...\n"
	pause && exit
)

:: ------------------------------------------
:: Parse paths from BUILDENV_TXT
:: ------------------------------------------

for /f "tokens=2,*" %%s in (%BUILDENV_TXT%) do (
	if "%%s"=="PATH_UPXBIN" set "PATH_UPXBIN=%%~t"
	if "%%s"=="PATH_MKNSIS" set "PATH_MKNSIS=%%~t"
	if "%%s"=="PATH_MSCDIR" set "PATH_MSCDIR=%%~t"
	if "%%s"=="PATH_WINSDK" set "PATH_WINSDK=%%~t"
	if "%%s"=="PATH_QTMSVC" set "PATH_QTMSVC=%%~t"
	if "%%s"=="PATH_GNUPG1" set "PATH_GNUPG1=%%~t"
	if "%%s"=="PATH_PANDOC" set "PATH_PANDOC=%%~t"
	if "%%s"=="PATH_VCTOOL" set "PATH_VCTOOL=%%~t"
	if "%%s"=="PATH_VCPROJ" set "PATH_VCPROJ=%%~t"
)

set "BUILDENV_TXT="

:: ------------------------------------------
:: Print all paths
:: ------------------------------------------

"%~dp0\..\Utilities\CEcho.exe" yellow "\n========== BEGIN PATHS =========="
"%~dp0\..\Utilities\CEcho.exe" yellow "PATH_UPXBIN = \"%PATH_UPXBIN:\=\\%\""
"%~dp0\..\Utilities\CEcho.exe" yellow "PATH_MKNSIS = \"%PATH_MKNSIS:\=\\%\""
"%~dp0\..\Utilities\CEcho.exe" yellow "PATH_MSCDIR = \"%PATH_MSCDIR:\=\\%\""
"%~dp0\..\Utilities\CEcho.exe" yellow "PATH_WINSDK = \"%PATH_WINSDK:\=\\%\""
"%~dp0\..\Utilities\CEcho.exe" yellow "PATH_QTMSVC = \"%PATH_QTMSVC:\=\\%\""
"%~dp0\..\Utilities\CEcho.exe" yellow "PATH_GNUPG1 = \"%PATH_GNUPG1:\=\\%\""
"%~dp0\..\Utilities\CEcho.exe" yellow "PATH_PANDOC = \"%PATH_PANDOC:\=\\%\""
"%~dp0\..\Utilities\CEcho.exe" yellow "PATH_VCTOOL = \"%PATH_VCTOOL:\=\\%\""
"%~dp0\..\Utilities\CEcho.exe" yellow "PATH_VCPROJ = \"%PATH_VCPROJ:\=\\%\""
"%~dp0\..\Utilities\CEcho.exe" yellow "=========== END PATHS ===========\n"

:: ------------------------------------------
:: Validate Paths
:: ------------------------------------------

call:validate_path PATH_UPXBIN "%PATH_UPXBIN%\upx.exe"
call:validate_path PATH_MKNSIS "%PATH_MKNSIS%\makensis.exe"
call:validate_path PATH_MSCDIR "%PATH_MSCDIR%\VC\vcvarsall.bat"
call:validate_path PATH_MSCDIR "%PATH_MSCDIR%\VC\bin\cl.exe"
call:validate_path PATH_WINSDK "%PATH_WINSDK%\Redist\ucrt\DLLs\x86\ucrtbase.dll"
call:validate_path PATH_QTMSVC "%PATH_QTMSVC%\bin\uic.exe"
call:validate_path PATH_QTMSVC "%PATH_QTMSVC%\bin\moc.exe"
call:validate_path PATH_QTMSVC "%PATH_QTMSVC%\bin\rcc.exe"
call:validate_path PATH_GNUPG1 "%PATH_GNUPG1%\gpg.exe"
call:validate_path PATH_PANDOC "%PATH_PANDOC%\pandoc.exe"
call:validate_path PATH_VCTOOL "%PATH_MSCDIR%\VC\redist\x86\Microsoft.VC%PATH_VCTOOL%.CRT\msvcp%PATH_VCTOOL%.dll"
call:validate_path PATH_VCPROJ "%~dp0\..\..\%PATH_VCPROJ%"

:: ------------------------------------------
:: Locate Qt Path
:: ------------------------------------------

if exist "%PATH_QTMSVC%\bin\qtvars.bat" goto:exit_success
if exist "%PATH_QTMSVC%\bin\qtenv2.bat" goto:exit_success

"%~dp0\..\Utilities\CEcho.exe" red "\nCould not find \"qtvars.bat\" or \"qtenv2.bat\" in your Qt path!\n\nPlease check your PATH_QTMSVC path variable and try again...\n"
pause && exit

:: ------------------------------------------
:: Validate Path
:: ------------------------------------------

:validate_path
if not exist "%~2" (
	"%~dp0\..\Utilities\CEcho.exe" red "\nPath %1 could not be found!\n\nPlease check your %1 path variable and try again...\n"
	pause && exit
)
goto:eof

:: ------------------------------------------
:: Completed
:: ------------------------------------------

:exit_success
set "_LAMEXP_PATHS_INITIALIZED_=%DATE%"

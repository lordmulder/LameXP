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
set "PATH_QTMSVC="
set "PATH_GNUPG1="
set "PATH_PANDOC="
set "PATH_VCPROJ="

:: ------------------------------------------
:: Setup BUILDENV_TXT
:: ------------------------------------------

set "BUILDENV_TXT=%~dp0\buildenv.txt"
if not "%~1"=="" (
	set "BUILDENV_TXT=%~1"
)

if not exist "%BUILDENV_TXT%" (
	echo.
	echo Could not find 'buildenv.txt' in current directory^!
	echo Please create your 'buildenv.txt' file from 'buildenv.template.txt' first.
	echo.
	pause
	exit
)

:: ------------------------------------------
:: Parse paths from BUILDENV_TXT
:: ------------------------------------------

for /f "tokens=2,*" %%s in (%BUILDENV_TXT%) do (
	if "%%s"=="PATH_UPXBIN" set "PATH_UPXBIN=%%~t"
	if "%%s"=="PATH_MKNSIS" set "PATH_MKNSIS=%%~t"
	if "%%s"=="PATH_MSCDIR" set "PATH_MSCDIR=%%~t"
	if "%%s"=="PATH_QTMSVC" set "PATH_QTMSVC=%%~t"
	if "%%s"=="PATH_GNUPG1" set "PATH_GNUPG1=%%~t"
	if "%%s"=="PATH_PANDOC" set "PATH_PANDOC=%%~t"
	if "%%s"=="PATH_VCPROJ" set "PATH_VCPROJ=%%~t"
)

set "BUILDENV_TXT="

:: ------------------------------------------
:: Print all paths
:: ------------------------------------------

echo.
echo ======= BEGIN PATHS =======
echo PATH_UPXBIN = "%PATH_UPXBIN%"
echo PATH_MKNSIS = "%PATH_MKNSIS%"
echo PATH_MSCDIR = "%PATH_MSCDIR%"
echo PATH_QTMSVC = "%PATH_QTMSVC%"
echo PATH_GNUPG1 = "%PATH_GNUPG1%"
echo PATH_PANDOC = "%PATH_PANDOC%"
echo PATH_VCPROJ = "%PATH_VCPROJ%"
echo ======== END PATHS ========
echo.

:: ------------------------------------------
:: Validate Paths
:: ------------------------------------------

call:validate_path PATH_UPXBIN "%PATH_UPXBIN%\upx.exe"
call:validate_path PATH_MKNSIS "%PATH_MKNSIS%\makensis.exe"
call:validate_path PATH_MSCDIR "%PATH_MSCDIR%\VC\vcvarsall.bat"
call:validate_path PATH_MSCDIR "%PATH_MSCDIR%\VC\bin\cl.exe"
call:validate_path PATH_QTMSVC "%PATH_QTMSVC%\bin\uic.exe"
call:validate_path PATH_QTMSVC "%PATH_QTMSVC%\bin\moc.exe"
call:validate_path PATH_QTMSVC "%PATH_QTMSVC%\bin\rcc.exe"
call:validate_path PATH_GNUPG1 "%PATH_GNUPG1%\gpg.exe"
call:validate_path PATH_PANDOC "%PATH_PANDOC%\pandoc.exe"
call:validate_path PATH_VCPROJ "%~dp0\..\..\%PATH_VCPROJ%"

:: ------------------------------------------
:: Locate Qt Path
:: ------------------------------------------

if exist "%PATH_QTMSVC%\bin\qtvars.bat" goto:exit_success
if exist "%PATH_QTMSVC%\bin\qtenv2.bat" goto:exit_success

echo. && echo Could not find "qtvars.bat" or "qtenv2.bat" in your Qt path.
echo. && echo Please check your PATH_QTMSVC and try again!
echo. && pause && exit

:: ------------------------------------------
:: Validate Path
:: ------------------------------------------

:validate_path
if not exist "%~2" (
	echo. && echo Path could not be found: && echo "%~2"
	echo. && echo Please check your %1 and try again!
	echo. && pause && exit
)
goto:eof

:: ------------------------------------------
:: Completed
:: ------------------------------------------

:exit_success
set "_LAMEXP_PATHS_INITIALIZED_=%DATE%"

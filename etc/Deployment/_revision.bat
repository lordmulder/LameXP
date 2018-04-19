@echo off

"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" cyan "\n==========================================================================="
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" cyan "Detecting Git revision..."
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" cyan "===========================================================================\n"

:: ---------------------------------------------------------------------------
:: SETUP PATHS
:: ---------------------------------------------------------------------------

call "%~dp0\_paths.bat"
cd /d "%~dp0"

:: ---------------------------------------------------------------------------
:: CLEAR VARIABLES
:: ---------------------------------------------------------------------------

set "GIT_REV_NAME="
set "GIT_REV_HASH="
set "GIT_REV_NMBR="
set "GIT_REV_DATE="
set "GIT_REV_TIME="

:: ---------------------------------------------------------------------------
:: DETECT THE GIT REVISION
:: ---------------------------------------------------------------------------

for /f "usebackq tokens=1" %%i in (`"%PATH_GITWIN%\bin\git.exe" rev-parse --abbrev-ref HEAD`) do (
	set "GIT_REV_NAME=%%~i"
)

for /f "usebackq tokens=1" %%i in (`"%PATH_GITWIN%\bin\git.exe" rev-parse --short HEAD`) do (
	set "GIT_REV_HASH=%%~i"
)

for /f "usebackq tokens=1" %%i in (`"%PATH_GITWIN%\bin\git.exe" rev-list --count HEAD`) do (
	set "GIT_REV_NMBR=%%~i"
)

for /f "usebackq tokens=1,2" %%i in (`"%PATH_GITWIN%\bin\git.exe" log -1 --format^=%%ci HEAD`) do (
	set "GIT_REV_DATE=%%~i"
	set "GIT_REV_TIME=%%~j"
)

:: ------------------------------------------
:: VALIDATE RESULT
:: ------------------------------------------

if "%GIT_REV_NAME%"=="" goto git_rev_incomplete
if "%GIT_REV_HASH%"=="" goto git_rev_incomplete
if "%GIT_REV_NMBR%"=="" goto git_rev_incomplete
if "%GIT_REV_DATE%"=="" goto git_rev_incomplete
if "%GIT_REV_TIME%"=="" goto git_rev_incomplete
goto git_rev_complete

:git_rev_incomplete
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nFailed to determine the current Git revision!\n"
pause && exit

:: ------------------------------------------
:: Completed
:: ------------------------------------------

:git_rev_complete
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" yellow "Git revision: %GIT_REV_NAME%+%GIT_REV_NMBR%-%GIT_REV_HASH% [%GIT_REV_DATE%]"

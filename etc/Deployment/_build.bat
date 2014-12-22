@echo off

echo.
echo ----------------------------------------------------------------
echo Solution File: %1
echo Configuration: %~2
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
:: BUILD THE PROJECT
:: ---------------------------------------------------------------------------

msbuild.exe /property:Configuration=%3 /property:Platform=%2 /target:Clean   /verbosity:normal "%~1"
if not "%ERRORLEVEL%"=="0" (
	echo. && echo Build process has failed!
	echo. && pause && exit
)

msbuild.exe /property:Configuration=%3 /property:Platform=%2 /target:Rebuild /verbosity:normal "%~1"
if not "%ERRORLEVEL%"=="0" (
	echo. && echo Build process has failed!
	echo. && pause && exit
)

msbuild.exe /property:Configuration=%3 /property:Platform=%2 /target:Build   /verbosity:normal "%~1"
if not "%ERRORLEVEL%"=="0" (
	echo. && echo Build process has failed!
	echo. && pause && exit
)

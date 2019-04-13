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
call "%PATH_MSCDIR%\vcvars32.bat"

if exist "%PATH_QTMSVC%\bin\qtenv2.bat" (
	call "%PATH_QTMSVC%\bin\qtenv2.bat"
)

if exist "%PATH_QTMSVC%\bin\qtvars.bat" (
	call "%PATH_QTMSVC%\bin\qtvars.bat"
)

:: ---------------------------------------------------------------------------
:: BUILD THE PROJECT
:: ---------------------------------------------------------------------------

if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
	set PreferredToolArchitecture=x64
)

msbuild.exe /property:Configuration=%3 /property:Platform=%2 /target:Clean   /verbosity:normal "%~1"
if not "%ERRORLEVEL%"=="0" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nBuild process has failed!\n"
	pause && exit
)

msbuild.exe /property:Configuration=%3 /property:Platform=%2 /target:Rebuild /verbosity:normal "%~1"
if not "%ERRORLEVEL%"=="0" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nBuild process has failed!\n"
	pause && exit
)

msbuild.exe /property:Configuration=%3 /property:Platform=%2 /target:Build   /verbosity:normal "%~1"
if not "%ERRORLEVEL%"=="0" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nBuild process has failed!\n"
	pause && exit
)

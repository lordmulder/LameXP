@echo off
set "LAMEXP_ERROR=1"
echo ----------------------------------------------------------------
echo Solution File: %1
echo Configuration: %~n2
echo ----------------------------------------------------------------
call _paths.bat
call "%PATH_MSVC90%\VC\bin\vcvars32.bat"
call "%PATH_QTMSVC%\bin\qtvars.bat"
REM -----------------------------------------------------------------
msbuild.exe /property:Configuration=%~n2 /target:Clean /verbosity:d %1
if exist "%~d1%~p1bin\%~n2\*.exe" GOTO:EOF
if exist "%~d1%~p1obj\%~n2\*.obj" GOTO:EOF
echo ----------------------------------------------------------------
msbuild.exe /property:Configuration=%~n2 /target:Rebuild /verbosity:d %1
echo ----------------------------------------------------------------
if not exist "%~d1%~p1bin\%~n2\%~n1.exe" GOTO:EOF
REM -----------------------------------------------------------------
set "LAMEXP_ERROR=0"

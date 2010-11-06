@echo off
call _paths.bat
echo ----------------------------------------------------------------
echo Solution File: %1
echo Configuration: %~n2
echo ----------------------------------------------------------------
call "%PATH_MSVC90%\VC\bin\vcvars32.bat"
call "%PATH_QTMSVC%\bin\qtvars.bat"
msbuild.exe /property:Configuration=%~n2 /target:Clean,Rebuild /verbosity:d %1

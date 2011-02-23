@echo off
set "LAMEXP_ERROR=1"
echo ----------------------------------------------------------------
echo Updating Translation files
echo ----------------------------------------------------------------
call _paths.bat
if not "%LAMEXP_ERROR%"=="0" GOTO:EOF
REM -----------------------------------------------------------------
call "%PATH_MSVC90%\VC\bin\vcvars32.bat" x86
call "%PATH_QTMSVC%\bin\qtvars.bat"
REM -----------------------------------------------------------------
set "LAMEXP_ERROR=1"
set "LST_FILE=%TEMP%\~list.%DATE:~6,4%-%DATE:~3,2%-%DATE:~0,2%.lst"
echo %LST_FILE%
REM -----------------------------------------------------------------
del "%LST_FILE%" 2> NUL
for %%f in (..\..\gui\*.ui) do (
	echo %%f >> "%LST_FILE%"
)
for %%f in (..\..\src\*.cpp) do (
	echo %%f >> "%LST_FILE%"
)
for %%f in (..\..\src\*.h) do (
	echo %%f >> "%LST_FILE%"
)
REM -----------------------------------------------------------------
for %%f in (..\Translation\*.ts) do (
	del %%f.bak 2> NUL
	copy %%f %%f.bak
	lupdate.exe "@%LST_FILE%" -no-obsolete -ts %%f
)
del "%LST_FILE%"
echo ----------------------------------------------------------------
for %%f in (..\Translation\LameXP_??.ts) do (
	lrelease.exe %%f -qm ..\..\res\localization\%%~nf.qm
)
echo ----------------------------------------------------------------
set "LST_FILE="
set "LAMEXP_ERROR=0"

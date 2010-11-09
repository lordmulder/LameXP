@echo off
call _paths.bat
REM ------------------------------------------
set "OUT_PATH=..\..\bin\Release"
set "OUT_DATE=%DATE:~6,4%-%DATE:~3,2%-%DATE:~0,2%"
set "OUT_FILE=%OUT_PATH%\..\LameXP.%OUT_DATE%.Release"
set "TMP_PATH=%TEMP%\~LameXP.%OUT_DATE%.tmp"
REM ------------------------------------------
set "VER_LAMEXP_MAJOR=X"
set "VER_LAMEXP_MINOR_HI=X"
set "VER_LAMEXP_MINOR_LO=X"
set "VER_LAMEXP_BUILD=X"
set "VER_LAMEXP_SUFFIX=X"
REM ------------------------------------------
FOR /F "tokens=2,3" %%s IN (..\..\src\Resource.h) DO (
	if "%%s"=="VER_LAMEXP_MAJOR" set "VER_LAMEXP_MAJOR=%%t"
	if "%%s"=="VER_LAMEXP_MINOR_HI" set "VER_LAMEXP_MINOR_HI=%%t"
	if "%%s"=="VER_LAMEXP_MINOR_LO" set "VER_LAMEXP_MINOR_LO=%%t"
	if "%%s"=="VER_LAMEXP_BUILD" set "VER_LAMEXP_BUILD=%%t"
	if "%%s"=="VER_LAMEXP_SUFFIX" set "VER_LAMEXP_SUFFIX=%%t"
)
REM ------------------------------------------
echo Version: %VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO%, Build #%VER_LAMEXP_BUILD% (%VER_LAMEXP_SUFFIX%)
REM ------------------------------------------
del "%OUT_FILE%.exe"
del "%OUT_FILE%.zip"
if exist "%OUT_FILE%.exe" (
	echo BUILD HAS FAILED !!!
	pause
	exit
)
if exist "%OUT_FILE%.zip" (
	echo BUILD HAS FAILED !!!
	pause
	exit
)
REM ------------------------------------------
call _build.bat "..\..\LameXP.sln" Release
REM ------------------------------------------
if not "%LAMEXP_BUILD_SUCCESS%"=="YES" (
	echo.
	echo BUILD HAS FAILED !!!
	echo.
	pause
	exit
)
REM ------------------------------------------
rd /S /Q "%TMP_PATH%"
mkdir "%TMP_PATH%"
mkdir "%TMP_PATH%\imageformats"
REM ------------------------------------------
copy "%OUT_PATH%\*.exe" "%TMP_PATH%"
copy "%QTDIR%\bin\QtCore4.dll" "%TMP_PATH%"
copy "%QTDIR%\bin\QtGui4.dll" "%TMP_PATH%"
copy "%QTDIR%\bin\QtXml4.dll" "%TMP_PATH%"
copy "%QTDIR%\bin\QtSvg4.dll" "%TMP_PATH%"
copy "%QTDIR%\plugins\imageformats\q???4.dll" "%TMP_PATH%\imageformats"
REM ------------------------------------------
for %%f in ("%TMP_PATH%\*.exe") do (
	"%PATH_UPXBIN%" --best --lzma "%%f"
)
for %%f in ("%TMP_PATH%\*.dll") do (
	"%PATH_UPXBIN%" --best --lzma "%%f"
)
REM ------------------------------------------
copy "..\Redist\*.*" "%TMP_PATH%"
copy "..\..\License.txt" "%TMP_PATH%"
REM ------------------------------------------
"%PATH_SEVENZ%" a -tzip -r "%OUT_FILE%.zip" "%TMP_PATH%\*"
"%PATH_MKNSIS%" "/DLAMEXP_SOURCE_PATH=%TMP_PATH%" "/DLAMEXP_OUTPUT_FILE=%OUT_FILE%.exe" "/DLAMEXP_DATE=%OUT_DATE%" "/DLAMEXP_VERSION=%VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO%" "/DLAMEXP_BUILD=%VER_LAMEXP_BUILD%" "/DLAMEXP_SUFFIX=%VER_LAMEXP_SUFFIX%" "..\NSIS\setup.nsi"
rd /S /Q "%TMP_PATH%"
REM ------------------------------------------
pause

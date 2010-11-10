@echo off
call _paths.bat
REM ------------------------------------------
set "OUT_PATH=..\..\bin\Release"
set "OUT_DATE=%DATE:~6,4%-%DATE:~3,2%-%DATE:~0,2%"
set "OUT_FILE=%OUT_PATH%\..\LameXP.%OUT_DATE%.Release"
set "TMP_PATH=%TEMP%\~LameXP.%OUT_DATE%.tmp"
REM ------------------------------------------
call _version.bat
if not "%LAMEXP_ERROR%"=="0" (
	call _error.bat	"FAILD TO READ VERSION INFO!"
	GOTO:EOF
)
REM ------------------------------------------
del "%OUT_FILE%.exe"
del "%OUT_FILE%.zip"
if exist "%OUT_FILE%.exe" (
	call _error.bat	"FAILD TO DELET EXISTING FILE"
	GOTO:EOF
)
if exist "%OUT_FILE%.zip" (
	call _error.bat	"FAILD TO DELET EXISTING FILE"
	GOTO:EOF
)
REM ------------------------------------------
call _build.bat "..\..\LameXP.sln" Release
if not "%LAMEXP_ERROR%"=="0" (
	call _error.bat	"BUILD HAS FAILED"
	GOTO:EOF
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
if not exist "%OUT_FILE%.zip" (
	call _error.bat	"PACKAGING HAS FAILED"
	GOTO:EOF
)
if not exist "%OUT_FILE%.exe" (
	call _error.bat	"PACKAGING HAS FAILED"
	GOTO:EOF
)
REM ------------------------------------------
echo.
echo BUIDL COMPLETED SUCCESSFULLY :-)
echo.
REM ------------------------------------------
pause

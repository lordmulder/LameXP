@echo off
setlocal ENABLEDELAYEDEXPANSION
REM ------------------------------------------
REM :: SETUP ENVIRONMENT ::
REM ------------------------------------------
call _paths.bat
if not "%LAMEXP_ERROR%"=="0" (
	call _error.bat	"FAILD TO SETUP PATHS. CHECK YOUR 'BUILDENV.TXT' FILE"
	GOTO:EOF
)
REM ------------------------------------------
if "%LAMEXP_CONFIG%"=="" (
	set "LAMEXP_CONFIG=Release"
)
if not "%LAMEXP_REDIST%"=="0" (
	set "LAMEXP_REDIST=1"
)
REM ------------------------------------------
REM :: SETUP PATHS ::
REM ------------------------------------------
set "OUT_PATH=..\..\bin\%LAMEXP_CONFIG%"
set "OUT_DATE=%DATE:~6,4%-%DATE:~3,2%-%DATE:~0,2%"
set "TMP_PATH=%TEMP%\~LameXP.%LAMEXP_CONFIG%.%OUT_DATE%.tmp"
set "OBJ_PATH=..\..\obj\%LAMEXP_CONFIG%"
set "MOC_PATH=..\..\tmp"
REM ------------------------------------------
REM goto SkipBuildThisTime
REM ------------------------------------------
REM :: CLEAN UP ::
REM ------------------------------------------
del /Q "%OUT_PATH%\*.exe"
del /Q "%OUT_PATH%\*.dll"
del /Q "%OBJ_PATH%\*.obj"
del /Q "%OBJ_PATH%\*.res"
del /Q "%OBJ_PATH%\*.bat"
del /Q "%OBJ_PATH%\*.idb"
del /Q "%MOC_PATH%\*.cpp"
del /Q "%MOC_PATH%\*.h"
REM ------------------------------------------
REM :: BUILD BINARIES ::
REM ------------------------------------------
call _lupdate.bat
call _build.bat "..\..\LameXP.sln" "%LAMEXP_CONFIG%"
if not "%LAMEXP_ERROR%"=="0" (
	call _error.bat	"BUILD HAS FAILED"
	GOTO:EOF
)
REM ------------------------------------------
REM :SkipBuildThisTime
REM ------------------------------------------
REM :: READ VERSION INFO ::
REM ------------------------------------------
call _version.bat
if not "%LAMEXP_ERROR%"=="0" (
	call _error.bat	"FAILD TO READ VERSION INFO!"
	GOTO:EOF
)
REM ------------------------------------------
set "OUT_FILE=%OUT_PATH%\..\LameXP.%OUT_DATE%.%LAMEXP_CONFIG:_=-%.Build-%VER_LAMEXP_BUILD%"
for /L %%n in (1, 1, 99) do (
	if exist "!OUT_FILE!.exe" set "OUT_FILE=%OUT_PATH%\..\LameXP.%OUT_DATE%.%LAMEXP_CONFIG:_=-%.Build-%VER_LAMEXP_BUILD%.Update-%%n"
	if exist "!OUT_FILE!.zip" set "OUT_FILE=%OUT_PATH%\..\LameXP.%OUT_DATE%.%LAMEXP_CONFIG:_=-%.Build-%VER_LAMEXP_BUILD%.Update-%%n"
)
REM ------------------------------------------
REM :: DELETE OLD OUTPUT FILE ::
REM ------------------------------------------
del "%OUT_FILE%.exe"
del "%OUT_FILE%.zip"
REM ------------------------------------------
if exist "%OUT_FILE%.exe" (
	call _error.bat	"FAILD TO DELET EXISTING FILE"
	GOTO:EOF
)
if exist "%OUT_FILE%.zip" (
	call _error.bat	"FAILD TO DELET EXISTING FILE"
	GOTO:EOF
)
REM ------------------------------------------
REM :: POST BUILD ::
REM ------------------------------------------
rd /S /Q "%TMP_PATH%"
mkdir "%TMP_PATH%"
copy "%OUT_PATH%\*.exe" "%TMP_PATH%"
REM ------------------------------------------
if "%LAMEXP_REDIST%"=="1" (
	copy "%QTDIR%\bin\QtCore4.dll" "%TMP_PATH%"
	copy "%QTDIR%\bin\QtGui4.dll" "%TMP_PATH%"
	copy "%QTDIR%\bin\QtXml4.dll" "%TMP_PATH%"
	copy "%QTDIR%\bin\QtSvg4.dll" "%TMP_PATH%"
	mkdir "%TMP_PATH%\imageformats"
	copy "%QTDIR%\plugins\imageformats\q???4.dll" "%TMP_PATH%\imageformats"
)
REM ------------------------------------------
for %%f in ("%TMP_PATH%\*.exe") do (
	"%PATH_UPXBIN%\upx.exe" --best "%%f"
)
for %%f in ("%TMP_PATH%\*.dll") do (
	"%PATH_UPXBIN%\upx.exe" --best "%%f"
)
REM ------------------------------------------
if exist _postproc.bat (
	call _postproc.bat "%TMP_PATH%"
)
REM ------------------------------------------
if "%LAMEXP_REDIST%"=="1" (
	copy "..\Redist\*.*" "%TMP_PATH%"
)
copy "..\..\ReadMe.txt" "%TMP_PATH%"
copy "..\..\License.txt" "%TMP_PATH%"
copy "..\..\doc\Changelog.html" "%TMP_PATH%"
copy "..\..\doc\Translate.html" "%TMP_PATH%"
copy "..\..\doc\FAQ.html" "%TMP_PATH%"
REM ------------------------------------------
REM :: CREATE PACKAGES ::
REM ------------------------------------------
"%PATH_SEVENZ%\7z.exe" a -tzip -r "%OUT_FILE%.zip" "%TMP_PATH%\*"
"%PATH_MKNSIS%\makensis.exe" "/DLAMEXP_SOURCE_PATH=%TMP_PATH%" "/DLAMEXP_OUTPUT_FILE=%OUT_FILE%.exe" "/DLAMEXP_UPX_PATH=%PATH_UPXBIN%" "/DLAMEXP_DATE=%OUT_DATE%" "/DLAMEXP_VERSION=%VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO%" "/DLAMEXP_BUILD=%VER_LAMEXP_BUILD%" "/DLAMEXP_SUFFIX=%VER_LAMEXP_SUFFIX%" "..\NSIS\setup.nsi"
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
attrib +R "%OUT_FILE%.zip"
attrib +R "%OUT_FILE%.exe"
REM ------------------------------------------
REM :: CREATE SIGNATURE ::
REM ------------------------------------------
"%PATH_GNUPG1%\gpg.exe" --detach-sign "%OUT_FILE%.exe"
attrib +R "%OUT_FILE%.exe.sig"
REM ------------------------------------------
echo.
echo BUIDL COMPLETED SUCCESSFULLY :-)
echo.
REM ------------------------------------------
pause

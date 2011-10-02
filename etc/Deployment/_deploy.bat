@echo off
setlocal ENABLEDELAYEDEXPANSION
REM ------------------------------------------
REM :: SETUP ENVIRONMENT ::
REM ------------------------------------------
call "%~dp0\_paths.bat"
if not "%LAMEXP_ERROR%"=="0" (
	call "%~dp0\_error.bat" "FAILD TO SETUP PATHS. CHECK YOUR 'BUILDENV.TXT' FILE"
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
REM :: SETUP BUILD DATE ::
REM ------------------------------------------
call "%~dp0\_date.bat"
if not "%LAMEXP_ERROR%"=="0" (
	call "%~dp0\_error.bat" "FAILD TO SETUP BUILD-DATE"
	GOTO:EOF
)
REM ------------------------------------------
REM :: SETUP PATHS ::
REM ------------------------------------------
set "OUT_PATH=%~dp0\..\..\bin\%LAMEXP_CONFIG%"
set "TMP_PATH=%TEMP%\~LameXP.%LAMEXP_CONFIG%.%ISO_DATE%.%RANDOM%.tmp"
set "OBJ_PATH=%~dp0\..\..\obj\%LAMEXP_CONFIG%"
set "MOC_PATH=%~dp0\..\..\tmp"
set "IPC_PATH=%~dp0\..\..\ipch"
REM ------------------------------------------
if "%LAMEXP_SKIP_BUILD%"=="YES" (
	goto SkipBuildThisTime
)
REM ------------------------------------------
REM :: CLEAN UP ::
REM ------------------------------------------
del /Q "%OUT_PATH%\*.exe"
del /Q "%OUT_PATH%\*.dll"
del /Q "%OBJ_PATH%\*.obj"
del /Q "%OBJ_PATH%\*.res"
del /Q "%OBJ_PATH%\*.bat"
del /Q "%OBJ_PATH%\*.idb"
del /Q "%OBJ_PATH%\*.log"
del /Q "%OBJ_PATH%\*.manifest"
del /Q "%OBJ_PATH%\*.lastbuildstate"
del /Q "%OBJ_PATH%\*.htm"
del /Q "%OBJ_PATH%\*.dep"
del /Q "%MOC_PATH%\*.cpp"
del /Q "%MOC_PATH%\*.h"
del /Q /S "%IPC_PATH%\*.*"
REM ------------------------------------------
REM :: BUILD BINARIES ::
REM ------------------------------------------
call "%~dp0\_lupdate.bat"
call "%~dp0\_build.bat" "%~dp0\..\..\%PATH_VCPROJ%" "%LAMEXP_CONFIG%"
if not "%LAMEXP_ERROR%"=="0" (
	call "%~dp0\_error.bat" "BUILD HAS FAILED"
	GOTO:EOF
)
REM ------------------------------------------
:SkipBuildThisTime
REM ------------------------------------------
REM :: READ VERSION INFO ::
REM ------------------------------------------
call "%~dp0\_version.bat"
if not "%LAMEXP_ERROR%"=="0" (
	call "%~dp0\_error.bat" "FAILD TO READ VERSION INFO!"
	GOTO:EOF
)
REM ------------------------------------------
set "OUT_FILE=%OUT_PATH%\..\LameXP.%ISO_DATE%.%LAMEXP_CONFIG:_=-%.Build-%VER_LAMEXP_BUILD%"
for /L %%n in (1, 1, 99) do (
	if exist "!OUT_FILE!.exe" set "OUT_FILE=%OUT_PATH%\..\LameXP.%ISO_DATE%.%LAMEXP_CONFIG:_=-%.Build-%VER_LAMEXP_BUILD%.Update-%%n"
	if exist "!OUT_FILE!.zip" set "OUT_FILE=%OUT_PATH%\..\LameXP.%ISO_DATE%.%LAMEXP_CONFIG:_=-%.Build-%VER_LAMEXP_BUILD%.Update-%%n"
)
REM ------------------------------------------
REM :: DELETE OLD OUTPUT FILE ::
REM ------------------------------------------
del "%OUT_FILE%.exe"
del "%OUT_FILE%.sfx.exe"
del "%OUT_FILE%.zip"
REM ------------------------------------------
if exist "%OUT_FILE%.exe" (
	call "%~dp0\_error.bat" "FAILD TO DELET EXISTING FILE"
	GOTO:EOF
)
if exist "%OUT_FILE%.zip" (
	call "%~dp0\_error.bat" "FAILD TO DELET EXISTING FILE"
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
if exist "%~dp0\_postproc.bat" (
	call "%~dp0\_postproc.bat" "%TMP_PATH%"
)
REM ------------------------------------------
if "%LAMEXP_REDIST%"=="1" (
	copy "..\Redist\*.*" "%TMP_PATH%"
)
copy "%~dp0\..\..\ReadMe.txt" "%TMP_PATH%"
copy "%~dp0\..\..\License.txt" "%TMP_PATH%"
copy "%~dp0\..\..\doc\Changelog.html" "%TMP_PATH%"
copy "%~dp0\..\..\doc\Translate.html" "%TMP_PATH%"
copy "%~dp0\..\..\doc\FAQ.html" "%TMP_PATH%"
REM ------------------------------------------
REM :: CREATE PACKAGES ::
REM ------------------------------------------
"%PATH_SEVENZ%\7z.exe" a -tzip -r "%OUT_FILE%.zip" "%TMP_PATH%\*"
"%PATH_MKNSIS%\makensis.exe" "/DLAMEXP_UPX_PATH=%PATH_UPXBIN%" "/DLAMEXP_DATE=%ISO_DATE%" "/DLAMEXP_VERSION=%VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO%" "/DLAMEXP_BUILD=%VER_LAMEXP_BUILD%" "/DLAMEXP_INSTTYPE=%VER_LAMEXP_TYPE%" "/DLAMEXP_PATCH=%VER_LAMEXP_PATCH%" "/DLAMEXP_SOURCE_PATH=%TMP_PATH%" "/DLAMEXP_OUTPUT_FILE=%OUT_FILE%.exe" "%~dp0\..\NSIS\setup.nsi"
"%PATH_MKNSIS%\makensis.exe" "/DLAMEXP_UPX_PATH=%PATH_UPXBIN%" "/DLAMEXP_DATE=%ISO_DATE%" "/DLAMEXP_VERSION=%VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO%" "/DLAMEXP_BUILD=%VER_LAMEXP_BUILD%" "/DLAMEXP_INSTTYPE=%VER_LAMEXP_TYPE%" "/DLAMEXP_PATCH=%VER_LAMEXP_PATCH%" "/DLAMEXP_SOURCE_FILE=%OUT_FILE%.exe" "/DLAMEXP_OUTPUT_FILE=%OUT_FILE%.sfx.exe" "%~dp0\..\NSIS\wrapper.nsi"
rd /S /Q "%TMP_PATH%"
REM ------------------------------------------
if not exist "%OUT_FILE%.zip" (
	call "%~dp0\_error.bat" "PACKAGING HAS FAILED"
	GOTO:EOF
)
if not exist "%OUT_FILE%.exe" (
	call "%~dp0\_error.bat" "PACKAGING HAS FAILED"
	GOTO:EOF
)
REM ------------------------------------------
attrib +R "%OUT_FILE%.zip"
attrib +R "%OUT_FILE%.exe"
attrib +R "%OUT_FILE%.sfx.exe"
REM ------------------------------------------
REM :: CREATE SIGNATURE ::
REM ------------------------------------------
"%PATH_GNUPG1%\gpg.exe" --detach-sign "%OUT_FILE%.sfx.exe"
attrib +R "%OUT_FILE%.exe.sig"
REM ------------------------------------------
echo.
echo BUIDL COMPLETED SUCCESSFULLY :-)
echo.
REM ------------------------------------------
pause

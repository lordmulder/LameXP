@echo off
setlocal ENABLEDELAYEDEXPANSION

:: ---------------------------------------------------------------------------
:: SETUP ENVIRONMENT
:: ---------------------------------------------------------------------------

set "_LAMEXP_PATHS_INITIALIZED_="

call "%~dp0\_paths.bat"
call "%~dp0\_date.bat"

if "%LAMEXP_CONFIG%"=="" (
	set "LAMEXP_CONFIG=Release"
)

if "%LAMEXP_PLATFORM%"=="" (
	set "LAMEXP_PLATFORM=Win32"
)

if not "%LAMEXP_REDIST%"=="0" (
	set "LAMEXP_REDIST=1"
)

:: ---------------------------------------------------------------------------
:: SETUP PATHS
:: ---------------------------------------------------------------------------

set "BIN_PATH=%~dp0\..\..\bin\%LAMEXP_PLATFORM%\%LAMEXP_CONFIG%"
set "TMP_PATH=%TEMP%\~LameXP.%LAMEXP_CONFIG%.%ISO_DATE%.%RANDOM%.tmp"

if "%LAMEXP_SKIP_BUILD%"=="YES" (
	goto SkipBuildThisTime
)

:: ---------------------------------------------------------------------------
:: CLEAN UP
:: ---------------------------------------------------------------------------

"%~dp0\..\Utilities\CEcho.exe" cyan "\n==========================================================================="
"%~dp0\..\Utilities\CEcho.exe" cyan "Cleaning up..."
"%~dp0\..\Utilities\CEcho.exe" cyan "===========================================================================\n"

for %%i in (bin,obj,tmp,ipch) do (
	del /Q /S /F "%~dp0\..\..\%%i\*.*"
)

:: ---------------------------------------------------------------------------
:: UPDATE LANGUAGE FILES AND DCOS
:: ---------------------------------------------------------------------------

call "%~dp0\_mkdocs.bat"
call "%~dp0\_lupdate.bat"

:: ---------------------------------------------------------------------------
:: BUILD THE BINARIES
:: ---------------------------------------------------------------------------

call "%~dp0\_build.bat" "%~dp0\..\..\%PATH_VCPROJ%" "%LAMEXP_PLATFORM%" "%LAMEXP_CONFIG%"

:SkipBuildThisTime

:: ---------------------------------------------------------------------------
:: READ VERSION INFO
:: ---------------------------------------------------------------------------

call "%~dp0\_version.bat"

:: ---------------------------------------------------------------------------
:: GENERATE OUTPUT FILE NAME
:: ---------------------------------------------------------------------------

mkdir "%~dp0\..\..\out" 2> NUL
set "OUT_FILE=%~dp0\..\..\out\%VER_LAMEXP_BASENAME%.%ISO_DATE%.%LAMEXP_CONFIG:_=-%.Build-%VER_LAMEXP_BUILD%"
for /L %%n in (1, 1, 99) do (
	if exist "!OUT_FILE!.exe" set "OUT_FILE=%~dp0\..\..\out\%VER_LAMEXP_BASENAME%.%ISO_DATE%.%LAMEXP_CONFIG:_=-%.Build-%VER_LAMEXP_BUILD%.Update-%%n"
	if exist "!OUT_FILE!.zip" set "OUT_FILE=%~dp0\..\..\out\%VER_LAMEXP_BASENAME%.%ISO_DATE%.%LAMEXP_CONFIG:_=-%.Build-%VER_LAMEXP_BUILD%.Update-%%n"
)

:: ---------------------------------------------------------------------------
:: DELETE OLD OUTPUT FILE
:: ---------------------------------------------------------------------------

for %%i in (exe,sfx,zip,txt) do (
	del "%OUT_FILE%.%%i" 2> NUL
	if exist "%OUT_FILE%.%%i" (
		"%~dp0\..\Utilities\CEcho.exe" red "\nFailed to delete existing output file^^!\n"
		pause && exit
	)
)

:: ---------------------------------------------------------------------------
:: COPY BINARY FILES AND REDIST
:: ---------------------------------------------------------------------------

"%~dp0\..\Utilities\CEcho.exe" cyan "\n==========================================================================="
"%~dp0\..\Utilities\CEcho.exe" cyan "Copying binary files..."
"%~dp0\..\Utilities\CEcho.exe" cyan "===========================================================================\n"

rd /S /Q "%TMP_PATH%"
mkdir "%TMP_PATH%"

for %%i in (exe,dll) do (
	copy "%BIN_PATH%\*.%%i" "%TMP_PATH%"
)

if "%LAMEXP_REDIST%"=="1" (
	mkdir "%TMP_PATH%\imageformats"
	for %%i in (Core,Gui,Network,Xml,Svg) do (
		copy "%QTDIR%\bin\Qt%%i4.dll" "%TMP_PATH%"
	)
	copy "%QTDIR%\plugins\imageformats\q???4.dll" "%TMP_PATH%\imageformats"
	for %%i in (100,110,120) do (
		if exist "%PATH_MSCDIR%\VC\redist\x86\Microsoft.VC%%i.CRT\*.dll" (
			copy "%PATH_MSCDIR%\VC\redist\x86\Microsoft.VC%%i.CRT\*.dll" "%TMP_PATH%"
		)
	)
)

for %%e in (exe,dll) do (
	for %%f in (%TMP_PATH%\*.%%e) do (
		"%PATH_UPXBIN%\upx.exe" --best "%%f"
	)
)

if exist "%~dp0\_postproc.bat" (
	call "%~dp0\_postproc.bat" "%TMP_PATH%"
)

copy "%~dp0\..\..\ReadMe.txt"         "%TMP_PATH%"
copy "%~dp0\..\..\License.txt"        "%TMP_PATH%"
copy "%~dp0\..\..\Copying.txt"        "%TMP_PATH%"
copy "%~dp0\..\..\doc\Changelog.html" "%TMP_PATH%"
copy "%~dp0\..\..\doc\Translate.html" "%TMP_PATH%"
copy "%~dp0\..\..\doc\Manual.html"    "%TMP_PATH%"
copy "%~dp0\..\..\doc\FAQ.html"       "%TMP_PATH%"

if not "%VER_LAMEXP_TYPE%" == "Final" (
	if not "%VER_LAMEXP_TYPE%" == "Hotfix" (
		copy "%~dp0\..\..\doc\PRE_RELEASE_INFO.txt" "%TMP_PATH%"
	)
)

attrib +R "%TMP_PATH%\*.txt"
attrib +R "%TMP_PATH%\*.html"
attrib +R "%TMP_PATH%\*.exe"

:: ---------------------------------------------------------------------------
:: CREATE PACKAGES
:: ---------------------------------------------------------------------------

"%~dp0\..\Utilities\CEcho.exe" cyan "\n==========================================================================="
"%~dp0\..\Utilities\CEcho.exe" cyan "Creating release packages..."
"%~dp0\..\Utilities\CEcho.exe" cyan "===========================================================================\n"

"%~dp0\..\Utilities\Echo.exe" LameXP - Audio Encoder Front-End > "%OUT_FILE%.txt"
"%~dp0\..\Utilities\Echo.exe" v%VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO% %VER_LAMEXP_TYPE%-%VER_LAMEXP_PATCH% (Build #%VER_LAMEXP_BUILD%)\n >> "%OUT_FILE%.txt"
"%~dp0\..\Utilities\Echo.exe" Built on %ISO_DATE% at %TIME%\n\n >> "%OUT_FILE%.txt"
"%~dp0\..\Utilities\Echo.exe" ---------------------------\nREADME.TXT\n--------------------------- >> "%OUT_FILE%.txt"
"%~dp0\..\Utilities\Cat.exe" "%~dp0\..\..\ReadMe.txt" >> "%OUT_FILE%.txt"
"%~dp0\..\Utilities\Echo.exe" \n\n---------------------------\nLICENSE.TXT\n---------------------------\n >> "%OUT_FILE%.txt"
"%~dp0\..\Utilities\Cat.exe" "%~dp0\..\..\License.txt" >> "%OUT_FILE%.txt"

pushd "%TMP_PATH%"
"%~dp0\..\Utilities\Zip.exe" -r -9 -z "%OUT_FILE%.zip" "*.*" < "%OUT_FILE%.txt"
popd

"%PATH_MKNSIS%\makensis.exe" "/DLAMEXP_UPX_PATH=%PATH_UPXBIN%" "/DLAMEXP_DATE=%ISO_DATE%" "/DLAMEXP_VERSION=%VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO%" "/DLAMEXP_BUILD=%VER_LAMEXP_BUILD%" "/DLAMEXP_INSTTYPE=%VER_LAMEXP_TYPE%" "/DLAMEXP_PATCH=%VER_LAMEXP_PATCH%" "/DLAMEXP_OUTPUT_FILE=%OUT_FILE%.sfx" "/DLAMEXP_SOURCE_PATH=%TMP_PATH%" "%~dp0\..\NSIS\setup.nsi"
"%PATH_MKNSIS%\makensis.exe" "/DLAMEXP_UPX_PATH=%PATH_UPXBIN%" "/DLAMEXP_DATE=%ISO_DATE%" "/DLAMEXP_VERSION=%VER_LAMEXP_MAJOR%.%VER_LAMEXP_MINOR_HI%%VER_LAMEXP_MINOR_LO%" "/DLAMEXP_BUILD=%VER_LAMEXP_BUILD%" "/DLAMEXP_INSTTYPE=%VER_LAMEXP_TYPE%" "/DLAMEXP_PATCH=%VER_LAMEXP_PATCH%" "/DLAMEXP_OUTPUT_FILE=%OUT_FILE%.exe" "/DLAMEXP_SOURCE_FILE=%OUT_FILE%.sfx" "%~dp0\..\NSIS\wrapper.nsi"

attrib -R "%TMP_PATH%\*.txt"
attrib -R "%TMP_PATH%\*.html"
attrib -R "%TMP_PATH%\*.exe"
rd /S /Q "%TMP_PATH%"

for %%i in (zip,exe) do (
	if not exist "%OUT_FILE%.zip" (
		"%~dp0\..\Utilities\CEcho.exe" red "\nFailed to create release packages^^!\n"
		pause && exit
	)
)

attrib +R "%OUT_FILE%.zip"
attrib +R "%OUT_FILE%.sfx"
attrib +R "%OUT_FILE%.exe"

:: ---------------------------------------------------------------------------
:: SIGN OUTPUT FILE
:: ---------------------------------------------------------------------------

"%~dp0\..\Utilities\CEcho.exe" cyan "\n==========================================================================="
"%~dp0\..\Utilities\CEcho.exe" cyan "Signing output file..."
"%~dp0\..\Utilities\CEcho.exe" cyan "===========================================================================\n"

"%PATH_GNUPG1%\gpg.exe" --detach-sign "%OUT_FILE%.exe"
attrib +R "%OUT_FILE%.exe.sig"

:: ---------------------------------------------------------------------------
:: COMPLETED
:: ---------------------------------------------------------------------------

"%~dp0\..\Utilities\CEcho.exe" green "\n==========================================================================="
"%~dp0\..\Utilities\CEcho.exe" green "Completed successfully :-)"
"%~dp0\..\Utilities\CEcho.exe" green "===========================================================================\n"

pause

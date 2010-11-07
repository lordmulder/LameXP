@echo off
call _paths.bat
REM ------------------------------------------
set "TEMP_DIR=%TEMP%\~LameXP.%DATE%.tmp"
set "OUT_PATH=..\..\bin\Release"
set "OUT_FILE=%OUT_PATH%\..\LameXP.%DATE:~6,4%-%DATE:~3,2%-%DATE:~0,2%.Release.zip"
REM ------------------------------------------
del "%OUT_FILE%"
if exist "%OUT_FILE%" (
	echo.
	echo BUILD HAS FAILED !!!
	echo.
	pause
	exit
)
REM ------------------------------------------
call _build.bat "..\..\LameXP.sln" Release
REM ------------------------------------------
if not exist "%OUT_PATH%\LameXP.exe" (
	echo.
	echo BUILD HAS FAILED !!!
	echo.
	pause
	exit
)
REM ------------------------------------------
rd /S /Q "%TEMP_DIR%"
mkdir "%TEMP_DIR%"
mkdir "%TEMP_DIR%\imageformats"
REM ------------------------------------------
copy "%OUT_PATH%\*.exe" "%TEMP_DIR%"
copy "%QTDIR%\bin\QtCore4.dll" "%TEMP_DIR%"
copy "%QTDIR%\bin\QtGui4.dll" "%TEMP_DIR%"
copy "%QTDIR%\bin\QtXml4.dll" "%TEMP_DIR%"
copy "%QTDIR%\bin\QtSvg4.dll" "%TEMP_DIR%"
copy "%QTDIR%\plugins\imageformats\q???4.dll" "%TEMP_DIR%\imageformats"
REM ------------------------------------------
for %%f in ("%TEMP_DIR%\*.exe") do (
	"%PATH_MPRESS%" -s "%%f"
)
for %%f in ("%TEMP_DIR%\*.dll") do (
	"%PATH_MPRESS%" -s "%%f"
)
REM ------------------------------------------
copy "..\Redist\*.*" "%TEMP_DIR%"
copy "..\..\License.txt" "%TEMP_DIR%"
REM ------------------------------------------
"%PATH_SEVENZ%" a -tzip -r "%OUT_FILE%" "%TEMP_DIR%\*"
rd /S /Q "%TEMP_DIR%"
REM ------------------------------------------
pause

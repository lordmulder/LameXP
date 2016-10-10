@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Print help screen
if "%~4"=="" (
	echo 7-Zip SFX Builder
	echo Usage: 7zSD.cmd ^<input^> ^<output^> ^<title^> ^<fname^>
	exit /b 1
)

REM Generate temp file names
set "SEVENZIP_SFX_CFG=%TMP%\~7zSD%RANDOM%%RANDOM%.cf"
set "SEVENZIP_SFX_ARC=%TMP%\~7zSD%RANDOM%%RANDOM%.7z"

REM Create the configuration file
echo ;^^!@Install@^^!UTF-8^^!> "%SEVENZIP_SFX_CFG%"
echo Title="%~3">>             "%SEVENZIP_SFX_CFG%"
echo ExecuteFile="%~n4.exe">>  "%SEVENZIP_SFX_CFG%"
echo ;^^!@InstallEnd@^^!>>     "%SEVENZIP_SFX_CFG%"

REM Create the 7-Zip archive
"%~dp0\7za.exe" a -t7z "%SEVENZIP_SFX_ARC%" "%~1"
if %ERRORLEVEL% NEQ 0 (
	del "%SEVENZIP_SFX_CFG%"
	del "%SEVENZIP_SFX_ARC%"
	exit /b 1
)

REM Rename the embedded file
"%~dp0\7za.exe" rn "%SEVENZIP_SFX_ARC%" "%~nx1" "%~n4.exe"
if %ERRORLEVEL% NEQ 0 (
	del "%SEVENZIP_SFX_CFG%"
	del "%SEVENZIP_SFX_ARC%"
	exit /b 1
)

REM Actually build the SFX file
copy /b "%~dp0\7zSD.sfx" + "%SEVENZIP_SFX_CFG%" + "%SEVENZIP_SFX_ARC%" "%~2"
if %ERRORLEVEL% NEQ 0 (
	del "%SEVENZIP_SFX_CFG%"
	del "%SEVENZIP_SFX_ARC%"
	exit /b 1
)

REM Final clean-up
del "%SEVENZIP_SFX_CFG%"
del "%SEVENZIP_SFX_ARC%"
exit /b 0

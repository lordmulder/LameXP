@echo off
setlocal ENABLEDELAYEDEXPANSION
REM -----------------------------------------------------------------
set "LAMEXP_ERROR=1"
echo ----------------------------------------------------------------
echo Building software documentation
echo ----------------------------------------------------------------
call "%~dp0\_paths.bat"
if not "%LAMEXP_ERROR%"=="0" GOTO:EOF
REM -----------------------------------------------------------------
set "LAMEXP_ERROR=1"
REM -----------------------------------------------------------------
for %%i in ("%~dp0\..\..\doc\*.md") do (
	echo PANDOC: %%~nxi
	"%PATH_PANDOC%\pandoc.exe" --from markdown_github+pandoc_title_block --to html5 --toc -N --standalone -H "%~dp0\..\Style\style.css" "%%~i" --output "%%~dpni.local.html"
	echo.
	if not "!ERRORLEVEL!"=="0" GOTO:EOF
)
echo ----------------------------------------------------------------
set "LAMEXP_ERROR=0"

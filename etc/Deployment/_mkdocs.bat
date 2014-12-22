@echo off
setlocal EnableDelayedExpansion

:: ------------------------------------------
:: Setup Paths
:: ------------------------------------------

call "%~dp0\_paths.bat"

:: ------------------------------------------
:: Create Documents
:: ------------------------------------------

echo ----------------------------------------------------------------
echo Building software documentation
echo ----------------------------------------------------------------
echo.

for %%i in ("%~dp0\..\..\doc\*.md") do (
	echo PANDOC: %%~nxi
	"%PATH_PANDOC%\pandoc.exe" --from markdown_github+pandoc_title_block --to html5 --toc -N --standalone -H "%~dp0\..\Style\style.css" "%%~i" --output "%%~dpni.html"
	echo.
	if not "!ERRORLEVEL!"=="0" goto:eof
)

echo.

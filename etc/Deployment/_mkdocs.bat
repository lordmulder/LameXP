@echo off
setlocal EnableDelayedExpansion

"%~dp0\..\Utilities\CEcho.exe" cyan "\n==========================================================================="
"%~dp0\..\Utilities\CEcho.exe" cyan "Building software documentation..."
"%~dp0\..\Utilities\CEcho.exe" cyan "===========================================================================\n"

:: ------------------------------------------
:: Setup Paths
:: ------------------------------------------

call "%~dp0\_paths.bat"

:: ------------------------------------------
:: Create Documents
:: ------------------------------------------

for %%i in ("%~dp0\..\..\doc\*.md") do (
	echo PANDOC: %%~nxi
	"%PATH_PANDOC%\pandoc.exe" --from markdown_github+pandoc_title_block+header_attributes --to html5 --toc -N --standalone -H "%~dp0\..\Style\style.css" "%%~i" --output "%%~dpni.html"
	if not "!ERRORLEVEL!"=="0" (
		"%~dp0\..\Utilities\CEcho.exe" red "\nSomething went wrong^^!\n"
		pause && exit
	)
)

echo.

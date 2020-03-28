@echo off
setlocal EnableDelayedExpansion

"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" cyan "\n==========================================================================="
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" cyan "Building software documentation..."
"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" cyan "===========================================================================\n"

:: ------------------------------------------
:: Setup Paths
:: ------------------------------------------

call "%~dp0\_paths.bat"

if not exist "%JAVA_HOME%\bin\java.exe" (
	"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nJava Runtime not found. Please check your JAVA_HOME environment variable^!\n"
	pause && exit
)

:: ------------------------------------------
:: Create Documents
:: ------------------------------------------

for %%i in ("%~dp0\..\..\doc\*.md") do (
	echo PANDOC: %%~nxi
	"%~dp0\..\..\..\Prerequisites\Pandoc\pandoc.exe" --from markdown_github+pandoc_title_block+header_attributes+implicit_figures+yaml_metadata_block --to html5 --toc -N --standalone -H "%~dp0\..\..\..\Prerequisites\Pandoc\css\github-pandoc.inc" "%%~i" | "%JAVA_HOME%\bin\java.exe" -jar "%~dp0\..\..\..\Prerequisites\HTMLCompressor\bin\htmlcompressor-1.5.3.jar" --compress-css -o "%%~dpni.html"
	if not "!ERRORLEVEL!"=="0" (
		"%~dp0\..\..\..\Prerequisites\CEcho\cecho.exe" red "\nSomething went wrong^^!\n"
		pause && exit
	)
)

echo.

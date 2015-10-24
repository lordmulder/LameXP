@echo off
set "GIT_PATH=c:\Program Files\Git"
set "ZIP_PATH=c:\Program Files\7-Zip"
set "PATH=%GIT_PATH%;%GIT_PATH%\mingw64\bin;%GIT_PATH%\usr\bin;%PATH%"
set "OUT_PATH=%TEMP%\~%RANDOM%%RANDOM%.tmp"

mkdir "%OUT_PATH%"
for %%i in (LameXP_Qt,MUtilities,Prerequisites) do (
	mkdir "%OUT_PATH%\%%i"
)

call::git_export "%~dp0..\.."                LameXP_Qt
call::git_export "%~dp0\..\..\..\MUtilities" MUtilities

copy "..\..\*.txt" "%OUT_PATH%"

for %%i in (EncodePointer,VisualLeakDetector) do (
	mkdir "%OUT_PATH%\Prerequisites\%%i"
	xcopy /S /Y "%~dp0\..\..\..\Prerequisites\%%i" "%OUT_PATH%\Prerequisites\%%i"
)

for %%k in (v100,v120_xp,v140_xp) do (
	for %%i in (Static,Shared,Debug) do (
		mkdir "%OUT_PATH%\Prerequisites\Qt4\%%k\%%i"
		echo Please put the Qt library files here! > "%OUT_PATH%\Prerequisites\Qt4\%%k\%%i\README.txt"
	)
)

pushd "%OUT_PATH%"
tar -cvf ./sources.tar *
"%ZIP_PATH%\7z.exe" a -txz "%~dp0\..\..\out\~sources.tar.xz" "sources.tar"
popd

pushd "%~dp0"
rmdir /S /Q "%OUT_PATH%"

pause
exit


:git_export
pushd "%~1"
git archive --verbose --output "%OUT_PATH%\%~2.tar" MASTER
popd
pushd "%OUT_PATH%\%~2"
tar -xvf "../%~2.tar"
del "%OUT_PATH%\%~2.tar"
popd
goto:eof

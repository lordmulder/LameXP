@echo off
cd tools
set "QTDIR=C:\Qt\4.8.7"
set "PATH=%QTDIR%\bin;%PATH%"
set "TMPFILENAME=%TMP%\~%RANDOM%%RANDOM%.txt"
for %%f in (*.*) do (
	echo %%f
	"%~dp0\..\..\MakeHash\Release\MakeHash.exe" "%%f" 2> NUL >> "%TMPFILENAME%"
)
start /WAIT notepad.exe "%TMPFILENAME%"
del "%TMPFILENAME%"

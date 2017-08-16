@echo off
cd /d "%~dp0"

"%~dp0\..\..\Prerequisites\QRCMerger\QRCMerger.exe" "%~dp0\." "%~dp0\_ALL.qrc"
pause

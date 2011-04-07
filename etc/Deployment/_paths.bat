@echo off
set "LAMEXP_ERROR=1"
REM ------------------------------------------
set "PATH_SEVENZ="
set "PATH_MPRESS="
set "PATH_UPXBIN="
set "PATH_MKNSIS="
set "PATH_MSVC90="
set "PATH_QTMSVC="
set "PATH_GNUPG1="
set "PATH_VCPROJ="
REM ------------------------------------------
for /f "tokens=2,*" %%s in (buildenv.txt) do (
	if "%%s"=="PATH_SEVENZ" set "PATH_SEVENZ=%%~t"
	if "%%s"=="PATH_MPRESS" set "PATH_MPRESS=%%~t"
	if "%%s"=="PATH_UPXBIN" set "PATH_UPXBIN=%%~t"
	if "%%s"=="PATH_MKNSIS" set "PATH_MKNSIS=%%~t"
	if "%%s"=="PATH_MSVC90" set "PATH_MSVC90=%%~t"
	if "%%s"=="PATH_QTMSVC" set "PATH_QTMSVC=%%~t"
	if "%%s"=="PATH_GNUPG1" set "PATH_GNUPG1=%%~t"
	if "%%s"=="PATH_VCPROJ" set "PATH_VCPROJ=%%~t"
)
REM ------------------------------------------
echo PATH_SEVENZ ="%PATH_SEVENZ%"
echo PATH_MPRESS ="%PATH_MPRESS%"
echo PATH_UPXBIN ="%PATH_UPXBIN%"
echo PATH_MKNSIS ="%PATH_MKNSIS%"
echo PATH_MSVC90 ="%PATH_MSVC90%"
echo PATH_QTMSVC ="%PATH_QTMSVC%"
echo PATH_GNUPG1 ="%PATH_GNUPG1%"
echo PATH_VCPROJ ="%PATH_VCPROJ%"
REM ------------------------------------------
set "LAMEXP_ERROR=1"
REM ------------------------------------------
if not exist "%PATH_SEVENZ%\7z.exe" GOTO:EOF
if not exist "%PATH_MPRESS%\mpress.exe" GOTO:EOF
if not exist "%PATH_UPXBIN%\upx.exe" GOTO:EOF
if not exist "%PATH_MKNSIS%\makensis.exe" GOTO:EOF
if not exist "%PATH_MSVC90%\VC\vcvarsall.bat" GOTO:EOF
if not exist "%PATH_MSVC90%\VC\bin\cl.exe" GOTO:EOF
if not exist "%PATH_QTMSVC%\bin\qtvars.bat" GOTO:EOF
if not exist "%PATH_QTMSVC%\bin\uic.exe" GOTO:EOF
if not exist "%PATH_QTMSVC%\bin\moc.exe" GOTO:EOF
if not exist "%PATH_QTMSVC%\bin\rcc.exe" GOTO:EOF
if not exist "%PATH_GNUPG1%\gpg.exe" GOTO:EOF
if not exist "%PATH_GNUPG1%\gpg.exe" GOTO:EOF
if not exist "%PATH_VCPROJ%" GOTO:EOF
REM ------------------------------------------
set "LAMEXP_ERROR=0"

; ///////////////////////////////////////////////////////////////////////////////
; // LameXP - Audio Encoder Front-End
; // Copyright (C) 2004-2011 LoRd_MuldeR <MuldeR2@GMX.de>
; //
; // This program is free software; you can redistribute it and/or modify
; // it under the terms of the GNU General Public License as published by
; // the Free Software Foundation; either version 2 of the License, or
; // (at your option) any later version.
; //
; // This program is distributed in the hope that it will be useful,
; // but WITHOUT ANY WARRANTY; without even the implied warranty of
; // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; // GNU General Public License for more details.
; //
; // You should have received a copy of the GNU General Public License along
; // with this program; if not, write to the Free Software Foundation, Inc.,
; // 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
; //
; // http://www.gnu.org/licenses/gpl-2.0.txt
; ///////////////////////////////////////////////////////////////////////////////

;--------------------------------
;Basic Defines
;--------------------------------

!ifndef LAMEXP_VERSION
  !error "LAMEXP_VERSION is not defined !!!"
!endif
!ifndef LAMEXP_BUILD
  !error "LAMEXP_BUILD is not defined !!!"
!endif
!ifndef LAMEXP_INSTTYPE
  !error "LAMEXP_INSTTYPE is not defined !!!"
!endif
!ifndef LAMEXP_PATCH
  !error "LAMEXP_PATCH is not defined !!!"
!endif
!ifndef LAMEXP_DATE
  !error "LAMEXP_DATE is not defined !!!"
!endif
!ifndef LAMEXP_OUTPUT_FILE
  !error "LAMEXP_OUTPUT_FILE is not defined !!!"
!endif
!ifndef LAMEXP_SOURCE_FILE
  !error "LAMEXP_SOURCE_FILE is not defined !!!"
!endif
!ifndef LAMEXP_UPX_PATH
  !error "LAMEXP_UPX_PATH is not defined !!!"
!endif

;UUID
!define MyRegPath "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{FBD7A67D-D700-4043-B54F-DD106D00F308}"

;Web-Site
!define MyWebSite "http://mulder.at.gg/"


;--------------------------------
;Includes
;--------------------------------

!include `parameters.nsh`


;--------------------------------
;Installer Attributes
;--------------------------------

RequestExecutionLevel user
XPStyle on
Name "LameXP v${LAMEXP_VERSION} ${LAMEXP_INSTTYPE}-${LAMEXP_PATCH} [Build #${LAMEXP_BUILD}]"
OutFile "${LAMEXP_OUTPUT_FILE}"
BrandingText "${LAMEXP_DATE} / Build ${LAMEXP_BUILD}"
InstallDir "$PROGRAMFILES\MuldeR\LameXP v${LAMEXP_VERSION}"
Icon "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
ChangeUI all "${NSISDIR}\Contrib\UIs\sdbarker_tiny.exe"
ShowInstDetails show
AutoCloseWindow true


;--------------------------------
;Page Captions
;--------------------------------

SubCaption 0 " "
SubCaption 1 " "
SubCaption 2 " "
SubCaption 3 " "
SubCaption 4 " "


;--------------------------------
;Compressor
;--------------------------------

!packhdr "$%TEMP%\exehead.tmp" '"${LAMEXP_UPX_PATH}\upx.exe" --brute "$%TEMP%\exehead.tmp"'


;--------------------------------
;Version Info
;--------------------------------

!searchreplace PRODUCT_VERSION_DATE "${LAMEXP_DATE}" "-" "."
VIProductVersion "${PRODUCT_VERSION_DATE}.${LAMEXP_BUILD}"

VIAddVersionKey "Author" "LoRd_MuldeR <mulder2@gmx.de>"
VIAddVersionKey "Comments" "This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version."
VIAddVersionKey "CompanyName" "Free Software Foundation"
VIAddVersionKey "FileDescription" "LameXP v${LAMEXP_VERSION} ${LAMEXP_INSTTYPE}-${LAMEXP_PATCH} [Build #${LAMEXP_BUILD}]"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION_DATE}.${LAMEXP_BUILD} (${LAMEXP_VERSION})"
VIAddVersionKey "LegalCopyright" "Copyright 2004-2011 LoRd_MuldeR"
VIAddVersionKey "LegalTrademarks" "GNU"
VIAddVersionKey "OriginalFilename" "LameXP.${LAMEXP_DATE}.exe"
VIAddVersionKey "ProductName" "LameXP - Audio Encoder Frontend"
VIAddVersionKey "ProductVersion" "${LAMEXP_VERSION}, Build #${LAMEXP_BUILD} (${LAMEXP_DATE})"
VIAddVersionKey "Website" "${MyWebSite}"


;--------------------------------
;Installer initialization
;--------------------------------

Section "-LaunchTheInstaller"
	SetDetailsPrint textonly
	DetailPrint "Launching installer, please stay tuned..."
	SetDetailsPrint listonly
	
	InitPluginsDir
	SetOutPath "$PLUGINSDIR"
	File "/oname=$PLUGINSDIR\Install.exe" "${LAMEXP_SOURCE_FILE}"
	!insertmacro GetCommandlineParameter "Update" "?" $R9
	
	RunTryAgain:
	
	ClearErrors
	StrCmp "$R9" "?" +3 0
	ExecShell "open" "$PLUGINSDIR\Install.exe" '/Update=$R9' SW_SHOWNORMAL
	Goto +2
	ExecShell "open" "$PLUGINSDIR\Install.exe" '/Update' SW_SHOWNORMAL
	IfErrors 0 RunSuccess
	
	ClearErrors
	StrCmp "$R9" "?" +3 0
	ExecShell "" "$PLUGINSDIR\Install.exe" '/Update=$R9' SW_SHOWNORMAL
	Goto +2
	ExecShell "" "$PLUGINSDIR\Install.exe" '/Update' SW_SHOWNORMAL
	IfErrors 0 RunSuccess

	MessageBox MB_RETRYCANCEL|MB_ICONSTOP|MB_TOPMOST "Failed to launch installer. Please try again!" IDRETRY RunTryAgain
	Abort "Failed to launch installer :-("
	
	RunSuccess:
	Delete /REBOOTOK "$PLUGINSDIR\Install.exe"
SectionEnd

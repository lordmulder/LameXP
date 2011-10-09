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

;Web-Site
!define MyWebSite "http://mulder.at.gg/"


;--------------------------------
;Includes
;--------------------------------

!include `StdUtils.nsh`


;--------------------------------
;Installer Attributes
;--------------------------------

XPStyle on
RequestExecutionLevel user
InstallColors /windows
Name "LameXP v${LAMEXP_VERSION} ${LAMEXP_INSTTYPE}-${LAMEXP_PATCH} [Build #${LAMEXP_BUILD}]"
OutFile "${LAMEXP_OUTPUT_FILE}"
BrandingText "${LAMEXP_DATE} / Build #${LAMEXP_BUILD}"
Icon "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
ChangeUI all "${NSISDIR}\Contrib\UIs\sdbarker_tiny.exe"
ShowInstDetails show
AutoCloseWindow true
InstallDir ""


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
;Reserved Files
;--------------------------------

ReserveFile "${NSISDIR}\Plugins\System.dll"
ReserveFile "${NSISDIR}\Plugins\StdUtils.dll"


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
	File "/oname=$PLUGINSDIR\LameXP-Install.exe" "${LAMEXP_SOURCE_FILE}"
	
	; --------
	
	StrCpy $R9 ""
	${StdUtils.GetParameter} $R0 "Update" "?"
	
	StrCmp "$R0" "?" +5
	StrCmp "$R0" "" 0 +3
	StrCpy $R9 "/Update"
	Goto +2
	StrCpy $R9 '"/Update=$R0"'
	
	StrCmp "$INSTDIR" "" +5
	StrCmp "$R9" "" 0 +3
	StrCpy $R9 '/D=$INSTDIR'
	Goto +2
	StrCpy $R9 '$R9 /D=$INSTDIR'
	
	; --------

	RunTryAgain:
	
	DetailPrint "ExecShellWait: $PLUGINSDIR\LameXP-Install.exe"
	${StdUtils.ExecShellWait} $R1 "$PLUGINSDIR\LameXP-Install.exe" "open" '$R9'
	DetailPrint "Result: $R1"
	
	StrCmp $R1 "error" RunFailed
	StrCmp $R1 "no_wait" RunSuccess
	Sleep 333
	HideWindow
	${StdUtils.WaitForProc} $R1
	Goto RunSuccess
	
	; --------

	RunFailed:

	MessageBox MB_RETRYCANCEL|MB_ICONSTOP|MB_TOPMOST "Failed to launch the installer. Please try again!" IDRETRY RunTryAgain

	; --------

	ClearErrors
	ExecShell "open" "$PLUGINSDIR\LameXP-Install.exe" '$R9' SW_SHOWNORMAL
	IfErrors 0 RunSuccess

	ClearErrors
	ExecShell "" "$PLUGINSDIR\LameXP-Install.exe" '$R9' SW_SHOWNORMAL
	IfErrors 0 RunSuccess

	; --------

	SetDetailsPrint both
	DetailPrint "Failed to launch installer :-("
	SetDetailsPrint listonly

	Abort "Aborted."

	; --------
	
	RunSuccess:

	Delete /REBOOTOK "$PLUGINSDIR\LameXP-Install.exe"
SectionEnd

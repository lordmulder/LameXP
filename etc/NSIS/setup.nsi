; ///////////////////////////////////////////////////////////////////////////////
; // LameXP - Audio Encoder Front-End
; // Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
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
!ifndef LAMEXP_SUFFIX
  !error "LAMEXP_SUFFIX is not defined !!!"
!endif
!ifndef LAMEXP_DATE
  !error "LAMEXP_DATE is not defined !!!"
!endif
!ifndef LAMEXP_OUTPUT_FILE
  !error "LAMEXP_OUTPUT_FILE is not defined !!!"
!endif
!ifndef LAMEXP_SOURCE_PATH
  !error "LAMEXP_SOURCE_PATH is not defined !!!"
!endif
!ifndef LAMEXP_UPX_PATH
  !error "LAMEXP_UPX_PATH is not defined !!!"
!endif

;UUID
!define MyRegPath "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{FBD7A67D-D700-4043-B54F-DD106D00F308}"


;--------------------------------
;Includes
;--------------------------------

!include `MUI2.nsh`
!include `UAC.nsh`
!include `parameters.nsh`


;--------------------------------
;Installer Attributes
;--------------------------------

RequestExecutionLevel user
ShowInstDetails show
ShowUninstDetails show
Name "LameXP v${LAMEXP_VERSION} ${LAMEXP_SUFFIX} [Build #${LAMEXP_BUILD}]"
OutFile "${LAMEXP_OUTPUT_FILE}"
BrandingText "Date created: ${LAMEXP_DATE}"
InstallDir "$PROGRAMFILES\MuldeR\LameXP v${LAMEXP_VERSION}"
InstallDirRegKey HKLM "${MyRegPath}" "InstallLocation"


;--------------------------------
;Compressor
;--------------------------------

SetCompressor /SOLID LZMA
SetCompressorDictSize 64

!packhdr "$%TEMP%\exehead.tmp" '"${LAMEXP_UPX_PATH}\upx.exe" --brute "$%TEMP%\exehead.tmp"'


;--------------------------------
;Variables
;--------------------------------

Var StartMenuFolder


;--------------------------------
;Version Info
;--------------------------------

!searchreplace PRODUCT_VERSION_DATE "${LAMEXP_DATE}" "-" "."
VIProductVersion "${PRODUCT_VERSION_DATE}.${LAMEXP_BUILD}"

VIAddVersionKey "Author" "LoRd_MuldeR <mulder2@gmx.de>"
VIAddVersionKey "Comments" "This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version."
VIAddVersionKey "CompanyName" "Free Software Foundation"
VIAddVersionKey "FileDescription" "LameXP v${LAMEXP_VERSION} ${LAMEXP_SUFFIX} [Build #${LAMEXP_BUILD}]"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION_DATE}.${LAMEXP_BUILD} (${LAMEXP_VERSION})"
VIAddVersionKey "LegalCopyright" "Copyright 2004-2010 LoRd_MuldeR"
VIAddVersionKey "LegalTrademarks" "GNU"
VIAddVersionKey "OriginalFilename" "LameXP.${LAMEXP_DATE}.exe"
VIAddVersionKey "ProductName" "LameXP - Audio Encoder Frontend"
VIAddVersionKey "ProductVersion" "${LAMEXP_VERSION}, Build #${LAMEXP_BUILD} (${LAMEXP_DATE})"
VIAddVersionKey "Website" "http://mulder.at.gg/"


;--------------------------------
;MUI2 Interface Settings
;--------------------------------

!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE_3LINES
!define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${MyRegPath}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartmenuFolder"
!define MUI_LANGDLL_REGISTRY_ROOT HKLM
!define MUI_LANGDLL_REGISTRY_KEY "${MyRegPath}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "SetupLanguage"
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "LameXP v${LAMEXP_VERSION}"
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION RunAppFunction
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION ShowReadmeFunction
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange-uninstall.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\orange.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "${NSISDIR}\Contrib\Graphics\Header\orange-uninstall.bmp"
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_CUSTOMFUNCTION_GUIINIT MyUacInit
!define MUI_CUSTOMFUNCTION_UNGUIINIT un.MyUacInit


;--------------------------------
;MUI2 Pages
;--------------------------------

;Installer
!insertmacro MUI_PAGE_WELCOME
!define MUI_PAGE_CUSTOMFUNCTION_SHOW CheckForUpdate
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

;Uninstaller
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


;--------------------------------
;Languages
;--------------------------------
 
!insertmacro MUI_LANGUAGE "English" ;first language is the default language
!insertmacro MUI_LANGUAGE "German"

; !insertmacro MUI_LANGUAGE "French"
; !insertmacro MUI_LANGUAGE "Spanish"
; !insertmacro MUI_LANGUAGE "SpanishInternational"
; !insertmacro MUI_LANGUAGE "SimpChinese"
; !insertmacro MUI_LANGUAGE "TradChinese"
; !insertmacro MUI_LANGUAGE "Japanese"
; !insertmacro MUI_LANGUAGE "Italian"
; !insertmacro MUI_LANGUAGE "Dutch"
; !insertmacro MUI_LANGUAGE "Greek"
; !insertmacro MUI_LANGUAGE "Russian"
; !insertmacro MUI_LANGUAGE "Polish"
; !insertmacro MUI_LANGUAGE "Ukrainian"
; !insertmacro MUI_LANGUAGE "Hungarian"
; !insertmacro MUI_LANGUAGE "Romanian"
; !insertmacro MUI_LANGUAGE "Serbian"
; !insertmacro MUI_LANGUAGE "SerbianLatin"
; !insertmacro MUI_LANGUAGE "Arabic"
; !insertmacro MUI_LANGUAGE "Portuguese"
; !insertmacro MUI_LANGUAGE "Afrikaans"
; !insertmacro MUI_LANGUAGE "Malay"
; !insertmacro MUI_LANGUAGE "Indonesian"


;--------------------------------
;Translation
;--------------------------------

;English
LangString LAMEXP_LANG_STATUS_CLOSING    ${LANG_ENGLISH} "Closing running instance, please wait..."
LangString LAMEXP_LANG_STATUS_INSTFILES  ${LANG_ENGLISH} "Installing program files, please wait..."
LangString LAMEXP_LANG_STATUS_MAKEUNINST ${LANG_ENGLISH} "Creating the uninstaller, please wait..."
LangString LAMEXP_LANG_STATUS_SHORTCUTS  ${LANG_ENGLISH} "Creating shortcuts, please wait..."
LangString LAMEXP_LANG_STATUS_REGISTRY   ${LANG_ENGLISH} "Updating the registry, please wait..."
LangString LAMEXP_LANG_STATUS_UNINSTALL  ${LANG_ENGLISH} "Uninstalling program, please wait..."

;German
LangString LAMEXP_LANG_STATUS_CLOSING    ${LANG_GERMAN} "Schließe laufende Instanz, bitte warten..."
LangString LAMEXP_LANG_STATUS_INSTFILES  ${LANG_GERMAN} "Installiere Programm-Dateien, bitte warten..."
LangString LAMEXP_LANG_STATUS_MAKEUNINST ${LANG_GERMAN} "Erzeuge Uninstaller, bitte warten..."
LangString LAMEXP_LANG_STATUS_SHORTCUTS  ${LANG_GERMAN} "Erzeuge Verknüpfungen, bitte warten..."
LangString LAMEXP_LANG_STATUS_REGISTRY   ${LANG_GERMAN} "Registrierung wird aktualisiert, bitte warten..."
LangString LAMEXP_LANG_STATUS_UNINSTALL  ${LANG_GERMAN} "Programm wird deinstalliert, bitte warten..."


;--------------------------------
;Installer initialization
;--------------------------------

Function .onInit
	${If} ${UAC_IsInnerInstance}
		!insertmacro MUI_LANGDLL_DISPLAY
	${EndIf}  
FunctionEnd

Function un.onInit
	${If} ${UAC_IsInnerInstance}
		!insertmacro MUI_LANGDLL_DISPLAY
	${EndIf}  
FunctionEnd


;--------------------------------
;UAC initialization
;--------------------------------

Function MyUacInit
	UAC_TryAgain:
	!insertmacro UAC_RunElevated
	${Switch} $0
	${Case} 0
		${IfThen} $1 = 1 ${|} Quit ${|}
		${IfThen} $3 <> 0 ${|} ${Break} ${|}
		${If} $1 = 3
			MessageBox MB_ICONEXCLAMATION|MB_TOPMOST|MB_SETFOREGROUND|MB_OKCANCEL "This installer requires admin access, please try again!" /SD IDCANCEL IDOK UAC_TryAgain
		${EndIf}
	${Case} 1223
		MessageBox MB_ICONEXCLAMATION|MB_TOPMOST|MB_SETFOREGROUND|MB_OKCANCEL "This installer requires admin privileges, please try again!" /SD IDCANCEL IDOK UAC_TryAgain
		Quit
	${Case} 1062
		MessageBox MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Logon service not running, aborting!"
		Quit
	${Default}
		MessageBox MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Unable to elevate installer! (Error code: $0)"
		Quit
	${EndSwitch}
FunctionEnd

Function un.MyUacInit
	UAC_TryAgain:
	!insertmacro UAC_RunElevated
	${Switch} $0
	${Case} 0
		${IfThen} $1 = 1 ${|} Quit ${|}
		${IfThen} $3 <> 0 ${|} ${Break} ${|}
		${If} $1 = 3
			MessageBox MB_ICONEXCLAMATION|MB_TOPMOST|MB_SETFOREGROUND|MB_OKCANCEL "This un-installer requires admin access, please try again!" /SD IDCANCEL IDOK UAC_TryAgain
		${EndIf}
	${Case} 1223
		MessageBox MB_ICONEXCLAMATION|MB_TOPMOST|MB_SETFOREGROUND|MB_OKCANCEL "This un-installer requires admin privileges, please try again!" /SD IDCANCEL IDOK UAC_TryAgain
		Quit
	${Case} 1062
		MessageBox MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Logon service not running, aborting!"
		Quit
	${Default}
		MessageBox MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Unable to elevate installer! (Error code: $0)"
		Quit
	${EndSwitch}
FunctionEnd


;--------------------------------
;Macros
;--------------------------------

!macro PrintProgress Text
  SetDetailsPrint textonly
  DetailPrint '${Text}'
  SetDetailsPrint listonly
  Sleep 1000
!macroend

!macro CreateWebLink ShortcutFile TargetURL
  Push $0
  Push $1
  StrCpy $0 "${ShortcutFile}"
  StrCpy $1 "${TargetURL}"
  Call _CreateWebLink
  Pop $1
  Pop $0
!macroend

Function _CreateWebLink
  FlushINI "$0"
  SetFileAttributes "$0" FILE_ATTRIBUTE_NORMAL
  DeleteINISec "$0" "DEFAULT"
  DeleteINISec "$0" "InternetShortcut"
  WriteINIStr "$0" "DEFAULT" "BASEURL" "$1"
  WriteINIStr "$0" "InternetShortcut" "ORIGURL" "$1"
  WriteINIStr "$0" "InternetShortcut" "URL" "$1"
  WriteINIStr "$0" "InternetShortcut" "IconFile" "$SYSDIR\SHELL32.dll"
  WriteINIStr "$0" "InternetShortcut" "IconIndex" "150"
  FlushINI "$0"
  SetFileAttributes "$0" FILE_ATTRIBUTE_READONLY
FunctionEnd


;--------------------------------
;Install Files
;--------------------------------

Section "-Prepare"
	SetOutPath "$INSTDIR"
SectionEnd

Section "-Terminate"
	IfFileExists "$INSTDIR\LameXP.exe" 0 NotInstalled
		!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_CLOSING)"
		!insertmacro UAC_AsUser_Call Function CloseRunningInstance UAC_SYNCOUTDIR
	NotInstalled:
SectionEnd

Section "!Install Files"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_INSTFILES)"
	File /r `${LAMEXP_SOURCE_PATH}\*.*`
SectionEnd

Section "-Write Uinstaller"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_MAKEUNINST)"
	WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "-Create Shortcuts"
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
		!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_SHORTCUTS)"
		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		
		Delete "$SMPROGRAMS\$StartMenuFolder\*.lnk"
		Delete "$SMPROGRAMS\$StartMenuFolder\*.pif"
		Delete "$SMPROGRAMS\$StartMenuFolder\*.url"
		
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\LameXP.lnk" "$INSTDIR\LameXP.exe" "" "$INSTDIR\LameXP.exe" 0
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\License.lnk" "$INSTDIR\License.txt"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
		
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\Official LameXP Homepage.url" "http://mulder.dummwiedeutsch.de/"
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\RareWares.org.url" "http://rarewares.org/"
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\Hydrogenaudio Forums.url" "http://www.hydrogenaudio.org/"
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "-Update Registry"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_REGISTRY)"
	WriteRegStr HKLM "${MyRegPath}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "${MyRegPath}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	WriteRegStr HKLM "${MyRegPath}" "DisplayName" "LameXP"
SectionEnd

Section
	!insertmacro PrintProgress "$(MUI_TEXT_FINISH_TITLE)."
SectionEnd


;--------------------------------
;Uninstaller
;--------------------------------

Section "Uninstall"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_UNINSTALL)"

	IfFileExists "$INSTDIR\LameXP.exe" 0 UnNotInstalled
		!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_CLOSING)"
		!insertmacro UAC_AsUser_Call Function un.CloseRunningInstance UAC_SYNCOUTDIR
	UnNotInstalled:

	Delete /REBOOTOK "$INSTDIR\*.exe"
	Delete /REBOOTOK "$INSTDIR\*.txt"
	RMDir "$INSTDIR"

	!insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
	StrCmp "$StartMenuFolder" "" NoStartmenuFolder
	IfFileExists "$SMPROGRAMS\$StartMenuFolder\*.*" 0 NoStartmenuFolder
	Delete /REBOOTOK "$SMPROGRAMS\$StartMenuFolder\*.lnk"
	Delete /REBOOTOK "$SMPROGRAMS\$StartMenuFolder\*.url"
	RMDir "$SMPROGRAMS\$StartMenuFolder"

	NoStartmenuFolder:

	DeleteRegValue HKLM "${MyRegPath}" "InstallLocation"
	DeleteRegValue HKLM "${MyRegPath}" "UninstallString"
	DeleteRegValue HKLM "${MyRegPath}" "DisplayName"
	DeleteRegValue HKLM "${MyRegPath}" "StartmenuFolder"
	DeleteRegValue HKLM "${MyRegPath}" "SetupLanguage"
	
	!insertmacro PrintProgress "$(MUI_UNTEXT_FINISH_TITLE)."
SectionEnd


;--------------------------------
;Check For Update Mode
;--------------------------------

Function CheckForUpdate
	!insertmacro GetCommandlineParameter "Update" "error" $R0
	StrCmp $R0 "error" 0 EnableUpdateMode
	IfFileExists "$INSTDIR\LameXP.exe" EnableUpdateMode
	Return

	EnableUpdateMode:

	FindWindow $R0 "#32770" "" $HWNDPARENT
	GetDlgItem $R1 $R0 1019
	EnableWindow $R1 0

	FindWindow $R0 "#32770" "" $HWNDPARENT
	GetDlgItem $R1 $R0 1001
	EnableWindow $R1 0
FunctionEnd


;--------------------------------
;Install Success
;--------------------------------

Function RunAppFunction
	!insertmacro UAC_AsUser_ExecShell "explore" "$INSTDIR" "" "" SW_SHOWNORMAL
	!insertmacro UAC_AsUser_ExecShell "open" "$INSTDIR\LameXP.exe" "" "$INSTDIR" SW_SHOWNORMAL
FunctionEnd

Function ShowReadmeFunction
	!insertmacro UAC_AsUser_ExecShell "open" "$INSTDIR\License.txt" "" "" SW_SHOWNORMAL
FunctionEnd


;--------------------------------
;Close Running Instance
;--------------------------------

Function CloseRunningInstance
	nsExec::Exec /TIMEOUT=30000 '"$OUTDIR\LameXP.exe" --force-kill'
FunctionEnd

Function un.CloseRunningInstance
	nsExec::Exec /TIMEOUT=30000 '"$OUTDIR\LameXP.exe" --force-kill'
FunctionEnd


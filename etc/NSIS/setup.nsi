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
!ifndef LAMEXP_SOURCE_PATH
  !error "LAMEXP_SOURCE_PATH is not defined !!!"
!endif
!ifndef LAMEXP_UPX_PATH
  !error "LAMEXP_UPX_PATH is not defined !!!"
!endif

;UUID
!define MyRegPath "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{FBD7A67D-D700-4043-B54F-DD106D00F308}"

;Web-Site
!define MyWebSite "http://mulder.at.gg/"


;--------------------------------
;Check for Pre-Release
;--------------------------------

!define LAMEXP_IS_PRERELEASE

!if '${LAMEXP_INSTTYPE}' == 'Final'
  !undef LAMEXP_IS_PRERELEASE
!endif
!if '${LAMEXP_INSTTYPE}' == 'Hotfix'
  !undef LAMEXP_IS_PRERELEASE
!endif


;--------------------------------
;Includes
;--------------------------------

!include `MUI2.nsh`
!include `WinVer.nsh`
!include `UAC.nsh`
!include `parameters.nsh`


;--------------------------------
;Installer Attributes
;--------------------------------

RequestExecutionLevel user
ShowInstDetails show
ShowUninstDetails show
Name "LameXP v${LAMEXP_VERSION} ${LAMEXP_INSTTYPE}-${LAMEXP_PATCH} [Build #${LAMEXP_BUILD}]"
OutFile "${LAMEXP_OUTPUT_FILE}"
BrandingText "Date created: ${LAMEXP_DATE} [Build #${LAMEXP_BUILD}]"
InstallDir "$PROGRAMFILES\MuldeR\LameXP v${LAMEXP_VERSION}"
InstallDirRegKey HKLM "${MyRegPath}" "InstallLocation"


;--------------------------------
;Compressor
;--------------------------------

SetCompressor /SOLID LZMA
SetCompressorDictSize 64

!packhdr "$%TEMP%\exehead.tmp" '"${LAMEXP_UPX_PATH}\upx.exe" --brute "$%TEMP%\exehead.tmp"'


;--------------------------------
;Reserved Files
;--------------------------------

ReserveFile "${NSISDIR}\Plugins\UAC.dll"
ReserveFile "${NSISDIR}\Plugins\System.dll"
ReserveFile "${NSISDIR}\Plugins\nsDialogs.dll"
ReserveFile "${NSISDIR}\Plugins\StartMenu.dll"
ReserveFile "${NSISDIR}\Plugins\LockedList.dll"
ReserveFile "${NSISDIR}\Plugins\Aero.dll"


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
VIAddVersionKey "FileDescription" "LameXP v${LAMEXP_VERSION} ${LAMEXP_INSTTYPE}-${LAMEXP_PATCH} [Build #${LAMEXP_BUILD}]"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION_DATE}.${LAMEXP_BUILD} (${LAMEXP_VERSION})"
VIAddVersionKey "LegalCopyright" "Copyright 2004-2011 LoRd_MuldeR"
VIAddVersionKey "LegalTrademarks" "GNU"
VIAddVersionKey "OriginalFilename" "LameXP.${LAMEXP_DATE}.exe"
VIAddVersionKey "ProductName" "LameXP - Audio Encoder Frontend"
VIAddVersionKey "ProductVersion" "${LAMEXP_VERSION}, Build #${LAMEXP_BUILD} (${LAMEXP_DATE})"
VIAddVersionKey "Website" "${MyWebSite}"


;--------------------------------
;MUI2 Interface Settings
;--------------------------------

!define MUI_ABORTWARNING
!define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${MyRegPath}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartmenuFolder"
!define MUI_LANGDLL_REGISTRY_ROOT HKLM
!define MUI_LANGDLL_REGISTRY_KEY "${MyRegPath}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "SetupLanguage"
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "LameXP v${LAMEXP_VERSION}"
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION RunAppFunction
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION ShowReadmeFunction
!define MUI_FINISHPAGE_LINK ${MyWebSite}
!define MUI_FINISHPAGE_LINK_LOCATION ${MyWebSite}
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "wizard.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "wizard-un.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "header-un.bmp"
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_CUSTOMFUNCTION_GUIINIT MyUacInit
!define MUI_CUSTOMFUNCTION_UNGUIINIT un.MyUacInit
!define MUI_LANGDLL_ALWAYSSHOW


;--------------------------------
;MUI2 Pages
;--------------------------------

;Installer
!define MUI_WELCOMEPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_TITLE_3LINES
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.rtf"
!define MUI_PAGE_CUSTOMFUNCTION_SHOW CheckForUpdate
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
Page Custom LockedListShow
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

;Uninstaller
!define MUI_WELCOMEPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_TITLE_3LINES
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
UninstPage Custom un.LockedListShow
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


;--------------------------------
;Languages
;--------------------------------
 
!insertmacro MUI_LANGUAGE "English" ;first language is the default language
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Ukrainian"

; !insertmacro MUI_LANGUAGE "French"
; !insertmacro MUI_LANGUAGE "SpanishInternational"
; !insertmacro MUI_LANGUAGE "SimpChinese"
; !insertmacro MUI_LANGUAGE "TradChinese"
; !insertmacro MUI_LANGUAGE "Japanese"
; !insertmacro MUI_LANGUAGE "Italian"
; !insertmacro MUI_LANGUAGE "Dutch"
; !insertmacro MUI_LANGUAGE "Greek"
; !insertmacro MUI_LANGUAGE "Polish"
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
!include "..\Translation\Blank.nsh"

;German
!include "..\Translation\LameXP_DE.nsh"

;Spanish
!include "..\Translation\LameXP_ES.nsh"

;Russian
!include "..\Translation\LameXP_RU.nsh"

;Ukrainian
!include "..\Translation\LameXP_UK.nsh"


;--------------------------------
;Installer initialization
;--------------------------------

Function .onInit
	${If} ${UAC_IsInnerInstance}
		!insertmacro MUI_LANGDLL_DISPLAY
	${ElseIf} ${UAC_IsAdmin}
		!insertmacro MUI_LANGDLL_DISPLAY
	${Else}
		System::Call 'kernel32::CreateMutexA(i 0, i 0, t "{2B3D1EBF-B3B6-4E93-92B9-6853029A7162}") i .r1 ?e'
		Pop $0
		StrCmp $0 0 +3
		MessageBox MB_ICONSTOP|MB_TOPMOST "Sorry, the installer is already running!"
		Quit
	${EndIf}

	${IfNot} ${IsNT}
		MessageBox MB_TOPMOST|MB_ICONSTOP "Sorry, this application does NOT support Windows 9x or Windows ME!"
		Quit
	${EndIf}

	${If} ${AtMostWinNT4}
		!insertmacro GetCommandlineParameter "Update" "?" $R0
		${If} $R0 == "?"
			MessageBox MB_TOPMOST|MB_ICONSTOP "Sorry, your platform is not supported anymore. Installation aborted!$\nThe minimum required platform is Windows 2000."
		${Else}
			MessageBox MB_TOPMOST|MB_ICONSTOP "Sorry, your platform is not supported anymore. Update not possible!$\nThe minimum required platform is Windows 2000."
		${EndIf}
		Quit
	${EndIf}

	${If} ${IsWin2000}
	${AndIf} ${AtMostServicePack} 3
		MessageBox MB_TOPMOST|MB_ICONEXCLAMATION "This application recommends Windows 2000 with Service Pack 4!"
		MessageBox MB_TOPMOST|MB_ICONQUESTION|MB_YESNO "Do you want to download Service Pack 4 for Windows 2000 now?" IDNO +2
		ExecShell "open" "http://www.microsoft.com/download/en/details.aspx?id=4127"
	${EndIf}

	${If} ${IsWinXP}
	${AndIf} ${AtMostServicePack} 2
		MessageBox MB_TOPMOST|MB_ICONEXCLAMATION "This application recommends Windows XP with Service Pack 3!"
		MessageBox MB_TOPMOST|MB_ICONQUESTION|MB_YESNO "Do you want to download Service Pack 3 for Windows XP now?" IDNO +2
		ExecShell "open" "http://www.microsoft.com/download/en/details.aspx?id=24"
	${EndIf}
	
	InitPluginsDir
	File "/oname=$PLUGINSDIR\checkproc.exe" "checkproc.exe"
	nsExec::Exec /TIMEOUT=5000 '"$PLUGINSDIR\checkproc.exe" Softonic Brothersoft'
FunctionEnd

Function un.onInit
	${If} ${UAC_IsInnerInstance}
		!insertmacro MUI_LANGDLL_DISPLAY
	${ElseIf} ${UAC_IsAdmin}
		!insertmacro MUI_LANGDLL_DISPLAY
	${Else}
		System::Call 'kernel32::CreateMutexA(i 0, i 0, t "{2B3D1EBF-B3B6-4E93-92B9-6853029A7162}") i .r1 ?e'
		Pop $0
		StrCmp $0 0 +3
		MessageBox MB_ICONSTOP|MB_TOPMOST "Sorry, the un-installer is already running!"
		Quit
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
	
	!ifdef LAMEXP_IS_PRERELEASE
		!insertmacro GetCommandlineParameter "Update" "?" $R0
		StrCmp $R0 "?" 0 SkipPrereleaseWarning
		MessageBox MB_TOPMOST|MB_ICONEXCLAMATION|MB_OKCANCEL "$(LAMEXP_LANG_PRERELEASE_WARNING)" /SD IDOK IDOK +2
		Abort
		SkipPrereleaseWarning:
	!endif
	
	Aero::Apply
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
	
	Aero::Apply
FunctionEnd


;--------------------------------
;Macros & Auxiliary Functions
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

!macro TrimStr VarName
	Push ${VarName}
	Call _TrimStr
	Pop ${VarName}
!macroend

Function _TrimStr
	Exch $R1
	Push $R2
 
	TrimLoop1:
	StrCpy $R2 "$R1" 1
	StrCmp "$R2" " " TrimLeft
	StrCmp "$R2" "$\r" TrimLeft
	StrCmp "$R2" "$\n" TrimLeft
	StrCmp "$R2" "$\t" TrimLeft
	Goto TrimLoop2
	TrimLeft:	
	StrCpy $R1 "$R1" "" 1
	Goto TrimLoop1
 
	TrimLoop2:
	StrCpy $R2 "$R1" 1 -1
	StrCmp "$R2" " " TrimRight
	StrCmp "$R2" "$\r" TrimRight
	StrCmp "$R2" "$\n" TrimRight
	StrCmp "$R2" "$\t" TrimRight
	Goto TrimDone
	TrimRight:	
	StrCpy $R1 "$R1" -1
	Goto TrimLoop2
 
	TrimDone:
	Pop $R2
	Exch $R1
FunctionEnd

!macro GetExecutableName OutVar
	!insertmacro GetCommandlineParameter "Update" "LameXP.exe" ${OutVar}
	!insertmacro TrimStr ${OutVar}
	StrCmp ${OutVar} "" 0 +2
	StrCpy ${OutVar} "LameXP.exe"
!macroend


;--------------------------------
;Install Files
;--------------------------------

Section "-PreInit"
	SetShellVarContext all
	SetOutPath "$INSTDIR"
SectionEnd

Section "!Install Files"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_INSTFILES)"

	Delete "$INSTDIR\Changelog.htm"
	Delete "$INSTDIR\Changelog.html"
	Delete "$INSTDIR\Contributors.txt"
	Delete "$INSTDIR\FAQ.html"
	Delete "$INSTDIR\Howto.html"
	Delete "$INSTDIR\LameXP.exe"
	Delete "$INSTDIR\LameXP.exe.sig"
	Delete "$INSTDIR\License.txt"
	Delete "$INSTDIR\ReadMe.txt"
	Delete "$INSTDIR\Translate.html"
	Delete "$INSTDIR\Uninstall.exe"

	!insertmacro GetExecutableName $R0
	File `/oname=$R0` `${LAMEXP_SOURCE_PATH}\LameXP.exe`
	File `${LAMEXP_SOURCE_PATH}\*.txt`
	File `${LAMEXP_SOURCE_PATH}\*.html`
SectionEnd

Section "-Write Uinstaller"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_MAKEUNINST)"
	WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "-Create Shortcuts"
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
		!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_SHORTCUTS)"
		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		
		SetShellVarContext current
		
		Delete "$SMPROGRAMS\$StartMenuFolder\*.lnk"
		Delete "$SMPROGRAMS\$StartMenuFolder\*.pif"
		Delete "$SMPROGRAMS\$StartMenuFolder\*.url"
		
		SetShellVarContext all
		
		Delete "$SMPROGRAMS\$StartMenuFolder\*.lnk"
		Delete "$SMPROGRAMS\$StartMenuFolder\*.pif"
		Delete "$SMPROGRAMS\$StartMenuFolder\*.url"

		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\LameXP.lnk" "$INSTDIR\LameXP.exe" "" "$INSTDIR\LameXP.exe" 0
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_LICENSE).lnk" "$INSTDIR\License.txt"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_CHANGELOG).lnk" "$INSTDIR\Changelog.html"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_TRANSLATE).lnk" "$INSTDIR\Translate.html"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_FAQ).lnk" "$INSTDIR\FAQ.html"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_UNINSTALL).lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
		
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\Official LameXP Homepage.url" "http://mulder.dummwiedeutsch.de/"
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\Doom9's Forum.url" "http://forum.doom9.org/"
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

Section "-Finished"
	!insertmacro PrintProgress "$(MUI_TEXT_FINISH_TITLE)."
	
	; ---- POLL ----
	; !insertmacro UAC_AsUser_ExecShell "" "http://mulder.brhack.net/temp/style_poll/" "" "" SW_SHOWNORMAL
	; ---- POLL ----
SectionEnd


;--------------------------------
;Uninstaller
;--------------------------------

Section "Uninstall"
	SetOutPath "$INSTDIR"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_UNINSTALL)"

	Delete /REBOOTOK "$INSTDIR\LameXP.exe"
	Delete /REBOOTOK "$INSTDIR\LameXP-Portable.exe"
	Delete /REBOOTOK "$INSTDIR\LameXP.exe.sig"
	Delete /REBOOTOK "$INSTDIR\Uninstall.exe"
	Delete /REBOOTOK "$INSTDIR\Changelog.htm"
	Delete /REBOOTOK "$INSTDIR\Changelog.html"
	Delete /REBOOTOK "$INSTDIR\Translate.html"
	Delete /REBOOTOK "$INSTDIR\FAQ.html"
	Delete /REBOOTOK "$INSTDIR\Howto.html"
	Delete /REBOOTOK "$INSTDIR\License.txt"
	Delete /REBOOTOK "$INSTDIR\Contributors.txt"
	Delete /REBOOTOK "$INSTDIR\ReadMe.txt"

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
	
	MessageBox MB_YESNO|MB_TOPMOST "$(LAMEXP_LANG_UNINST_PERSONAL)" IDNO +3
	Delete "$LOCALAPPDATA\LoRd_MuldeR\LameXP - Audio Encoder Front-End\config.ini"
	Delete "$INSTDIR\*.ini"

	!insertmacro PrintProgress "$(MUI_UNTEXT_FINISH_TITLE)."
SectionEnd


;--------------------------------
;Check For Update Mode
;--------------------------------

Function CheckForUpdate
	!insertmacro GetCommandlineParameter "Update" "?" $R0
	StrCmp $R0 "?" 0 EnableUpdateMode

	StrCmp "$INSTDIR" "" 0 +2
	Return
	IfFileExists "$INSTDIR\*.*" +2
	Return
	StrCmp "$EXEDIR" "$INSTDIR" 0 +2
	Return
	IfFileExists "$INSTDIR\LameXP.exe" +2
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
;Locked List
;--------------------------------

Function LockedListShow
	!insertmacro MUI_HEADER_TEXT "$(LAMEXP_LANG_LOCKEDLIST_HEADER)" "$(LAMEXP_LANG_LOCKEDLIST_TEXT)"
	!insertmacro GetExecutableName $R0
	LockedList::AddModule "\$R0"
	LockedList::AddModule "\Uninstall.exe"
	LockedList::AddModule "\Au_.exe"
	LockedList::Dialog /autonext /heading "$(LAMEXP_LANG_LOCKEDLIST_HEADING)" /noprograms "$(LAMEXP_LANG_LOCKEDLIST_NOPROG)" /searching  "$(LAMEXP_LANG_LOCKEDLIST_SEARCH)" /colheadings "$(LAMEXP_LANG_LOCKEDLIST_COLHDR1)" "$(LAMEXP_LANG_LOCKEDLIST_COLHDR2)"
	Pop $R0
FunctionEnd

Function un.LockedListShow
	!insertmacro MUI_HEADER_TEXT "$(LAMEXP_LANG_LOCKEDLIST_HEADER)" "$(LAMEXP_LANG_LOCKEDLIST_TEXT)"
	LockedList::AddModule "\LameXP.exe"
	LockedList::AddModule "\Uninstall.exe"
	LockedList::Dialog /autonext /heading "$(LAMEXP_LANG_LOCKEDLIST_HEADING)" /noprograms "$(LAMEXP_LANG_LOCKEDLIST_NOPROG)" /searching  "$(LAMEXP_LANG_LOCKEDLIST_SEARCH)" /colheadings "$(LAMEXP_LANG_LOCKEDLIST_COLHDR1)" "$(LAMEXP_LANG_LOCKEDLIST_COLHDR2)"
	Pop $R0
FunctionEnd


;--------------------------------
;Install Success
;--------------------------------

Function RunAppFunction
	!insertmacro GetExecutableName $R0
	!insertmacro UAC_AsUser_ExecShell "explore" "$INSTDIR" "" "" SW_SHOWNORMAL
	!insertmacro UAC_AsUser_ExecShell "open" "$INSTDIR\$R0" "" "$INSTDIR" SW_SHOWNORMAL
FunctionEnd

Function ShowReadmeFunction
	!insertmacro UAC_AsUser_ExecShell "open" "$INSTDIR\FAQ.html" "" "" SW_SHOWNORMAL
FunctionEnd


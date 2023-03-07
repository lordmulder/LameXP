; ///////////////////////////////////////////////////////////////////////////////
; // LameXP - Audio Encoder Front-End
; // Copyright (C) 2004-2023 LoRd_MuldeR <MuldeR2@GMX.de>
; //
; // This program is free software; you can redistribute it and/or modify
; // it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
; // the Free Software Foundation; either version 2 of the License, or
; // (at your option) any later version; always including the non-optional
; // LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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

!ifndef NSIS_UNICODE
  !error "NSIS_UNICODE is undefined, please compile with Unicode NSIS !!!"
!endif

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

;UUID
!define MyRegPath "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{FBD7A67D-D700-4043-B54F-DD106D00F308}"

;App Paths
!define AppPaths "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths"

;Web-Site
!define MyWebSite "http://muldersoft.com/"

;Prerequisites
!define PrerequisitesDir "..\..\..\Prerequisites"

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
;Manifest
;--------------------------------

!tempfile PACKHDRTEMP
!packhdr "${PACKHDRTEMP}" '"${PrerequisitesDir}\MSVC\redist\bin\mt.exe" -manifest "setup.manifest" -outputresource:"${PACKHDRTEMP};1" && "${PrerequisitesDir}\UPX\upx.exe" --brute "${PACKHDRTEMP}"'


;--------------------------------
;Includes
;--------------------------------

!include `MUI2.nsh`
!include `WinVer.nsh`
!include `x64.nsh`
!include `StdUtils.nsh`


;--------------------------------
;Installer Attributes
;--------------------------------

RequestExecutionLevel admin
ShowInstDetails show
ShowUninstDetails show
Name "LameXP v${LAMEXP_VERSION} ${LAMEXP_INSTTYPE}-${LAMEXP_PATCH} [Build #${LAMEXP_BUILD}]"
OutFile "${LAMEXP_OUTPUT_FILE}"
BrandingText "Date created: ${LAMEXP_DATE} [Build #${LAMEXP_BUILD}]"
InstallDir "$PROGRAMFILES\MuldeR\LameXP"
InstallDirRegKey HKLM "${MyRegPath}" "InstallLocation"


;--------------------------------
;Compressor
;--------------------------------

SetCompressor /SOLID LZMA
SetCompressorDictSize 64


;--------------------------------
;Reserved Files
;--------------------------------

ReserveFile "${NSISDIR}\Plugins\Aero.dll"
ReserveFile "${NSISDIR}\Plugins\LangDLL.dll"
ReserveFile "${NSISDIR}\Plugins\LockedList.dll"
ReserveFile "${NSISDIR}\Plugins\LockedList64.dll"
ReserveFile "${NSISDIR}\Plugins\nsDialogs.dll"
ReserveFile "${NSISDIR}\Plugins\nsExec.dll"
ReserveFile "${NSISDIR}\Plugins\StartMenu.dll"
ReserveFile "${NSISDIR}\Plugins\StdUtils.dll"
ReserveFile "${NSISDIR}\Plugins\System.dll"
ReserveFile "${NSISDIR}\Plugins\UserInfo.dll"


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
VIAddVersionKey "Comments" "This program is free software; you can redistribute it and/or modify it under the terms of the GNU GENERAL PUBLIC LICENSE as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version; always including the non-optional LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM."
VIAddVersionKey "CompanyName" "Free Software Foundation"
VIAddVersionKey "FileDescription" "LameXP v${LAMEXP_VERSION} ${LAMEXP_INSTTYPE}-${LAMEXP_PATCH} [Build #${LAMEXP_BUILD}]"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION_DATE}.${LAMEXP_BUILD} (${LAMEXP_VERSION})"
VIAddVersionKey "LegalCopyright" "Copyright 2004-2023 LoRd_MuldeR"
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
!define MUI_CUSTOMFUNCTION_GUIINIT MyGuiInit
!define MUI_CUSTOMFUNCTION_UNGUIINIT un.MyGuiInit
!define MUI_LANGDLL_ALWAYSSHOW


;--------------------------------
;MUI2 Pages (Installer)
;--------------------------------

;Welcome
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipIfUnattended
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE CheckForPreRelease
!define MUI_WELCOMEPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_TITLE_3LINES
!insertmacro MUI_PAGE_WELCOME

;License
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipIfUnattended
!insertmacro MUI_PAGE_LICENSE "license.rtf"

;Directory
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipIfUnattended
!define MUI_PAGE_CUSTOMFUNCTION_SHOW CheckForUpdate
!insertmacro MUI_PAGE_DIRECTORY

;Startmenu
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipIfUnattended
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

;LockedList
Page Custom LockedListShow

;Install Files
!insertmacro MUI_PAGE_INSTFILES

;Finish
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipIfUnattended
!insertmacro MUI_PAGE_FINISH


;--------------------------------
;MUI2 Pages (Uninstaller)
;--------------------------------

;Welcome
!define MUI_WELCOMEPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_TITLE_3LINES
!define MUI_PAGE_CUSTOMFUNCTION_PRE un.CheckForcedUninstall
!insertmacro MUI_UNPAGE_WELCOME

;Confirm
!define MUI_PAGE_CUSTOMFUNCTION_PRE un.CheckForcedUninstall
!insertmacro MUI_UNPAGE_CONFIRM

;LockedList
UninstPage Custom un.LockedListShow

;Uninstall
!insertmacro MUI_UNPAGE_INSTFILES

;Finish
!define MUI_PAGE_CUSTOMFUNCTION_PRE un.CheckForcedUninstall
!insertmacro MUI_UNPAGE_FINISH


;--------------------------------
;Languages
;--------------------------------
 
!insertmacro MUI_LANGUAGE "English" ;first language is the default language
!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Ukrainian"

; !insertmacro MUI_LANGUAGE "Afrikaans"
; !insertmacro MUI_LANGUAGE "Arabic"
; !insertmacro MUI_LANGUAGE "Dutch"
; !insertmacro MUI_LANGUAGE "French"
; !insertmacro MUI_LANGUAGE "Greek"
; !insertmacro MUI_LANGUAGE "Indonesian"
; !insertmacro MUI_LANGUAGE "Italian"
; !insertmacro MUI_LANGUAGE "Malay"
; !insertmacro MUI_LANGUAGE "Portuguese"
; !insertmacro MUI_LANGUAGE "Romanian"
; !insertmacro MUI_LANGUAGE "Serbian"
; !insertmacro MUI_LANGUAGE "SerbianLatin"
; !insertmacro MUI_LANGUAGE "SimpChinese"
; !insertmacro MUI_LANGUAGE "SpanishInternational"
; !insertmacro MUI_LANGUAGE "TradChinese"


;--------------------------------
;Translation
;--------------------------------

;English
!include "..\Translation\Blank.nsh" ;first language is the default language

;Bulgarian
!include "..\Translation\LameXP_BG.nsh"

;German
!include "..\Translation\LameXP_DE.nsh"

;Hungarian
!include "..\Translation\LameXP_HU.nsh"

;Japanese
!include "..\Translation\LameXP_JA.nsh"

;Polish
!include "..\Translation\LameXP_PL.nsh"

;Russian
!include "..\Translation\LameXP_RU.nsh"

;Spanish
!include "..\Translation\LameXP_ES.nsh"

;Ukrainian
!include "..\Translation\LameXP_UK.nsh"


;--------------------------------
;LogicLib Extensions
;--------------------------------

!macro _UnattendedMode _a _b _t _f
	!insertmacro _LOGICLIB_TEMP
	${StdUtils.TestParameter} $_LOGICLIB_TEMP "Update"
	StrCmp "$_LOGICLIB_TEMP" "true" `${_t}` `${_f}`
!macroend
!define UnattendedMode `"" UnattendedMode ""`

!macro _ForcedMode _a _b _t _f
	!insertmacro _LOGICLIB_TEMP
	${StdUtils.TestParameter} $_LOGICLIB_TEMP "Force"
	StrCmp "$_LOGICLIB_TEMP" "true" `${_t}` `${_f}`
!macroend
!define ForcedMode `"" ForcedMode ""`

!macro _ValidFileName _a _b _t _f
	!insertmacro _LOGICLIB_TEMP
	${StdUtils.ValidFileName} $_LOGICLIB_TEMP `${_b}`
	StrCmp "$_LOGICLIB_TEMP" "ok" `${_t}` `${_f}`
!macroend
!define ValidFileName `"" ValidFileName`


;--------------------------------
;Installer initialization
;--------------------------------

Function .onInit
	InitPluginsDir

	; --------

	System::Call 'kernel32::CreateMutexA(i 0, i 0, t "{2B3D1EBF-B3B6-4E93-92B9-6853029A7162}") i .r1 ?e'
	Pop $0
	${If} $0 <> 0
		MessageBox MB_ICONSTOP|MB_TOPMOST "Sorry, the installer is already running!"
		Quit
	${EndIf}

	; --------
	
	# Running on Windows NT family?
	${IfNot} ${IsNT}
		MessageBox MB_TOPMOST|MB_ICONSTOP "Sorry, this application does *not* support Windows 9x/ME!"
		ExecShell "open" "http://windows.microsoft.com/"
		Quit
	${EndIf}

	# Running on Windows XP or later?
	${If} ${AtMostWin2000}
		MessageBox MB_TOPMOST|MB_ICONSTOP "Sorry, but your operating system is *not* supported anymore.$\nInstallation will be aborted!$\n$\nThe minimum required platform is Windows XP (Service Pack 3)."
		ExecShell "open" "http://windows.microsoft.com/"
		Quit
	${EndIf}

	# If on Windows XP, is the required Service Pack installed?
	${If} ${IsWinXP}
		${IfNot} ${RunningX64} # Windows XP 32-Bit, requires Service Pack 3
		${AndIf} ${AtMostServicePack} 2
			MessageBox MB_TOPMOST|MB_ICONEXCLAMATION "This application requires Service Pack 3 for Windows XP.$\nPlease install the required Service Pack and retry!"
			Quit
		${EndIf}
		${If} ${RunningX64} # Windows XP 64-Bit, requires Service Pack 2
		${AndIf} ${AtMostServicePack} 1
			MessageBox MB_TOPMOST|MB_ICONEXCLAMATION "This application requires Service Pack 2 for Windows XP x64.$\nPlease install the required Service Pack and retry!"
			Quit
		${EndIf}
		${IfNot} ${UnattendedMode}
			${If} ${Cmd} `MessageBox MB_TOPMOST|MB_ICONEXCLAMATION|MB_OKCANCEL|MB_DEFBUTTON2 "It appears that you are still running Windows XP, which reached $\"end of life$\" in April 2014 and therefore will not receive any updates or bug fixes anymore - not even for critical security vulnerabilities! We highly recommend updating to a contemporary operating system.$\n$\nClick 'OK' to proceed with the installation, or click 'Cancel' to abort." IDCANCEL`
				Quit
			${EndIf}
		${EndIf}
	${EndIf}

	# Running on Windows Vista?
	${If} ${IsWinVista}
		${If} ${AtMostServicePack} 1
			MessageBox MB_TOPMOST|MB_ICONEXCLAMATION "This application requires Service Pack 2 for Windows Vista.$\nPlease install the required Service Pack and retry!"
			Quit
		${EndIf}
		${IfNot} ${UnattendedMode}
			${If} ${Cmd} `MessageBox MB_TOPMOST|MB_ICONEXCLAMATION|MB_OKCANCEL|MB_DEFBUTTON2 "It appears that you are still running Windows Vista, which reached $\"end of life$\" in April 2017 and therefore will not receive any updates or bug fixes anymore - not even for critical security vulnerabilities! We highly recommend updating to a contemporary operating system.$\n$\nClick 'OK' to proceed with the installation, or click 'Cancel' to abort." IDCANCEL`
				Quit
			${EndIf}
		${EndIf}
	${EndIf}

	; --------

	${IfNot} ${UnattendedMode}
		!insertmacro MUI_LANGDLL_DISPLAY
	${EndIf}

	; --------

	UserInfo::GetAccountType
	Pop $0
	${If} $0 != "Admin"
		MessageBox MB_ICONSTOP|MB_TOPMOST "Your system requires administrative permissions in order to install this software."
		SetErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
		Quit
	${EndIf}
FunctionEnd

Function un.onInit
	System::Call 'kernel32::CreateMutexA(i 0, i 0, t "{2B3D1EBF-B3B6-4E93-92B9-6853029A7162}") i .r1 ?e'
	Pop $0
	${If} $0 <> 0
		MessageBox MB_ICONSTOP|MB_TOPMOST "Sorry, the un-installer is already running!"
		Quit
	${EndIf}

	${IfNot} ${ForcedMode}
		!insertmacro MUI_LANGDLL_DISPLAY
	${EndIf}
	
	; --------

	UserInfo::GetAccountType
	Pop $0
	${If} $0 != "Admin"
		MessageBox MB_ICONSTOP|MB_TOPMOST "Your system requires administrative permissions in order to install this software."
		SetErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
		Quit
	${EndIf}
FunctionEnd


;--------------------------------
;GUI initialization
;--------------------------------

Function MyGuiInit
	StrCpy $0 $HWNDPARENT
	System::Call "user32::SetWindowPos(i r0, i -1, i 0, i 0, i 0, i 0, i 3)"
	Aero::Apply
FunctionEnd

Function un.MyGuiInit
	StrCpy $0 $HWNDPARENT
	System::Call "user32::SetWindowPos(i r0, i -1, i 0, i 0, i 0, i 0, i 3)"
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

!macro GetExecutableName OutVar
	${StdUtils.GetParameter} ${OutVar} "Update" ""
	${StdUtils.TrimStr} ${OutVar}
	${If} "${OutVar}" == ""
	${OrIfNot} ${ValidFileName} "${OutVar}"
		StrCpy ${OutVar} "LameXP.exe"
	${EndIf}
!macroend

!macro DisableNextButton TmpVar
	GetDlgItem ${TmpVar} $HWNDPARENT 1
	EnableWindow ${TmpVar} 0
!macroend

!macro DisableBackButton TmpVar
	GetDlgItem ${TmpVar} $HWNDPARENT 3
	EnableWindow ${TmpVar} 0
!macroend

!macro CleanUpFiles options
	Delete ${options} "$INSTDIR\Changelog.htm"
	Delete ${options} "$INSTDIR\Changelog.html"
	Delete ${options} "$INSTDIR\Contributors.txt"
	Delete ${options} "$INSTDIR\Copying.txt"
	Delete ${options} "$INSTDIR\FAQ.html"
	Delete ${options} "$INSTDIR\Howto.html"
	Delete ${options} "$INSTDIR\LameEnc.sys"
	Delete ${options} "$INSTDIR\LameXP*.exe"
	Delete ${options} "$INSTDIR\LameXP*.exe.sig"
	Delete ${options} "$INSTDIR\LameXP*.rcc"
	Delete ${options} "$INSTDIR\LameXP*.VisualElementsManifest.xml"
	Delete ${options} "$INSTDIR\LameXP*.tag"
	Delete ${options} "$INSTDIR\License.txt"
	Delete ${options} "$INSTDIR\Manual.html"
	Delete ${options} "$INSTDIR\Readme.htm"
	Delete ${options} "$INSTDIR\Readme.html"
	Delete ${options} "$INSTDIR\ReadMe.txt"
	Delete ${options} "$INSTDIR\PRE_RELEASE_INFO.txt"
	Delete ${options} "$INSTDIR\Settings.cfg"
	Delete ${options} "$INSTDIR\Translate.html"
	Delete ${options} "$INSTDIR\Uninstall.exe"
	Delete ${options} "$INSTDIR\Qt*.dll"
	Delete ${options} "$INSTDIR\MUtils*.dll"
	Delete ${options} "$INSTDIR\msvcr*.dll"
	Delete ${options} "$INSTDIR\msvcp*.dll"
	Delete ${options} "$INSTDIR\concrt*.dll"
	Delete ${options} "$INSTDIR\vcruntime*.dll"
	Delete ${options} "$INSTDIR\vccorlib*.dll"
	Delete ${options} "$INSTDIR\api-ms-*.dll"
	Delete ${options} "$INSTDIR\ucrtbase.dll"
	
	RMDir /r ${options} "$INSTDIR\cache"
	RMDir /r ${options} "$INSTDIR\img"
	RMDir /r ${options} "$INSTDIR\imageformats"
	RMDir /r ${options} "$INSTDIR\redist"
!macroend

!macro InstallRedist srcdir filename
	File /a `/oname=$PLUGINSDIR\${filename}` `${srcdir}\${filename}`
	ExecWait `"$PLUGINSDIR\${filename}" /passive`
!macroend

;--------------------------------
;Install Files
;--------------------------------

Section "-PreInit"
	SetShellVarContext all
	SetOutPath "$INSTDIR"
SectionEnd

Section "-Clean Up Old Cruft"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_CLEANUP)"
	!insertmacro CleanUpFiles ""
SectionEnd

Section "!Install Files"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_INSTFILES)"
	
	DeleteOldBinary:
	!insertmacro GetExecutableName $R0
	ClearErrors
	Delete "$INSTDIR\$R0"
	
	${If} ${Errors}
		MessageBox MB_TOPMOST|MB_ICONSTOP|MB_RETRYCANCEL 'Could not delete old "$R0" file. Is LameXP still running?' IDRETRY DeleteOldBinary
		Abort "Could not delete old binary!"
	${EndIf}
	
	File /a `/oname=$R0` `${LAMEXP_SOURCE_PATH}\LameXP.exe`
	File /nonfatal /a /r `${LAMEXP_SOURCE_PATH}\*.dll`
	File /nonfatal /a /r `${LAMEXP_SOURCE_PATH}\*.rcc`
	
	${StdUtils.GetFileNamePart} $R1 "$R0"
	File /a `/oname=$R1.VisualElementsManifest.xml` `${LAMEXP_SOURCE_PATH}\LameXP.VisualElementsManifest.xml`
	File /a `/oname=$R1.tag` `${LAMEXP_SOURCE_PATH}\LameXP.tag`
	
	File /a /r `${LAMEXP_SOURCE_PATH}\*.txt`
	File /a /r `${LAMEXP_SOURCE_PATH}\*.html`
	File /a /r `${LAMEXP_SOURCE_PATH}\*.png`
SectionEnd

Section "-Write Uninstaller"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_MAKEUNINST)"
	WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "-Install Runtime Libraries"
	${If} ${AtMostWinXP}
	${AndIfNot} ${FileExists} `$SYSDIR\normaliz.dll`
		!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_VCREDIST)"
		${If} ${RunningX64}
			!insertmacro InstallRedist '${PrerequisitesDir}\IDNMInstaller' 'idndl.x64.exe'
		${Else}
			!insertmacro InstallRedist '${PrerequisitesDir}\IDNMInstaller' 'idndl.x86.exe'
		${EndIf}
	${EndIf}
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

		!insertmacro GetExecutableName $R0
		
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\LameXP.lnk"                        "$INSTDIR\$R0" "" "$INSTDIR\$R0" 0
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_LICENSE).lnk"   "$INSTDIR\License.txt"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_CHANGELOG).lnk" "$INSTDIR\Changelog.html"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_TRANSLATE).lnk" "$INSTDIR\Translate.html"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_MANUAL).lnk"    "$INSTDIR\Manual.html"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(LAMEXP_LANG_LINK_UNINSTALL).lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
		
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\Official LameXP Homepage.url" "${MyWebSite}"
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\Doom9's Forum.url"            "http://forum.doom9.org/"
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\Bug Tracker.url"              "https://github.com/lordmulder/LameXP/issues"
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\RareWares.org.url"            "http://rarewares.org/"
		!insertmacro CreateWebLink "$SMPROGRAMS\$StartMenuFolder\Hydrogenaudio Forums.url"     "http://www.hydrogenaudio.org/"

		${If} ${FileExists} "$SMPROGRAMS\$StartMenuFolder\LameXP.lnk"
			${StdUtils.InvokeShellVerb} $R1 "$SMPROGRAMS\$StartMenuFolder" "LameXP.lnk" ${StdUtils.Const.ShellVerb.PinToTaskbar}
			DetailPrint 'Pin: "$SMPROGRAMS\$StartMenuFolder\LameXP.lnk" -> $R1'
		${EndIf}
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "-Update Registry"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_REGISTRY)"

	!insertmacro GetExecutableName $R0
	WriteRegStr HKLM "${MyRegPath}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "${MyRegPath}" "ExecutableName" "$R0"
	WriteRegStr HKLM "${MyRegPath}" "DisplayIcon" "$INSTDIR\$R0,0"
	WriteRegStr HKLM "${MyRegPath}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	WriteRegStr HKLM "${MyRegPath}" "DisplayName" "LameXP v${LAMEXP_VERSION}"
	WriteRegStr HKLM "${MyRegPath}" "Publisher" "LoRd_MuldeR <mulder2@gmx.de>"
	WriteRegStr HKLM "${MyRegPath}" "DisplayVersion" "${LAMEXP_VERSION} ${LAMEXP_INSTTYPE}-${LAMEXP_PATCH} [Build #${LAMEXP_BUILD}]"
	WriteRegStr HKLM "${MyRegPath}" "URLInfoAbout" "${MyWebSite}"
	WriteRegStr HKLM "${MyRegPath}" "URLUpdateInfo" "${MyWebSite}"
	
	DeleteRegKey HKCU "${AppPaths}\LameXP.exe"
	WriteRegStr HKLM "${AppPaths}\LameXP.exe" "" "$INSTDIR\$R0"
	WriteRegStr HKLM "${AppPaths}\LameXP.exe" "Path" "$INSTDIR"
SectionEnd

Section "-Finished"
	!insertmacro PrintProgress "$(MUI_TEXT_FINISH_TITLE)."

!ifdef LAMEXP_IS_PRERELEASE
	${If} ${FileExists} "$INSTDIR\PRE_RELEASE_INFO.txt"
		${StdUtils.ExecShellAsUser} $R1 "$INSTDIR\PRE_RELEASE_INFO.txt" "open" ""
	${EndIf}
!endif

	${IfThen} ${UnattendedMode} ${|} SetAutoClose true ${|}
SectionEnd


;--------------------------------
;Uninstaller
;--------------------------------

Section "Uninstall"
	SetOutPath "$EXEDIR"
	!insertmacro PrintProgress "$(LAMEXP_LANG_STATUS_UNINSTALL)"

	; --------------
	; Startmenu
	; --------------
	
	!insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
	${IfNot} "$StartMenuFolder" == ""
		SetShellVarContext current
		${If} ${FileExists} "$SMPROGRAMS\$StartMenuFolder\LameXP.lnk"
			${StdUtils.InvokeShellVerb} $R1 "$SMPROGRAMS\$StartMenuFolder" "LameXP.lnk" ${StdUtils.Const.ShellVerb.UnpinFromTaskbar}
			DetailPrint 'Unpin: "$SMPROGRAMS\$StartMenuFolder\LameXP.lnk" -> $R1'
		${EndIf}
		${If} ${FileExists} "$SMPROGRAMS\$StartMenuFolder\*.*"
			Delete /REBOOTOK "$SMPROGRAMS\$StartMenuFolder\*.lnk"
			Delete /REBOOTOK "$SMPROGRAMS\$StartMenuFolder\*.url"
			RMDir "$SMPROGRAMS\$StartMenuFolder"
		${EndIf}
		
		SetShellVarContext all
		${If} ${FileExists} "$SMPROGRAMS\$StartMenuFolder\LameXP.lnk"
			${StdUtils.InvokeShellVerb} $R1 "$SMPROGRAMS\$StartMenuFolder" "LameXP.lnk" ${StdUtils.Const.ShellVerb.UnpinFromTaskbar}
			DetailPrint 'Unpin: "$SMPROGRAMS\$StartMenuFolder\LameXP.lnk" -> $R1'
		${EndIf}
		${If} ${FileExists} "$SMPROGRAMS\$StartMenuFolder\*.*"
			Delete /REBOOTOK "$SMPROGRAMS\$StartMenuFolder\*.lnk"
			Delete /REBOOTOK "$SMPROGRAMS\$StartMenuFolder\*.url"
			RMDir "$SMPROGRAMS\$StartMenuFolder"
		${EndIf}
	${EndIf}

	; --------------
	; Files
	; --------------

	ReadRegStr $R0 HKLM "${MyRegPath}" "ExecutableName"
	${IfThen} "$R0" == "" ${|} StrCpy $R0 "LameXP.exe" ${|}
	ExecWait '"$INSTDIR\$R0" --uninstall'

	Delete /REBOOTOK "$INSTDIR\$R0"
	!insertmacro CleanUpFiles /REBOOTOK
	RMDir "$INSTDIR"

	; --------------
	; Registry
	; --------------
	
	DeleteRegKey HKLM "${MyRegPath}"
	DeleteRegKey HKCU "${MyRegPath}"

	DeleteRegKey HKLM "SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{54dcbccb-c905-46dc-b6e6-48563d0e9e55}"
	DeleteRegKey HKCU "SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{54dcbccb-c905-46dc-b6e6-48563d0e9e55}"
	
	DeleteRegKey HKLM "${AppPaths}\LameXP.exe"
	DeleteRegKey HKCU "${AppPaths}\LameXP.exe"

	MessageBox MB_YESNO|MB_TOPMOST "$(LAMEXP_LANG_UNINST_PERSONAL)" IDNO +3
	Delete "$LOCALAPPDATA\LoRd_MuldeR\LameXP - Audio Encoder Front-End\config.ini"
	Delete "$INSTDIR\*.ini"

	!insertmacro PrintProgress "$(MUI_UNTEXT_FINISH_TITLE)."
SectionEnd


;--------------------------------
;Check For Update Mode
;--------------------------------

Function SkipIfUnattended
	${IfThen} ${UnattendedMode} ${|} Abort ${|}
FunctionEnd

Function CheckForUpdate
	${If} "$INSTDIR" == ""
	${OrIf} "$INSTDIR" == "$EXEDIR"
	${OrIfNot} ${FileExists} "$INSTDIR\LameXP.exe"
		Return
	${EndIf}

	FindWindow $R0 "#32770" "" $HWNDPARENT
	GetDlgItem $R1 $R0 1019
	EnableWindow $R1 0

	FindWindow $R0 "#32770" "" $HWNDPARENT
	GetDlgItem $R1 $R0 1001
	EnableWindow $R1 0
FunctionEnd

Function un.CheckForcedUninstall
	${IfThen} ${ForcedMode} ${|} Abort ${|}
FunctionEnd


;--------------------------------
;Check For Pre-Release
;--------------------------------

Function CheckForPreRelease
	!ifdef LAMEXP_IS_PRERELEASE
		${IfNot} ${UnattendedMode}
			MessageBox MB_TOPMOST|MB_ICONEXCLAMATION|MB_OKCANCEL "$(LAMEXP_LANG_PRERELEASE_WARNING)" /SD IDOK IDOK +2
			Quit
		${EndIf}
	!endif
FunctionEnd


;--------------------------------
;Locked List
;--------------------------------

!macro _LockedListShow uinst
	!insertmacro MUI_HEADER_TEXT "$(LAMEXP_LANG_LOCKEDLIST_HEADER)" "$(LAMEXP_LANG_LOCKEDLIST_TEXT)"
	${If} ${UnattendedMode}
		!insertmacro DisableBackButton $R0
	${EndIf}
	${If} ${RunningX64}
		InitPluginsDir
		File /oname=$PLUGINSDIR\LockedList64.dll `${NSISDIR}\Plugins\LockedList64.dll`
	${EndIf}
	!insertmacro GetExecutableName $R0
	LockedList::AddModule "\$R0"
	${If} "$R0" != "LameXP.exe"
		LockedList::AddModule "\LameXP.exe"
	${EndIf}
	LockedList::AddModule "\Uninstall.exe"
	!if ${uinst} < 1
		LockedList::AddModule "\Au_.exe"
	!endif
	#LockedList::AddFolder "$INSTDIR"
	LockedList::Dialog /autonext /heading "$(LAMEXP_LANG_LOCKEDLIST_HEADING)" /noprograms "$(LAMEXP_LANG_LOCKEDLIST_NOPROG)" /searching  "$(LAMEXP_LANG_LOCKEDLIST_SEARCH)" /colheadings "$(LAMEXP_LANG_LOCKEDLIST_COLHDR1)" "$(LAMEXP_LANG_LOCKEDLIST_COLHDR2)"
	Pop $R0
!macroend

Function LockedListShow
	!insertmacro _LockedListShow 0
FunctionEnd

Function un.LockedListShow
	!insertmacro _LockedListShow 1
FunctionEnd


;--------------------------------
;Install Success
;--------------------------------

Function RunAppFunction
	!insertmacro DisableNextButton $R0
	!insertmacro GetExecutableName $R0
	${StdUtils.ExecShellAsUser} $R1 "$INSTDIR" "explore" ""
	${StdUtils.ExecShellAsUser} $R1 "$INSTDIR\$R0" "open" "--first-run"
FunctionEnd

Function ShowReadmeFunction
	!insertmacro DisableNextButton $R0
	${StdUtils.ExecShellAsUser} $R1 "$INSTDIR\Manual.html" "open" ""
FunctionEnd

Function .onInstSuccess
	${If} ${UnattendedMode}
		!insertmacro GetExecutableName $R0
		${StdUtils.ExecShellAsUser} $R1 "$INSTDIR\$R0" "open" "--first-run"
	${EndIf}
FunctionEnd

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

RequestExecutionLevel user
BrandingText `Date created: ${LAMEXP_DATE}`
ShowInstDetails show

!define ZIP2EXE_NAME `LameXP v${LAMEXP_VERSION} ${LAMEXP_SUFFIX} [Build #${LAMEXP_BUILD}]`
!define ZIP2EXE_OUTFILE `${LAMEXP_OUTPUT_FILE}`
!define ZIP2EXE_COMPRESSOR_LZMA
!define ZIP2EXE_COMPRESSOR_SOLID
!define ZIP2EXE_INSTALLDIR `$PROGRAMFILES\${ZIP2EXE_NAME}`

!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\orange.bmp"
!define MUI_PAGE_CUSTOMFUNCTION_SHOW CheckForUpdate

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

!include `UAC.nsh`
!include `parameters.nsh`
!include `${NSISDIR}\Contrib\zip2exe\Base.nsh`
!include `${NSISDIR}\Contrib\zip2exe\Modern.nsh`

Function .onInit
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

!insertmacro SECTION_BEGIN
	File /r `${LAMEXP_SOURCE_PATH}\*.*`
!insertmacro SECTION_END

Function CheckForUpdate
	!insertmacro GetCommandlineParameter "Update" "error" $R0
	StrCmp $R0 "error" 0 +2
	Return

	FindWindow $R0 "#32770" "" $HWNDPARENT
	GetDlgItem $R1 $R0 1019
	SendMessage $R1 ${EM_SETREADONLY} 1 0

	FindWindow $R0 "#32770" "" $HWNDPARENT
	GetDlgItem $R1 $R0 1001
	EnableWindow $R1 0

	GetDlgItem $R1 $HWNDPARENT 1
	SendMessage $R1 ${WM_SETTEXT} 0 "STR:Update"
FunctionEnd

Function .onInstSuccess
	StrCpy $R0 "$INSTDIR"
	UAC_AsUser_ExecShell "explore" "$R0" "" "" SW_SHOWNORMAL
	UAC_AsUser_ExecShell "open" "$R0\LameXP.exe" "" "$OUTDIR" SW_SHOWNORMAL
FunctionEnd

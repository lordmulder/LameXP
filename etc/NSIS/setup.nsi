!define ZIP2EXE_NAME `LameXP v${LAMEXP_VERSION} ${LAMEXP_SUFFIX} [Build #${LAMEXP_BUILD}]`
!define ZIP2EXE_OUTFILE `${LAMEXP_OUTPUT_FILE}`
!define ZIP2EXE_COMPRESSOR_LZMA
!define ZIP2EXE_COMPRESSOR_SOLID
!define ZIP2EXE_INSTALLDIR `$PROGRAMFILES\${ZIP2EXE_NAME}`

RequestExecutionLevel user
BrandingText `Date created: ${LAMEXP_DATE}`
ShowInstDetails show

!define MUI_PAGE_CUSTOMFUNCTION_SHOW CheckForUpdate

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
			MessageBox MB_ICONEXCLAMATION|MB_TOPMOST|MB_SETFOREGROUND "This installer requires admin access, please try again!" /SD IDNO IDOK UAC_TryAgain IDNO 0
		${EndIf}
	${Case} 1223
		MessageBox MB_ICONEXCLAMATION|MB_TOPMOST|MB_SETFOREGROUND "This installer requires admin privileges, please try again!" /SD IDNO IDOK UAC_TryAgain IDNO 0
		Quit
	${Case} 1062
		MessageBox MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Logon service not running, aborting!"
		Quit
	${Default}
		MessageBox MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Unable to elevate installer! (Error code: $0)"
		Quit
	${EndSwitch}
FunctionEnd

Function .onInstSuccess
	!insertmacro UAC_AsUser_ExecShell "open" "$INSTDIR\LameXP.exe" "" "$INSTDIR" SW_SHOWNORMAL 
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

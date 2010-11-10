!define ZIP2EXE_NAME `LameXP v${LAMEXP_VERSION} ${LAMEXP_SUFFIX} [Build #${LAMEXP_BUILD}]`
!define ZIP2EXE_OUTFILE `${LAMEXP_OUTPUT_FILE}`
!define ZIP2EXE_COMPRESSOR_LZMA
!define ZIP2EXE_COMPRESSOR_SOLID
!define ZIP2EXE_INSTALLDIR `$PROGRAMFILES\${ZIP2EXE_NAME}`
BrandingText `Date created: ${LAMEXP_DATE}`
!include `${NSISDIR}\Contrib\zip2exe\Base.nsh`
!include `${NSISDIR}\Contrib\zip2exe\Modern.nsh`
!insertmacro SECTION_BEGIN
File /r `${LAMEXP_SOURCE_PATH}\*.*`
!insertmacro SECTION_END

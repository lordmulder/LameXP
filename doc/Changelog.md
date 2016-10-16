% LameXP Audio-Encoder Front-End &ndash; Changelog

# LameXP v4.xx History #

## LameXP v4.14 [2016-??-??] ## {-}

* Upgraded build environment to Microsoft Visual Studio 2015 with Update-2
* Fixed the location of temporary intermediate files for SoX-based audio effects
* Fixed embedding of meta tags with OggEnc2 when reading directly from OGG/FLAC input file
* Enabled the "built-in" resampler for QAAC encoder
* The "Algorithm Quality" slider now also affects the QAAC encoder
* Added "AVX" (Advanced Vector Extensions) to CPU feature detection code
* Updated Opus encoder/decoder libraries to v1.1.3 and Opus-Tools to v0.1.9 (2016-10-16)
* Updated LAME encoder to v3.100 Alpha-2 (2016-01-29), compiled with ICL 15.0 and MSVC 12.0
* Updated FLAC encoder/decoder to v1.3.1 (2016-10-04), compiled with ICL 17.0 and MSVC 12.0
* Updated MediaInfo to v0.7.88 (2016-08-31), compiled with ICL 15.0 and MSVC 12.0
* Updated mpg123 decoder to v1.23.4 (2016-05-11), compiled with GCC 5.3.0
* Updated ALAC decoder to refalac v1.61 (2016-10-02)
* Updated WavPack decoder to v4.80.0 (2016-03-28), compiled with ICL 15.0 and MSVC 12.0
* Updated GnuPG to v1.4.21 (2016-08-17), compiled with GCC 6.1.0
* Updated QAAC add-in to the to QAAC v2.61 (2016-10-02)
* Updated FhgAacEnc add-in to "Case" edition (2015-10-24)
* Improved auto-update function (faster Internet connectivity check)

## LameXP v4.13 [2015-12-12] ## {-}

* Upgraded build environment to Microsoft Visual Studio 2015 with Update-1
* Apply the original file's "creation" and "last modified" date/time to the output file (optional)
* Updated Vorbis encoder to OggEnc v2.88 (2015-09-10), using libvorbis v1.3.5 and aoTuV b6.03_2015
* Updated MediaInfo to v0.7.78 (2015-10-02), compiled with ICL 15.0 and MSVC 12.0
* Fixed resampling bug with Vorbis encoder, regression in OggEnc v2.87
* Fixed creation of Monkey's Audio (APE) files, when **no** meta data is being embedded
* Updated language files (big thank-you to all contributors !!!)


## LameXP v4.12 [2015-10-23] ## {-}

* Upgraded build environment to Microsoft Visual Studio 2013 with Update-5
* Updated Qt runtime libraries to v4.8.7 Final (2015-05-25), compiled with MSVC 12.0
* Added support for building LameXP and MUtilities with Visual Studio 2015
* Added Hungarian translation, contributed by Zityi's Translator Team <<zityisoft@gmail.com>>
* Added optional support for the *libfdk-aac* encoder, using the [fdkaac](https://github.com/nu774/fdkaac) front-end by nu774
* Added detection of the *64-Bit* version of QAAC encoder, requires 64-Bit Apple Application Support
* Added enhanced file renaming option: Default file extensions can now be overwritten
* Added enhanced file renaming option: Files can now be renamed via the [regular expression](http://www.regular-expressions.info/quickstart.html) engine
* Added capability to select *multiple* files on "Source Files" tab
* Updated Vorbis encoder to OggEnc v2.87 (2015-08-03), using libvorbis v1.3.5 and aoTuV b6.03_2015
* Updated MediaInfo to v0.7.76 (2015-08-06), compiled with ICL 15.0 and MSVC 12.0
* Updated mpg123 decoder to v1.22.4 (2015-08-12), compiled with GCC 5.1.0
* Updated ALAC decoder to refalac v1.47 (2015-02-15), based on reference implementation by Apple
* Updated Monkey's Audio binary to v4.16 (2015-03-24), compiled with ICL 15.0 and MSVC 12.0
* Updated WavPack decoder to v4.75.0 (2015-05-25), compiled with ICL 15.0 and MSVC 12.0
* Updated GnuPG to v1.4.19 (2015-02-27), compiled with GCC 4.9.2
* Fixed potential deadlock in Cue Sheet import dialog when "Browse..." button is clicked
* Fixed function to restore the default Temp folder, if custom Temp folder doesn't exist anymore
* Fixed parsing of command-line parameters, regression in MUtilities library (LameXP v4.12 RC-1)
* QAAC encoder is now using `--cvbr` instead of `--abr` when "ABR" mode is selected
* Enable the embedding of cover artwork for Opus encoder (opusenc), using the ``--picture`` option
* Some installer improvements have been implemented (especially in "update" mode)
* Full support for Windows 10 RTM (Build #10240)
* Updated language files (big thank-you to all contributors !!!)


## LameXP v4.11 [2015-04-05] ## {-}

* Upgraded build environment to Microsoft Visual Studio 2013 with Update-4
* Starting with this version, LameXP is based on the [*MUtilities*](http://sourceforge.net/p/mutilities/code/) library + massive code clean-up
* Added support for the [*DynamicAudioNormalizer*](https://github.com/lordmulder/DynamicAudioNormalizer) normalization filter
* Updated Qt runtime libraries to v4.8.7 snapshot-5 (2015-03-25), compiled with MSVC 12.0
* Updated MediaInfo to v0.7.72 (2015-01-07), compiled with ICL 15.0 and MSVC 12.0
* Updated SoX to v14.4.2-Final (2015-02-22), compiled with ICL 15.0 and MSVC 12.0
* Updated Opus libraries to v1.1.x and Opus-Tools v0.1.9 to latest Git Master (2015-03-26)
* Updated mpg123 decoder to v1.22.0 (2015-02-24), compiled with GCC 4.9.2
* Updated Vorbis encoder to OggEnc v2.87 (2014-07-03), using libvorbis v1.3.4 and aoTuV b6.03_2014
* Updated Vorbis decoder to OggDec v1.10.1 (2015-03-19), using libVorbis v1.3.5
* Updated FLAC encoder/decoder to v1.3.1 (2014-11-26), compiled with ICL 15.0 and MSVC 12.0
* Updated GnuPG to v1.4.18 (2014-06-30), compiled with GCC 4.9.1
* Updated QAAC add-in to the latest to QAAC v2.44, including a [fix](https://github.com/nu774/qaac/commit/ad1e0ea9daed076531e96cfa3b82f290ba9eeb20) for the ``--artwork`` option
* Fixed potential crash in Cue Sheet importer (occurred when *all* input files were missing)
* Fixed a severe performance bottleneck, especially with a large number of parallel instances
* Fixed a very rare problem that, occasionally, prevented the TEMP folder from being removed
* The limit for the maximum number of parallel instances has been increased to 32
* Experimental support for Windows 10 Technical Preview
* Updated language files (big thank-you to all contributors !!!)


## LameXP v4.10 [2014-06-23] ## {-}

* Upgraded build environment to Microsoft Visual Studio 2013 with Update-2
* Updated Qt runtime libraries to v4.8.6 (2014-04-25), compiled with MSVC 12.0
* Updated Opus libraries to v1.1.x and Opus-Tools v0.1.8 to latest Git Master (2014-04-13)
* Updated MediaInfo to v0.7.69 (2014-04-26), compiled with ICL 14.0 and MSVC 12.0
* Updated mpg123 decoder to v1.19.0 (2014-03-08), compiled with GCC 4.8.2
* Fixed a bug that could cause the cover artwork to be lost under certain circumstances
* Fixed "overwrite existing file" mode to NOT delete the input file
* Some more tweaks to the LAME algorithm quality selector
* Added command-line options to adjust the LameXP font size (see [Manual](Manual.html#gui-adjustment-options) for details)
* Various bugfixes and code improvements


## LameXP v4.09 [2014-01-26] ## {-}

* Upgraded build environment to Microsoft Visual Studio 2013 RTM
* Complete overhaul of the file analyzer, resulting in up to 2.5x faster file import speed
* Reworked the application initialization code, resulting in notably faster startup speed
* Added encoding support for Monkey's Audio (APE) format, including APEv2 tagging support
* Improved file analyzer to retain the original ordering of files imported from a playlist
* Improved internal encoder API, so each encoder can define its own configuration options
* Improved splash screen and working banner, using "sheet of glass" effect on supported OS
* Improved dropbox widget, including proper multi-monitor support
* Updated Opus encoder/decoder libraries to v1.1 and Opus-Tools to v0.1.8 (2013-12-05)
* Updated Monkey's Audio binary to v4.12 (2013-06-26)
* Updated mpg123 decoder to v1.16.0 (2013-10-06), compiled with GCC 4.8.1
* Updated WavPack decoder to v4.70.0 (2013-10-19), compiled with ICL 14.0 and MSVC 12.0
* Updated MediaInfo to v0.7.67 (2014-01-10), compiled with ICL 14.0 and MSVC 12.0
* Updated GNU Wget binary to v1.14.0 (2012-08-05), compiled with GCC 4.8.1
* Updated GnuPG to v1.4.16 (2013-12-13), compiled with GCC 4.8.1
* Updated the QAAC add-in for LameXP to QAAC v2.33 (2014-01-14), compiled with MSVC 12.0
* Updated language files (big thank-you to all contributors !!!)
* Fixed a resource (file descriptor) leak in "static" builds, didn't cause much harm though
* Various bugfixes and code improvements


## LameXP v4.08 [2013-09-04] ## {-}

* Upgraded build environment to Microsoft Visual Studio 2012 with Update-3
* Encoder settings (RC mode + bitrate/quality) are now stored separately for each encoder
* Updated Qt runtime libraries to v4.8.5 (2013-05-31), compiled with MSVC 11.0
* Updated FLAC encoder/decoder to v1.3.0 (2013-05-27), compiled with ICL 13.0
* Updated Opus encoder/decoder libraries to v1.1-beta and Opus-Tools to v0.1.6 (2013-07-22)
* Updated MediaInfo to v0.7.64 (2013-07-05), compiled with ICL 13.1 and MSVC 10.0
* Updated GnuPG to v1.4.14 (2013-07-25), compiled with GCC 4.8.1
* Updated GNU Wget binary to v1.13.4 (2011-09-17)
* Updated language files (big thank-you to all contributors !!!)
* Fixed a potential deadlock during startup when %TMP% points to an invalid folder
* Fixed a superfluous "beep" sound that appeared on application startup
* Fixed the Ogg Vorbis quality modes "-1" and "-2" (those were clipped to "0" before)
* Fixed a bug that could cause the output directory to be reset mistakenly
* Implemented "natural order" string comparison/sorting, using strnatcmp() by Martin Pool


## LameXP v4.07 [2013-04-28] ## {-}

* Upgraded build environment to Microsoft Visual Studio 2012 with Update-2
* Minimum supported platform now is Windows XP with [Service Pack 3](http://www.microsoft.com/en-us/download/details.aspx?id=24)
* Added option to select the "overwrite mode" to advanced options tab
* Added option to filter the log entries on the "processing" dialog (see context menu)
* Added "Up One Level" button to the output folder tab
* Added Opus decoder option to output always at the native sample rate of 48.000 Hz
* Updated Qt runtime libraries to v4.8.4 (2012-11-29), compiled with MSVC 11.0
* Updated Opus encoder/decoder libraries to v1.1.x and Opus-Tools to v0.1.6 (2013-04-23)
* Updated Valdec decoder (2013-04-07), based on AC3Filter Tools v1.0a
* Updated mpg123 decoder to v1.15.3 (2013-04-03), compiled with GCC 4.8.0
* Updated MediaInfo to v0.7.62 (2013-02-22), compiled with ICL 12.1.7 and MSVC 10.0
* Updated Monkey's Audio binary to v4.11 (2013-01-20)
* Updated SoX to v14.4.1 (2012-02-09), compiled with ICL 13.0 and MSVC 10.0
* Updated GnuPG to v1.4.13, compiled with GCC 4.7.2
* Updated language files (big thank-you to all contributors !!!)
* Fixed handling of certain characters when passing meta tags on the command-line
* Fixed handling of certain characters when renaming output files
* Fixed Keccak library to not crash on systems without SSE/SSE2 support
* Fixed LAME algorithm quality selector better match the [LAME documentation](http://lame.cvs.sourceforge.net/viewvc/lame/lame/doc/html/detailed.html#q)


## LameXP v4.06 [2012-11-04] ## {-}

* Updated Opus encoder/decoder libraries to v1.0.1 and Opus-Tools to v0.1.5 (2012-09-22)
* Updated mpg123 decoder to v1.14.4+ (2012-09-24), compiled with GCC 4.7.1
* Updated ALAC decoder to refalac v0.56 (2012-10-24), based on reference implementation by Apple
* Updated Qt runtime libraries to v4.8.3 (2012-09-13), compiled with MSVC 10.0
* Updated MediaInfo to v0.7.61+ (2012-10-28), compiled with ICL 12.1.7 and MSVC 10.0
* Updated language files (big thank-you to all contributors !!!)
* Fixed a bug with the "Store temporary files in your system's default TEMP director" checkbox
* Fixed a buffer overflow in FAAD2 decoder which could cause crashes with very long file names
* Fixed a regression in Qt v4.8.3 that broke Drag&Drop support ([details #1](https://bugreports.qt-project.org/browse/QTBUG-27265)) ([details #2](https://codereview.qt-project.org/35297))
* Reworked the "About..." dialog – now using a custom dialog instead of message boxes


## LameXP v4.05 [2012-09-03] ## {-}

* Added support for Opus Audio Codec, based on Opus-Tools v0.1.4 (2012-08-16) by Xiph.org/Mozilla
* Added Swedish translation, thanks to Åke Engelbrektson <eson57@gmail.com>
* Updated Qt runtime libraries to v4.8.2 (2012-05-22), compiled with MSVC 10.0
* Updated mpg123 decoder to v1.14.4 (2012-07-26), compiled with GCC 4.6.1
* Updated MediaInfo to v0.7.59 (2012-08-08), compiled with ICL 12.1.7 and MSVC 10.0
* Updated optional add-ins for QAAC encoder and FHG AAC encoder (see [Manual](Manual.html#qaac-apple-aac-encoder) for details)
* Updated DCA Enc to v2 (2012-04-19), compiled with ICL 12.1.7 and MSVC 10.0
* Updated language files (big thank-you to all contributors !!!)
* Implemented multi-threading in file analyzer for faster file import (about 2.5x to 6.0x faster!)
* Implemented multi-threading in initialization code for faster application startup
* Fixed a potential crash (stack overflow) when adding a huge number of files
* Fixed a problem with Cue Sheet import and files that contain trailing dots in their name
* Workaround for a bug (feature?) of Qt's command-line parser that screwed up some arguments


## LameXP v4.04 [2012-04-26] ## {-}

* Added support for the QAAC Encoder, requires QuickTime v7.7.1 or newer (see [Manual](Manual.html#qaac-apple-aac-encoder) for details)
* Added Chinese and Taiwanese translations, thanks to 456Vv <123@456vv.com>
* Added experimental support for DCA Enc, created by Alexander E. Patrakov <patrakov@gmail.com>
* Added CSV export/import for Meta tags (available from the context-menu on the "Source Files" tab)
* Added a button to modify the current output folder path in an edit box
* Updated Qt runtime libraries to v4.8.1 (2012-03-14), compiled with MSVC 10.0
* Updated LAME encoder to v3.99.5 Final (2012-02-28), compiled with ICL 12.1.7 and MSVC 10.0 ([details](http://lame.cvs.sourceforge.net/viewvc/lame/lame/doc/html/history.html?revision=1.139))
* Updated MediaInfo to v0.7.56 (2012-04-08), compiled with ICL 12.1.7 and MSVC 10.0
* Updated SoX to to v14.4.0 (2012-03-04), compiled with ICL 12.1.7 and MSVC 10.0
* Updated mpg123 decoder to v1.13.6 (2011-03-11), compiled with GCC 4.6.1
* Updated Monkey's Audio binary to v4.11 (2011-04-20)
* Updated Musepack decoder to revision 475 (2011-08-10), compiled with ICL 12.1.6 and MSVC 10.0
* Updated GnuPG to v1.4.12, compiled with GCC 4.6.1
* Updated language files (big thank-you to all contributors !!!)
* Implemented coalescing of update signals to reduce the CPU usage of the LameXP process ([details](http://forum.doom9.org/showpost.php?p=1539631&postcount=507))
* Run more than four instances in parallel on systems with more than four CPU cores
* Improved handling of different character encodings for Playlist and Cue Sheet import
* Tweaked directory outline on "output folder" tab for improved performance (hopefully)
* Improved LameXP inter-process communication by adding queue support
* Workaround for a bug that causes MediaInfo to not detect the duration of Wave files (64-Bit only)
* Prevent LameXP from blocking a system shutdown (encoding process is aborted, if necessary)
* Improved internal handling of MediaInfo output, including extraction of cover art
* Fixed a very rare "live-lock" situation in early initialization code


## LameXP v4.03 [2011-11-12] ## {-}


* Added an option to rename the output files (based on an user-defined naming pattern)
* Added an option to enforce Stereo Downmix for Multi-Channel sources
* Added "built-in" WMA decoder (see [*this*](http://forum.doom9.org/showthread.php?t=140273) thread for details) and removed all remnants of "old" decoder
* Added optional support for the FHG AAC Encoder included with Winamp 5.62
* Added a menu for bookmarking "favorite" output folders to the "output folder" tab
* Added an option to hibernate the computer (aka "Suspend-to-Disk") instead of shutting it down
* Added Polish translation, thanks to Sir Daniel K <sir.daniel.k@gmail.com>
* Added channel equalization options to the normalization filter (also fixes multi-channel processing)
* Added indicators for current CPU usage, RAM usage and free diskspace to the processing window
* Updated Qt runtime libraries to v4.8.0 RC-1 (2011-10-13), compiled with MSVC 10.0
* Updated LAME encoder to v3.99.1 Final (2011-11-05), compiled with ICL 12.1.6 and MSVC 10.0 ([details](http://lame.cvs.sourceforge.net/viewvc/lame/lame/doc/html/history.html?revision=1.133))
* Updated mpg123 decoder to v1.13.4 (2011-09-07), compiled with GCC 4.6.1
* Updated MediaInfo to v0.7.51 (2011-11-11), compiled with ICL 12.1.6 and MSVC 10.0
* Updated language files (big thank-you to all contributors !!!)
* Improved "downmix" filter by using explicit channel mappings for each number of input channels
* Fixed a potential bug in CPU type detection that might have caused the wrong binary to be used
* Fixed Cue Sheet import for tracks with certain characters in the title
* Fixed a bug with "Prepend relative source file path to output file" under certain conditions
* Workaround for malicious "anti-virus" programs that prevent innocent applications from functioning
* Enabled "Aero Glass" theme in installer and web-update program (Vista and Windows 7 only)
* Restored Windows 2000 support with Visual Studio 2010 builds (this is experimental!)
* The "Open File(s)" and "Open Folder" dialogs will now remember the most recent directory
* Miscellaneous bugfixes


## LameXP v4.02 [2011-06-14] ## {-}

* Upgraded build environment to Microsoft Visual Studio 2010
* Dropping support for Windows 2000 and Windows XP RTM. Windows XP needs (at least) Service-Pack 2 now!
* Added Cue Sheet import wizard, which allows splitting and importing tracks from Cue Sheet images
* Added ATSC A/52 (AC-3) encoding support, based on Aften encoder v0.0.8+ (Git Master)
* Added Avisynth input (audio only!) using 'avs2wav' tool, partly based on code by Jory Stone
* Added a method to use custom tools instead of the "built-in" ones (see [Manual](Manual.html) for details)
* Added an option to copy all meta information of a single file over to the "meta information" tab
* Added two new command-line switches: "--add-folder <path>" and "--add-recursive <path>"
* Added one new translation: Korean
* Updated Qt runtime libraries to v4.7.3
* Updated LAME encoder to v3.99.1.0 (2011-04-15), compiled with ICL 12.0.3 and MSVC 10.0 ([details](http://lame.cvs.sourceforge.net/viewvc/lame/lame/doc/html/history.html?revision=1.127))
* Updated Vorbis encoder to v2.87 using aoTuV Beta-6.03 (2011-05-04), compiled with ICL 11.1 and MSVC 9.0
* Updated mpg123 decoder to v1.13.3 (2011-04-21), compiled with GCC 4.6.0
* Updated MediaInfo to v0.7.45 Beta (2011-05-02), compiled with ICL 12.0.3 and MSVC 10.0
* Updated language files (big thank-you to all contributors !!!)
* Fixed placement of the Dropbox when the Taskbar is located on the top or on the left side
* Improved playlist generation: Generate M3U (Latin-1) or M3U8 (UTF-8) playlist file as required
* Only show the most recent 50 items in the "processing" window (for better performance)
* Miscellaneous bugfixes


## LameXP v4.01 [2011-04-04] ## {-}

* Added an option to manually specify the number of parallel instances
* Added an option to select a user-defined TEMP directory
* Added an option to shutdown the computer as soon as all files are completed
* Added an option to add directories recursively
* Added support for embedding cover artwork (currently works with LAME, FLAC and Nero AAC only)
* Updated Qt runtime libraries to v4.7.2
* Updated LAME encoder to v3.99.0.16 (2011-04-04), compiled with ICL 12.0.2
* Updated Vorbis encoder to v2.87 using aoTuV Beta-6.02 (2011-02-28), compiled with ICL 11.1 and MSVC 9.0
* Updated TTA decoder multiplatform library to v2.1 (2011-03-11), compiled with MSVC 9.0
* Updated SoX to v14.3.2 (2010-02-27), compiled with ICL 12.0.2
* Updated MediaInfo to v0.7.43 (2011-03-20), compiled with ICL 12.0.2 and MSVC 9.0
* Updated language files (big thank-you to all contributors !!!)
* Fixed a problem with the LAME encoder that could cause glitches in the encoded file (VBR mode only)
* Fixed a problem with the LAME encoder that could cause very slow encoding speed
* Fixed a bug that caused AAC encoding to fail in CBR mode (the "-2pass" parameter was set wrongly)
* A warning message will be emitted, if diskspace drops below a critical limit while processing


## LameXP v4.00 [2011-02-21] ## {-}

* Complete re-write of LameXP in the C++ programming language
* Switched IDE from Delphi 7.0 to Visual Studio 2008 + Qt Framework v4.7.1 (GNU Toolchain not yet)
* Added cross-plattfrom support - only Windows and Wine for now, native Linux version planned
* Added full Unicode support for file names, meta tags and translations (no more Codepage headaches!)
* Added support for Qt Linguist tool, which makes creating/updating translations much easier
* Added support for multiple user interface styles, including "Plastique" and "Cleanlooks" themes
* Added support for user-defined encoder parameters (please use with care!)
* Added support for a true "portable" mode, which will store the configuration in the program folder
* Added resampling filter for all encoders, based on SoX
* Added simple tone adjustment filter, based on SoX
* Added an option to prepend the relative source file path to the output file path
* Updated all command-line tools to support Unicode file names, mostly required custom patches
* Updated LAME encoder to v3.99.0.11 (2011-02-11), compiled with ICL 11.1.065
* Updated Vorbis encoder to v2.87 using libvorbis v1.3.2 (2010-11-06), compiled with ICL 11.1 and MSVC 9.0
* Updated mpg123 decoder to v1.13.2 (2011-02-19), compiled with GCC 4.5.2
* Updated MediaInfo to v0.7.41 (2011-01-24), compiled with ICL 11.1.065
* Updated SoX to v14.3.1 (2010-04-11), compiled with MSVC 9.0
* Updated GnuPG to v1.4.11, compiled with GCC 4.5.2
* Updated language files (big thank-you to all contributors !!!)
* Removed TAK support for now, as their CloseSource(!) tools don't support Unicode file names yet
* Removed Volumax tool, as we are using SoX for normalization from now on
* Countless minor fixes and improvements (hopefully not too many regressions ^^)



# LameXP v3.xx History ##

## LameXP v3.19 [2010-07-12] ## {-}

* Updated MediaInfo to v0.7.34 (2010-07-09), compiled with ICL 11.1.065
* Updated mpg123 decoder to v1.12.3 (2010-07-11), compiled with GCC 4.6.0
* Updated language files (big thank-you to all contributors !!!)
* Fixed decoding of certain invalid WavPack files


## LameXP v3.18 [2010-05-08] ## {-}

* Added an Unicode-safe "Open" dialog: File names are converted to "short" names if required
* Fixed mpg123 decoder to work on Windows 2000 (reported by Tim Womack)
* Updated LAME encoder to v3.98.4 (2010-03-23), compiled with ICL 11.1.054
* Updated MediaInfo to v0.7.32 (2010-05-02), compiled with ICL 11.1.065
* Updated mpg123 decoder to v1.12.1 (2010-03-31), compiled with GCC 4.4.4
* Updated Ogg Vorbis decoder to v1.9.7 (2010-03-29), compiled with MSVC 9.0
* Updated language files (big thank-you to all contributors !!!)


## LameXP v3.17 [2010-02-21] ## {-}

* Updated TAK decoder to v2.0.0 (2010-01-07)
* Updated ALAC decoder to v0.2.0 (2009-09-05)
* Updated MediaInfo to v0.7.28 (2010-02-19), compiled with ICL 11.1.054
* Fixed "No Disk" error message box that could appear under certain circumstances
* Fixed "...is not responding" error message box that could appear during startup
* Various minor fixes and improvements


## LameXP v3.16 [2010-01-26] ## {-}

* Added support for Nero AAC encoder v1.5.3.0 (2009-12-29)
* Disable DPI warning on Vista and later, as they handle DPI != 96 much better than WinXP
* Updated WavPack decoder to v4.60.1 (2009-11-29)
* Updated MediaInfo to v0.7.27 (2010-01-04), compiled with ICL 11.1.054
* Updated GnuPG to v1.4.10b (2009-09-03), compiled with GCC 4.2.1


## LameXP v3.15 [2009-12-24] ## {-}

* Added support for Nero AAC encoder v1.5.1.0 (2009-12-17)
* Updated mpg123 decoder to v1.10.0 (2009-12-05)
* Updated MediaInfo to v0.7.26 (2009-12-18), compiled with ICL 11.1.051
* Updated AC3Filter Tools to v0.31b (2009-10-01), compiled with ICL 11.1.051


## LameXP v3.14 [2009-12-01] ## {-}

* Added Suspend and Resume buttons to the processing window
* Added another language: Castilian Spanish (Spanish from north/central Spain)
* Updated mpg123 decoder to v1.9.2 (2009-11-20)
* Updated MediaInfo to v0.7.25 (2009-11-13), compiled with ICL 11.1.046
* Updated AC3Filter Tools to v0.31b (2009-10-01), compiled with ICL 11.1.046
* Updated language files (big thank-you to all contributors !!!)
* Updated JEDI-VCL from v3.38 to v3.39 (2009-11-05)
* Various minor fixes and improvements


## LameXP v3.13 [2009-10-21] ## {-}

* Updated LAME encoder to v3.98.2 (2009-09-26), compiled with ICL 11.1.046
* Updated FLAC encoder to v1.2.1b (2009-10-01), compiled with ICL 11.1.046
* Updated MediaInfo to v0.7.23 (2009-10-16), using statically linked build (MSVC 9.0)
* Updated AC3Filter Tools to v0.31b (2009-10-01)
* Updated TAK decoder to v1.1.2 (2009-07-27)
* Updated mpg123 decoder to v1.9.1 (2009-10-09)
* Updated language files (big thank-you to all contributors !!!)
* Updated the Splash screen and modified the sound that plays on very first launch
* Updated JEDI-VCL from v3.34 to v3.38 (2009-08-27)
* Updated GnuPG to v1.4.10 (2009-09-02)


## LameXP v3.12 [2009-09-19] ## {-}

* Added support for FLAC (Free Lossless Audio Codec) output
* Added progress display for individual files (for the "encoding" step only)
* Added a SSE2 (Pentium 4) build of the Ogg Vorbis encoder that will be used if supported by the CPU
* Added options to override the Nero AAC profile (be aware: it's not recommended to do that!)
* Added an option to analyze media files (powered by MediaInfo™)
* Added experimental support for Windows 7 taskbar progress indicator and overlay icons
* Updated LAME encoder to v3.98.2 (2009-09-05), compiled with ICL 11.0
* Updated MediaInfo to v0.7.21 (2009-09-04), using statically linked build
* Updated mpg123 decoder to v1.9.0 (2009-08-14)
* Updated Speex decoder to v1.2 RC-1 (2009-07-04)
* Updated AC3Filter Tools to v0.3b (2009-09-19)
* Updated Auto-Update tool, from now on only signed updates will be accepted (using GnuPG)
* Fixed a number of minor glitches


## LameXP v3.11 [2009-06-22] ## {-}

* Added options to sort the source files (by title, by filename or by track number)
* Updated language files (big thank-you to all contributors !!!)
* Updated mpg123 decoder to v1.8.1 (2009-06-14)
* Updated FLAC decoder, now using the ICL 9.1 build of FLAC v1.2.1b
* Updated MediaInfo to v0.7.17 (statically linked)
* Updated the "Normalization" filter to v0.41 (2009-06-16)
* Fixed a few minor issues in meta tag processing


## LameXP v3.10 [2009-06-11] ## {-}

* Added a NSIS-based installer (will be released in addition to the ZIP package)
* Added support for the TAK lossless audio format
* Added two new languages: Serbian (Latin) and Ukrainian
* Updated language files (big thank-you to all contributors !!!)
* Updated MediaInfo to a custom build of v0.7.16 that is statically linked (and removed the DLL)
* Updated mpg123 decoder to v1.8.0 RC-3 (2009-06-03)
* Updated Musepack decoder to v1.0.0 (2009-04-02) and fixed Musepack VS8 support
* Updated Monkey's Audio decoder to v4.06 (2009-03-17)
* Updated the "Normalization" filter to allow multiple instances running in parallel
* Updated Auto-Update tool
* Fixed a few minor issues and refactored the code


## LameXP v3.09 [2009-06-01] ## {-}

* Added support for detecting the file type via MediaInfo instead of guessing the type from file extension
* Updated mpg123 decoder to v1.7.3 (2009-04-27)
* Updated FAAD decoder to v2.7 (2009-05-13)
* Updated MediaInfo to v0.7.16.0 (2009-05-20)
* Fixed detection of the WMA decoder under certain circumstances (e.g. Windows 7)


## LameXP v3.08 [2009-03-05] ## {-}

* Updated Ogg Vorbis encoder to v2.85, libvorbis v1.2.1 RC2, aoTuV b5.7 (2009-03-04)
* Updated mpg123 decoder to v1.6.4 (2009-01-10)
* Updated MediaInfo to v0.7.11.0 (2009-02-13)


## LameXP v3.07 [2008-12-24] ## {-}

* Added an option to disable multi-threading on multi-core machines
* Updated Ogg Vorbis encoder to v2.85, libvorbis v1.2.1 RC2, aoTuV b5.61 (2008-12-24)
* Updated mpg123 decoder to v1.6.3 (2008-12-20)
* Updated MediaInfo to v0.7.8.0 (2008-12-10)
* Updated language files (big thank-you to all contributors !!!)


## LameXP v3.06 [2008-10-26] ## {-}

* Added a custom build of the mpg123 decoder v1.5.1
* Added two more languages: Romanian and Polish
* Added support for the ALAC audio format
* Updated MediaInfo to v0.7.7.7 (2008-10-17)
* Updated AC3 Filter Tools to v0.2a (2008-06-30)
* Updated language files (big thank-you to all contributors !!!)
* Fixed and improved "Normalization" filter
* Fixed a few minor bugs


## LameXP v3.05 [2008-10-11] ## {-}

* Added support for Nero AAC encoder v1.3.3.0
* Added option to add an entire directory or an entire directory-tree
* Added new languages: Russian, Nederlands, Greek and Hungarian
* Added Dropbox for improved Drag&Drop support
* Updated language files (big thank-you to all contributors !!!)
* Updated LAME encoder to v3.98.2 Final (2008-09-24)
* Updated MediaInfo to v0.7.7.6 (2008-09-12)


## LameXP v3.04 [2008-09-26] ## {-}

* Added support for reading Meta Data from source files (using MediaInfo)
* Added support for languages: English, German, French, Spanish, Italian, Japanese, Chinese (Simplified) and Taiwanese
* Added support for WMA, Shorten and TTA files (input only)
* Added support for various playlist formats (M3U, PLS, ASX, CueSheet)
* Added an option to permanently disable the Shell Intgegration (Explorer Conext Menus)
* Added an option to disable the periodic Update Reminder
* Added an option to shutdown the computer automatically as soon as all files are completed
* Added code to minimize the LameXP window into the taskbar notification area
* Added balloon tooltip to inform the user about "hidden" options
* Updated Ogg Vorbis encoder to v2.85, libvorbis v1.2.1 RC2, aoTuV b5.6 (2008-09-05)
* Improved code to handle child processes and capture the console output


## LameXP v3.03 [2008-08-12] ## {-}

* Added generic support for pre-processing filters
* Added "Normalization" filter, based on Volumax by John33
* Improved code to add/remove context menus in Window Explorer
* Improved code to handle multiple instances of LameXP


## LameXP v3.02 [2008-08-06] ## {-}

* Added support for new input format: MPEG Audio Layer-2 (mp2)
* Added option to choose a custom TEMP folder

## LameXP v3.01 [2008-08-01] ## {-}

* Added an option to disable all sounds in LameXP
* Added warning message for bitrates that violate the current bitrate restriction
* Fixed bitrate restrictions for LAME encoder (strictly enforce bitrate restrictions using "-F" parameter)
* Fixed file associations code (set file associations only for the current user)
* Updated load/save configuration code (store settings in an INI file instead of the registry)


## LameXP v3.00 [2008-07-04] ## {-}

* Added support for Nero's AAC Encoder (not included, available as free download from Nero website)
* Added support for more input formats: Wave, MP3, Ogg Vorbis, AAC/MP4, FLAC, Speex, WavPack, Musepack, Monkey's Audio
* Added support for uncompressed Wave output
* Added support for Multi-Threading (use multiple instances for batch processing)
* Added shell integration for Windows Explorer (Context-Menus and "Send To" folder)
* Added commandline support: LameXP.exe -add <File 1> [<File 2> ... <File N>]
* Added Auto-Update utility to periodically check for new updates
* Updated LAME encoder to v3.98 Final (2008-07-04)
* Updated Ogg Vorbis encoder to v2.85, aoTuV Beta-5.5 (2008-03-31)



# LameXP v2.xx History ##

## LameXP v2.03 [2007-08-17] ## {-}

* Updated LAME encoder to v3.98 to Beta-5 (2007-08-13)
* Updated Ogg Vorbis encoder to v2.84 aoTuV Beta-5 (2007-08-17)
* Fixed a bug with 'title' meta tags
* Fixed a few typos


## LameXP v2.00 [2007-02-19] ## {-}

* Added Ogg Vorbis encoder
* Updated LAME encoder to latest builds
* Improved progress display (parsing encoder progress from console output now)
* Improved ID3-Tag support (now supports "title" and "track" fields)
* Added feature to automatically generate playlists (.m3u)
* Many bug-fixes and GUI improvements



# LameXP v1.xx History ##

## LameXP v1.00 [2004-12-10] ## {-}

* Does not compute&hellip;



&nbsp;
&nbsp;
**EOF**

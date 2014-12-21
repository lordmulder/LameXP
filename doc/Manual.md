% LameXP - User Manual

Introduction
============

![LameXP](http://lamexp.sourceforge.net/lamexp.png)

LameXP is a graphical user-interface (front-end) for various of audio encoders: It allows you convert your audio files from one audio format to another one in the most simple way. Despite its name, LameXP is NOT only a front-end for the LAME encoder, but supports a wide range of output formats, including MP3, Ogg Vorbis, AAC/MP4, FLAC, AC-3 and Wave Audio. The number of supported input formats is even bigger! Furthermore LameXP does NOT only run on Windows XP, but also on Windows Vista, Windows 7 and many other operating systems.

As all the encoders and decoders used by LameXP are already "built-in" (with one exception), you do NOT need to install any additional software, such as "Codecs", "Filters" or "Plug-ins", on your computer. Everything works "out of the box"! You can even use LameXP as a "portable" application, e.g. run it from your USB stick. Moreover LameXP was designed for batch processing. This means that you can convert a huge number of audio files, e.g. a complete album or even your entire music collection, in a single step. And, as LameXP is able to process several audio files in parallel, it takes full advantage of modern multi-core processors! However LameXP is NOT only optimized for speed, it also provides excellent sound quality by using the most sophisticated encoders available and by giving the user unrestricted control over all encoding parameters. In addition to that, LameXP provides full support for metadata, including cover art. So when converting your audio files, LameXP will retain existing meta tags. But there also is an easy-to-use editor for adding or modifying metadata. LameXP supports Unicode for both, meta tags and filenames, so there won't be any problems with "foreign" characters. And, thanks to our translators, the user-interface of LameXP is available in multiple languages. Last but not least, LameXP supports a number of post-processing filters, including sample rate conversion, normalization (gain), tone adjustment and downmixing of multi-channel sources.


Platform Support
----------------

**Tier #1:** LameXP is currently being developed on the following platforms:

* Microsoft Windows 8.1, 32-Bit and 64-Bit editions
* Microsoft Windows 7 with Service Pack 1, 32-Bit and 64-Bit editions
* Microsoft Windows XP with Service Pack 3 (see remarks below!)


**Tier #2:** The following platforms should work too, but aren't tested extensively:

* Microsoft Windows 10, 32-Bit and 64-Bit editions
* Microsoft Windows 8.0, 32-Bit and 64-Bit editions
* Microsoft Windows Vista with Service Pack 2, 32-Bit and 64-Bit editions
* Microsoft Windows XP x64 Edition with Service Pack 2
* Microsoft Windows Server 2008 with Service Pack 2
* Microsoft Windows Server 2008 R2 with Service Pack 1
* GNU/Linux (e.g. Ubuntu 12.04) using Wine v1.4+, native Linux version planned

**Legacy:** The following platforms are NOT actively supported any longer:

* Microsoft Windows 2000
* Microsoft Windows NT 4.0
* Microsoft Windows Millennium Edition
* Microsoft Windows 98
* Microsoft Windows 95

*Remarks:* Windows XP has reached "end of life" on April 8th, 2014. This means that Microsoft has stopped all support for Windows XP, i.e. *no* updates or bugfixes are made available to regular Windows XP uses since that date, *not* even security fixes! Thus, all the security vulnerabilities that have been discovered *after* this deadline - and all the security vulnerabilities that will be discovered in the future - are going remain *unfixed* forever! Consequently, using Windows XP has become a severe security risk, and the situation is only going to get worse. While LameXP will continue to support Windows XP (note that Service Pack 3 is required!) for the foreseeable future, we *highly* recommend everybody to update to a less antiquated system now. Windows XP support will be discontinued in a future version, when most users have migrated to a contemporary system.


Supported Output Formats (Encoders)
-----------------------------------

Currently the following output formats are supported by LameXP:

* Opus Audio Codec, using the Opus-Tools by Xiph.org/Mozilla [built-in]
* Ogg Vorbis, using the OggEnc2/libvorbis encoder with aoTuV [built-in]
* MPEG Audio-Layer III (MP3), using the LAME encoder [built-in]
* Advanced Audio Coding (AAC), using Nero AAC encoder [separate download!]
* ATSC A/52 (aka "AC-3"), using the Aften encoder [built-in]
* DCA, using the DCA Enc encoder (still experimental) [built-in]
* Free Lossless Audio Codec (FLAC) [built-in]
* Uncompressed PCM / Waveform Audio File (WAV/RIFF)


Supported Input Formats (Decoders)
----------------------------------

Currently the following input formats are supported by LameXP:

* AC-3 (ATSC A/52), using Valib decoder [built-in]
* Advanced Audio Coding (AAC), using FAAD decoder [built-in]
* Apple Lossless (ALAC)
* Apple/SGI AIFF
* Avisynth, audio only [requires Avisynth 2.5.x to be installed]
* Digital Theater System, using Valib decoder [built-in]
* Free Lossless Audio Codec (FLAC)
* Microsoft ADPCM
* Monkey's Audio (APE)
* MPEG Audio-Layer I (MP1), using mpg123 decoder [built-in]
* MPEG Audio-Layer II (MP2), using mpg123 decoder [built-in]
* MPEG Audio-Layer III (MP3), using mpg123 decoder [built-in]
* Musepack
* Opus Audio Codec
* Shorten
* Speex
* Sun/NeXT Au
* The True Audio (TTA)
* Uncompressed PCM / Waveform Audio File (WAV/RIFF)
* WavPack Hybrid Lossless Audio
* Windows Media Audio (WMA), using wma2wav [built-in]

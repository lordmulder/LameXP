///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2011 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>

////////////////////////////////////////////////////////////
// CPU FLags
////////////////////////////////////////////////////////////

/* CPU_TYPE_<ARCH>_<TYPE> */
#define CPU_TYPE_X86_GEN 0x00000001UL //x86, generic
#define CPU_TYPE_X86_SSE 0x00000002UL //x86, with SSE and SSE2 support - Intel only!
#define CPU_TYPE_X64_GEN 0x00000004UL //x64, generic
#define CPU_TYPE_X64_SSE 0x00000008UL //x64, with SSE and SSE2 support - Intel only!

/* combined CPU types */
#define CPU_TYPE_X86_ALL (CPU_TYPE_X86_GEN|CPU_TYPE_X86_SSE) //all x86 (ignore SSE/SSE2 support)
#define CPU_TYPE_X64_ALL (CPU_TYPE_X64_GEN|CPU_TYPE_X64_SSE) //all x64 (ignore SSE/SSE2 support)
#define CPU_TYPE_ALL_GEN (CPU_TYPE_X86_GEN|CPU_TYPE_X64_GEN) //all generic           (ignore x86/x64)
#define CPU_TYPE_ALL_SSE (CPU_TYPE_X86_SSE|CPU_TYPE_X64_SSE) //all with SSE and SSE2 (ignore x86/x64)
#define CPU_TYPE_ALL_ALL (CPU_TYPE_X86_ALL|CPU_TYPE_X64_ALL) //use always, no exceptions

////////////////////////////////////////////////////////////
// TOOLS
////////////////////////////////////////////////////////////

static const struct
{
	char *pcHash;
	unsigned int uiCpuType;
	char *pcName;
	unsigned int uiVersion;
}
g_lamexp_tools[] =
{
	{"1e4c983fe00ccc6236ee1c3f76250f64139cf86a6f4aa68f36929907fe45cf37b00862d8", CPU_TYPE_X86_GEN, "aften.i386.exe", 8},
	{"7c843f1eaf61823285237911493d8d20da11149853d4bb7953a3279196f6dbbd310c61e3", CPU_TYPE_X86_SSE, "aften.sse2.exe", 8},
	{"29da0d3e810bc3e8d2cddb3db452325eefca0d0c1fff1379fa17806ad447752be1b88e2f", CPU_TYPE_X64_ALL, "aften.x64.exe",  8},
	{"1cca303fabd889a18fc01c32a7fd861194cfcac60ba63740ea2d7c55d049dbf8f59259fa", CPU_TYPE_ALL_ALL, "alac.exe", 20},
	{"6d22d4bbd7ce2162e38f70ac9187bc84eb28233b36ee6c0492d0a6195318782d7f05c444", CPU_TYPE_ALL_ALL, "avs2wav.exe", 13},
	{"d94bea403cbd24a890d91f55a4e4fed47898b21b62de3ba1202da55dc0917187304171f6", CPU_TYPE_ALL_ALL, "dcaenc.exe", 20111222},
	{"e53a787d4a0319453f4fe48c3145f190fcce7ac4802e521db908771437f6250746116e6c", CPU_TYPE_ALL_ALL, "elevator.exe", UINT_MAX},
	{"9ae98a3fc779f69ee876a3b477fbc35a709ba5066823b2eb62eeb015057c38807e4be51f", CPU_TYPE_ALL_ALL, "faad.exe", 27},
	{"446054f9a7f705f1aadc9053ca7b8a86a775499ef159978954ebdea92de056c34f8841f7", CPU_TYPE_ALL_ALL, "flac.exe", 121},
	{"dd68d074c5e13a607580f3a24595c0f3882e37211d2ca628f46e6df20fabcc832dad488a", CPU_TYPE_ALL_ALL, "gpgv.exe", 1411},
	{"b3fca757b3567dab75c042e62213c231de378ea0fdd7fe29b733417cd5d3d33558452f94", CPU_TYPE_ALL_ALL, "gpgv.gpg", UINT_MAX},
	{"d5b35689208aaaf2809c3fb501e8e74af410378dc5f1921df30f2525e2ee084e5a9f9595", CPU_TYPE_ALL_GEN, "lame.i386.exe", 3992},
	{"cfc59f7c299569355d8b4143a4f0524edf5b78dc61402c03be938dbabb5c2db5695feedd", CPU_TYPE_ALL_SSE, "lame.sse2.exe", 3992},
	{"67933924d68ce319795989648f29e7bd1abaac4ec09c26cbb0ff0d15a67a9df17e257933", CPU_TYPE_ALL_ALL, "mac.exe", 406},
	{"0eeaef330b43c3e24a7aeca5c2d3b60c9e2dc3baff89f8921b3d3687a8693f1fbd9f02a4", CPU_TYPE_X86_ALL, "mediainfo.i386.exe", 752},
	{"8a2b61a7f344bd145cb5ec0e0a35c8ba58b90a99ece91e462724143dd98b30db5a79bda7", CPU_TYPE_X64_ALL, "mediainfo.x64.exe",  752},
	{"a93ec86187025e66fb78026af35555bd3b4e30fe1a40e8d66f600cfd918f07f431f0b2f2", CPU_TYPE_ALL_ALL, "mpcdec.exe", 435},
	{"7fa1beb4161d603563089cadd601f68fb9f436f05d9477b6a604501b072f5a973dd45fbb", CPU_TYPE_ALL_ALL, "mpg123.exe", 1134},
	{"0c781805dda931c529bd16069215f616a7a4c5e5c2dfb6b75fe85d52b20511830693e528", CPU_TYPE_ALL_ALL, "oggdec.exe", UINT_MAX},
	{"0c019e13450dc664987e21f4e5489d182be7d6d0d81efbbaaf1c78693dfe3e38e0355b93", CPU_TYPE_X86_GEN, "oggenc2.i386.exe", 287603},
	{"693dd6f779df70a047c15c2c79350855db38d5b0cd7e529b6877b7c821cfe6addfdd50a4", CPU_TYPE_X86_SSE, "oggenc2.sse2.exe", 287603},
	{"32cb0b2182488e5e9278ba6b9fc9141214c7546eec67ee02fa895810b0e56900368695be", CPU_TYPE_X64_ALL, "oggenc2.x64.exe",  287603},
	{"58c2b8bcff8f27bfa8fab8172b80f5da731221d072c7dba4dd3a3d7d6423490a25dc6760", CPU_TYPE_ALL_ALL, "shorten.exe", 361},
	{"abdf9b20a8031a09d0abca9cb10c31c8418f72403b5d1350fd69bfa34041591aca3060ab", CPU_TYPE_ALL_ALL, "sox.exe", 1432},
	{"48e7f81c024cd17dac0eaeab253aad6b223e72dc80688f7576276b0563209514ff0bb9c8", CPU_TYPE_ALL_ALL, "speexdec.exe", 12},
	{"9b50cf64747d4afbad5d8d9b5a0a2d41c5a58256f47ebdbd8cc920e7e576085dfe1b14ff", CPU_TYPE_ALL_ALL, "tta.exe", 21},
	{"875871c942846f6ad163f9e4949bba2f4331bec678ca5aefe58c961b6825bd0d419a078b", CPU_TYPE_ALL_ALL, "valdec.exe", 31},
	{"e657331e281840878a37eb4fb357cb79f33d528ddbd5f9b2e2f7d2194bed4720e1af8eaf", CPU_TYPE_ALL_ALL, "wget.exe", 1114},
	{"8923cf65e181f8a99f28e9d1ea0d89ace02142241a7f76dae3d540ffd0790495af815644", CPU_TYPE_ALL_ALL, "wma2wav.exe", 20111001},
	{"bebc3eb0f7378a1f3d54fd75f1112d370f2f6a7fdd78067e6c40a96ce81a6841a1ad8283", CPU_TYPE_ALL_ALL, "wupdate.exe", 20111016},
	{"6b053b37d47a9c8659ebf2de43ad19dcba17b9cd868b26974b9cc8c27b6167e8bf07a5a2", CPU_TYPE_ALL_ALL, "wvunpack.exe", 4601},
	{NULL, NULL, NULL, NULL}
};

///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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
	{"fff2a8f9116c6cff9b8ccf18a486c827df6be623b715899ae882f514c46e112bdbf510a2", CPU_TYPE_X86_GEN, "aften.i386.exe", 8},
	{"9b52bd2efcb59aef1f65e9e11e6b51b171705e155af7c624562842f3c35429d41af9da30", CPU_TYPE_X86_SSE, "aften.sse2.exe", 8},
	{"73a9ab3cf1859d469a3e3acb29ebca504f2bf044c6cd2a1b0c3d91aec3e3197dd1a71af5", CPU_TYPE_X64_ALL, "aften.x64.exe",  8},
	{"1cca303fabd889a18fc01c32a7fd861194cfcac60ba63740ea2d7c55d049dbf8f59259fa", CPU_TYPE_ALL_ALL, "alac.exe", 20},
	{"6d22d4bbd7ce2162e38f70ac9187bc84eb28233b36ee6c0492d0a6195318782d7f05c444", CPU_TYPE_ALL_ALL, "avs2wav.exe", 13},
	{"f6375905541249f966b4caa3f90dd252841f68ecb50d22be2da86d666ad3e7b783e845ee", CPU_TYPE_ALL_ALL, "dcaenc.exe", 20120114},
	{"e53a787d4a0319453f4fe48c3145f190fcce7ac4802e521db908771437f6250746116e6c", CPU_TYPE_ALL_ALL, "elevator.exe", UINT_MAX},
	{"9ae98a3fc779f69ee876a3b477fbc35a709ba5066823b2eb62eeb015057c38807e4be51f", CPU_TYPE_ALL_ALL, "faad.exe", 27},
	{"446054f9a7f705f1aadc9053ca7b8a86a775499ef159978954ebdea92de056c34f8841f7", CPU_TYPE_ALL_ALL, "flac.exe", 121},
	{"52fce35084247acc12cd5d06701c143ade9a0915fc9902bb402abd164ea35f28717432a4", CPU_TYPE_ALL_ALL, "gpgv.exe", 1412},
	{"b3fca757b3567dab75c042e62213c231de378ea0fdd7fe29b733417cd5d3d33558452f94", CPU_TYPE_ALL_ALL, "gpgv.gpg", UINT_MAX},
	{"3fd15a6b5b0120794650f1dcd0c35f147cc21576e78f17425288bfacbad0b14696186739", CPU_TYPE_ALL_GEN, "lame.i386.exe", 3995},
	{"069a79d843939a65d8578f51b6acd09de95d44362c6a9c74e92a6e73ba40aea07916f7c4", CPU_TYPE_ALL_SSE, "lame.sse2.exe", 3995},
	{"d4d806fc3d0a36ef357ea43b870c7e46de9c18be9920f451314d72d02ba0fe4f7c867d9c", CPU_TYPE_ALL_ALL, "mac.exe", 411},
	{"ab3f6a8f2bc08011fdcea2e54a9b234ba67d304b5eea3fc0db653e603f938d0280fba0f0", CPU_TYPE_X86_ALL, "mediainfo.i386.exe", 753},
	{"c1d88d1b04f72118f21b5f574c4008fd0c99f3d6ed11cc9c8644b831971d1e1153cd63ea", CPU_TYPE_X64_ALL, "mediainfo.x64.exe",  753},
	{"ed49bfeb5113e8eca4f2f5c5c9359f6edeecf457cff8511178902c7d792380eaa578d9bc", CPU_TYPE_ALL_ALL, "mpcdec.exe", 475},
	{"436d788ca1514d1eb773a7e67fca413f35fff0d8aceed01d7817444db0ab1f1eced6521e", CPU_TYPE_ALL_ALL, "mpg123.exe", 1135},
	{"0c781805dda931c529bd16069215f616a7a4c5e5c2dfb6b75fe85d52b20511830693e528", CPU_TYPE_ALL_ALL, "oggdec.exe", UINT_MAX},
	{"0c019e13450dc664987e21f4e5489d182be7d6d0d81efbbaaf1c78693dfe3e38e0355b93", CPU_TYPE_X86_GEN, "oggenc2.i386.exe", 287603},
	{"693dd6f779df70a047c15c2c79350855db38d5b0cd7e529b6877b7c821cfe6addfdd50a4", CPU_TYPE_X86_SSE, "oggenc2.sse2.exe", 287603},
	{"291cedb6a1b213330a9cb508f975ee7132a25aa26770ab91cade50109b4ffb81c9bdd09a", CPU_TYPE_X64_ALL, "oggenc2.x64.exe",  287603},
	{"58c2b8bcff8f27bfa8fab8172b80f5da731221d072c7dba4dd3a3d7d6423490a25dc6760", CPU_TYPE_ALL_ALL, "shorten.exe", 361},
	{"39b0a21f1491efa752acd37017903ff8cfc5b1031750e70d5efad6119af55a532673b779", CPU_TYPE_ALL_ALL, "sox.exe", 1440},
	{"48e7f81c024cd17dac0eaeab253aad6b223e72dc80688f7576276b0563209514ff0bb9c8", CPU_TYPE_ALL_ALL, "speexdec.exe", 12},
	{"9b50cf64747d4afbad5d8d9b5a0a2d41c5a58256f47ebdbd8cc920e7e576085dfe1b14ff", CPU_TYPE_ALL_ALL, "tta.exe", 21},
	{"875871c942846f6ad163f9e4949bba2f4331bec678ca5aefe58c961b6825bd0d419a078b", CPU_TYPE_ALL_ALL, "valdec.exe", 31},
	{"e657331e281840878a37eb4fb357cb79f33d528ddbd5f9b2e2f7d2194bed4720e1af8eaf", CPU_TYPE_ALL_ALL, "wget.exe", 1114},
	{"8923cf65e181f8a99f28e9d1ea0d89ace02142241a7f76dae3d540ffd0790495af815644", CPU_TYPE_ALL_ALL, "wma2wav.exe", 20111001},
	{"b9529092b4cf05f1b30567d12b39d01d973dbf9b717dbd7aff10195f604e32bd93736a04", CPU_TYPE_ALL_ALL, "wupdate.exe", 20120310},
	{"6b053b37d47a9c8659ebf2de43ad19dcba17b9cd868b26974b9cc8c27b6167e8bf07a5a2", CPU_TYPE_ALL_ALL, "wvunpack.exe", 4601},
	{NULL, NULL, NULL, NULL}
};

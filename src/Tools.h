///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
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
	const char *pcHash;
	const unsigned int uiCpuType;
	const char *pcName;
	const unsigned int uiVersion;
	const char *pcVersTag;
}
g_lamexp_tools[] =
{
	{"4870d7abbc3995dc5ca82147ee33f28c3ee7dc1986fbb0131912effe10c1a209d3f983c5dffd963c31707a5ce39611b4", CPU_TYPE_X86_GEN, "aften.i386.exe", 8, ""},
	{"fc4e38b11a0f52b68cca79aa2d71c02180b70eb0e592923dee4f0ccf766f1006642369b2178f6a61d1c2506446cc442d", CPU_TYPE_X86_SSE, "aften.sse2.exe", 8, ""},
	{"b44b1c0e1a8f4633c9bc2e43d0a83ea5edc890e1cccc0054c812d0e64170e5292dc1dfd7430da61b73150831aa16e9c4", CPU_TYPE_X64_ALL, "aften.x64.exe",  8, ""},
	{"4558728999a05f12fe88566e8308cba0ada200814c2a1bfe2507f49faf8f3994b0d52a829148f5c6321d24faa2718439", CPU_TYPE_ALL_ALL, "avs2wav.exe", 13, ""},
	{"07834b1b8ecac2f3db79ad048005eca3a284cb759e15d399eb1a560a403122e20db5f08e60319759b8463aeef2410d02", CPU_TYPE_ALL_ALL, "dcaenc.exe", 20120419, ""},
	{"7c249f507b96967bedabdd7e631638807a7595ebff58eaaadf63530783d515eda9660bc2b1a0457fddae7e3eaef8a074", CPU_TYPE_ALL_ALL, "elevator.exe", UINT_MAX, ""},
	{"bbc262cfe9c48633e5f1780d30347d7663075cfd7bdc76347cce3b1191d62f788d9b91bc63dffae2f66d1759d5849e92", CPU_TYPE_ALL_ALL, "faad.exe", 27, ""},
	{"1f596224564452e66d0c717dfd776d0c4fa4fcc8650a424895a780b19d7acc8c8fb0f5c0501f2adfabc8a96da6074529", CPU_TYPE_ALL_ALL, "flac.exe", 130, ""},
	{"1633edab399014426b88da1bdf07b35ec7524303bcc972f7bcb4d86d06c6e808b840f2be63f105f0adfcc9116decc06a", CPU_TYPE_ALL_ALL, "gpgv.exe", 1416, ""},
	{"19c9dbe9089491c1f59ae48016d95d4336c4d3743577db4e782d8b59eca3b2bda6ed8f92f9004f88f434935b79e4974b", CPU_TYPE_ALL_ALL, "gpgv.gpg", UINT_MAX, ""},
	{"53cfab3896a47d48f523315f475fa07856d468ad1aefcc8cce19c18cdf509e2f92840dab92a442995df36d941cb7a6ca", CPU_TYPE_ALL_GEN, "lame.i386.exe", 3995, "Final"},
	{"9511e7ef2ad10de05386eedf7f14d637edab894a53dacd2f8f15c6f8ed582f12c25fb5bf88438e62c46b8eb92e7634b2", CPU_TYPE_ALL_SSE, "lame.sse2.exe", 3995, "Final"},
	{"fdbeb978025b9a5345300f37bb56446c31c7db594cf29407afdcc9ce20f4a5cec6eb8c03962c247d4f45b83c465ac705", CPU_TYPE_ALL_ALL, "mac.exe", 412, ""},
	{"975408454231ac8c51960907a7eea1dd4ef8f7e1269816737e8c072ab695de32d3dae433bc667f8d1343a60e95ddb38d", CPU_TYPE_X86_ALL, "mediainfo.i386.exe", 769, ""},
	{"3c049cb7e149413129cfc17588346bfa455554da6a6aec549a7b8e2a6f0953fa2a7f5d3425e6f41a26ed6b379c514687", CPU_TYPE_X64_ALL, "mediainfo.x64.exe",  769, ""},
	{"7e6346a057634ff07b2e1f427035324f7f02100cc996425990f87f71d767fce4c7b101588c7d944ba49cb2d7e51c9bdb", CPU_TYPE_ALL_ALL, "mpcdec.exe", 475, ""},
	{"96c6e22928f34a514d824f3d6e2fdc2cc1dcfb3e074ab109c989f54654e80c68f5f81648d1eca4199654289fd511df7f", CPU_TYPE_ALL_ALL, "mpg123.exe", 1190, ""},
	{"82d2f9c1422ecd4faa28eb3e0cb4ead95f35d46e445dbdd4319b1049dd11d37e3620c0e58d5d9c30fbfc3a49bae2d34d", CPU_TYPE_ALL_ALL, "oggdec.exe", 1101, ""},
	{"88f4bc96d5c2a266e8d054f591ec87e84f0d7253185b86fc49e5375d29ade506b7adfc948314d9b5f424c8b147a7a83a", CPU_TYPE_X86_GEN, "oggenc2.i386.exe", 134603, "2014"},
	{"35a356913c9169c11922e3f96086f38dd742e7d92581af73df17d3f44e99601531f4b9c6bd998d6294a6979fea1c9dba", CPU_TYPE_X86_SSE, "oggenc2.sse2.exe", 134603, "2014"},
	{"f4b610769910c4b6e03058c15aa94d964eaec1581aab957234125491ff6f0a2dad51012d9c7bb5a2a80e0d958f160b91", CPU_TYPE_X64_ALL, "oggenc2.x64.exe",  134603, "2014"},
	{"b8f8167ee344ada98d6cca831145c0f8f97a25875f34f8ee03395a5ac5203bfefca4e472f4c8ae5b9999bd2ded83bd7e", CPU_TYPE_ALL_GEN, "opusdec.i386.exe", 20140413, "v1.1"},
	{"262549c5f349c984e94451666419eaff4c0f5558719a729568f9131306c6455420d2551c956d8ba2edc2bce533103d8a", CPU_TYPE_ALL_SSE, "opusdec.sse2.exe", 20140413, "v1.1"},
	{"2127979169db9a1fbfeb71fb88b59f5f85047855b25bb3e5bff6ffadc1bc31a8369cd1902ea9e9efd184a94ce1be92bd", CPU_TYPE_ALL_GEN, "opusenc.i386.exe", 20140413, "v1.1"},
	{"b38e61a5c1d57f31349d035d3891038a0ca2c9a5a8252e3ae91ad98e23436d7c9c69cedfae26043fadf6ea215b2de5de", CPU_TYPE_ALL_SSE, "opusenc.sse2.exe", 20140413, "v1.1"},
	{"bdfa8dec142b6327a33af6bb314d7beb924588d1b73f2ef3f46b31fa6046fe2f4e64ca78b025b7eb9290a78320e2aa57", CPU_TYPE_ALL_ALL, "refalac.exe", 56, ""},
	{"d041b60de6c5c6e77cbad84440db57bbeb021af59dd0f7bebd3ede047d9e2ddc2a0c14179472687ba91063743d23e337", CPU_TYPE_ALL_ALL, "shorten.exe", 361, ""},
	{"cf988bfbb53e77a1dcaefbd5c08789abb4d67cc210723f1f8ba7850f17d34ebb7d0c426b67b963e7d2290a2744865244", CPU_TYPE_ALL_ALL, "sox.exe", 1441, ""},
	{"5a4261e1b41a59d1a5bc92e1d2766422a67454d77e06ea29af392811b7b4704e0f3e494ab9cb6375ce9e39257867c5ed", CPU_TYPE_ALL_ALL, "speexdec.exe", 12, ""},
	{"75d4c18dbb74e2dbf7342698428248d45cc4070d5f95da8831ef755e63dcd7ff9c3a760f289e8ef8b5c06b82548edbd8", CPU_TYPE_ALL_ALL, "tag.exe", 100, ""},
	{"a83628880da0b7519ec368a74a92da5a5099d8d46aa0583131f92d7321f47c9e16a1841b2a3fb8ffcca7205ef4b1bb0a", CPU_TYPE_ALL_ALL, "tta.exe", 21, ""},
	{"9e1ade2137ea5cee0ad4657971c314a372df3068594fbe4f77d45b9eb65fa7c69e55027b0df81b6fe072a220e9a8ba8a", CPU_TYPE_ALL_ALL, "valdec.exe", 100, "a"},
	{"509df39fdd7033b0f1af831304d0d6c08b74d5a48e2c038857a78b9dfaa4fb83c6b5c7ea202ba2270c0384607f2316ee", CPU_TYPE_ALL_ALL, "wget.exe", 1140, ""},
	{"572b9448bf4a338ecb9727951fdfcc5a219cc69896695cc96b9f6b083690e339910e41558968264a38992e45f2be152c", CPU_TYPE_ALL_ALL, "wma2wav.exe", 20111001, ""},
	{"cb4c284352e4c1a923e97d40ecac94fc492bf97dd830ea5ea08fcb7456da6f7152c376c2dfd28cd9bb9b48f96b05e0ec", CPU_TYPE_ALL_ALL, "wupdate.exe", 20140117, ""},
	{"221efeabe47e9bf65404c4687df156b326b4fd244d910ef937213e6b0169a57350e897140a2e9965822d60aada609f79", CPU_TYPE_ALL_ALL, "wvunpack.exe", 4700, ""},
	{NULL, NULL, NULL, NULL, NULL}
};

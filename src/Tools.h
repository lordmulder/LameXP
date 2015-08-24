///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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

#ifndef LAMEXP_INC_TOOLS
#error Please do *not* include TOOLS.H directly!
#endif

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
	{"36f9b021a8136664fce4c556571270f9151ca11d191b31630b0cad7b28107be2175909b8d28c903cb1c17d260c38d5de", CPU_TYPE_X86_ALL, "flac.i386.exe", 131, ""},
	{"68e08fc34f981d183f243308791b5e158c28903a2052ca239bb9ad3d54d976c9ec7f0bb0b50c0aadfcefb4a1d0f63a18", CPU_TYPE_X64_ALL, "flac.x64.exe" , 131, ""},
	{"9d2008d06b731b0d88377e9b2ceda30826ee787fe09ae8ef98084d5ef80655199e5069672366314b67a4285b56a282f0", CPU_TYPE_ALL_ALL, "gpgv.exe", 1419, ""},
	{"c66992384b6388a7b68f2659624c150b33ff1cacb0c28c0ab2a3249eca4c31e204953f325403f78c077d1735b275ac12", CPU_TYPE_ALL_ALL, "keyring.gpg", UINT_MAX, ""},
	{"53cfab3896a47d48f523315f475fa07856d468ad1aefcc8cce19c18cdf509e2f92840dab92a442995df36d941cb7a6ca", CPU_TYPE_ALL_GEN, "lame.i386.exe", 3995, "Final"},
	{"9511e7ef2ad10de05386eedf7f14d637edab894a53dacd2f8f15c6f8ed582f12c25fb5bf88438e62c46b8eb92e7634b2", CPU_TYPE_ALL_SSE, "lame.sse2.exe", 3995, "Final"},
	{"fdbeb978025b9a5345300f37bb56446c31c7db594cf29407afdcc9ce20f4a5cec6eb8c03962c247d4f45b83c465ac705", CPU_TYPE_ALL_ALL, "mac.exe", 412, ""},
	{"a503572a9fd5e75acfd5e70c8b6618db3076b3a865b9aea34443fccb6c363705b0c392e7fbfd50ba5cbf09b20f4d467a", CPU_TYPE_X86_ALL, "mediainfo.i386.exe", 774, ""},
	{"e25caa6e199a3f5d86b9b2fb5e4bcd369fa67b13b1afad982196b44ae323f55e8729b76a3ad83f97c2575342ac57a013", CPU_TYPE_X64_ALL, "mediainfo.x64.exe",  774, ""},
	{"7e6346a057634ff07b2e1f427035324f7f02100cc996425990f87f71d767fce4c7b101588c7d944ba49cb2d7e51c9bdb", CPU_TYPE_ALL_ALL, "mpcdec.exe", 475, ""},
	{"02f3e52fca662c97fc15ee5d5c6ded2966d716ec5e5ab958accd4761eb864cc7c62ace97198b876a78cf75ab11c555b2", CPU_TYPE_ALL_ALL, "mpg123.exe", 1224, ""},
	{"f1f2ea5c9e5539620b706e7af68e543bf7a731afb06ccce3815ab34dad64d697e4d6ffcd187a396619b8b52efe7edf88", CPU_TYPE_ALL_ALL, "oggdec.exe", 1101, ""},
	{"c203bb7abeed5745256ade3ddfb42af33f68814e0914627039353ed2fc4d501fac6b708360a157a48fbf5ba0dfb2c738", CPU_TYPE_X86_GEN, "oggenc2.i386.exe", 287134603, "2015"},
	{"da01a29ea7c3b09463b477c3dd10d8887ef08238f7d3f920bfa797f6b23940e5e38ac022653ae51cf5a5d44685782601", CPU_TYPE_X86_SSE, "oggenc2.sse2.exe", 287134603, "2015"},
	{"abef7e13d606406caded7d7669ddf46d88624a69556351d470ed9f599d6cbd7c2e99a6a56a7815b670b3a0a3b28f08d2", CPU_TYPE_X64_ALL, "oggenc2.x64.exe",  287134603, "2015"},
	{"d9949ab989fa412bffb08c47f431b74a66223aa584beb9a6aeaa66885932c2ed251798748cbc489e984f0acce0e0053c", CPU_TYPE_ALL_GEN, "opusdec.i386.exe", 20150326, "v1.1"},
	{"e3fc96044491bd96734dc25c8fdcb1760ee86a8fd47ad74481999ae18593d0647f0d4a45f058950b9def0903e8a30fc3", CPU_TYPE_ALL_SSE, "opusdec.sse2.exe", 20150326, "v1.1"},
	{"ae777a525a4670df1deca0483c5087f129d8131eaf946b2cd72fa96ab65db7fb600766448d28caf2102d97b3fa26e6bc", CPU_TYPE_ALL_GEN, "opusenc.i386.exe", 20150326, "v1.1"},
	{"8eadcdfe01a6ff2d88b6cfdf203c6eae6f858bd17b894644fa1d78d293235d8dc21b0102b8ca3d48f718251b3f2e9e5a", CPU_TYPE_ALL_SSE, "opusenc.sse2.exe", 20150326, "v1.1"},
	{"256882a5b7af7f23fe9ca6b63d9ec00482e54ee6f621581de385dac7a115046758151c45a97828936f7e967434b9bc19", CPU_TYPE_ALL_ALL, "refalac.exe", 147, ""},
	{"d041b60de6c5c6e77cbad84440db57bbeb021af59dd0f7bebd3ede047d9e2ddc2a0c14179472687ba91063743d23e337", CPU_TYPE_ALL_ALL, "shorten.exe", 361, ""},
	{"6e3f86cc464d84b0039139c9688e3097d0f42b794a5db10954d24fe77929585a0d0dba16cb677cc1b390392c39cdefad", CPU_TYPE_ALL_ALL, "sox.exe", 1442, ""},
	{"5a4261e1b41a59d1a5bc92e1d2766422a67454d77e06ea29af392811b7b4704e0f3e494ab9cb6375ce9e39257867c5ed", CPU_TYPE_ALL_ALL, "speexdec.exe", 12, ""},
	{"75d4c18dbb74e2dbf7342698428248d45cc4070d5f95da8831ef755e63dcd7ff9c3a760f289e8ef8b5c06b82548edbd8", CPU_TYPE_ALL_ALL, "tag.exe", 100, ""},
	{"a83628880da0b7519ec368a74a92da5a5099d8d46aa0583131f92d7321f47c9e16a1841b2a3fb8ffcca7205ef4b1bb0a", CPU_TYPE_ALL_ALL, "tta.exe", 21, ""},
	{"9e1ade2137ea5cee0ad4657971c314a372df3068594fbe4f77d45b9eb65fa7c69e55027b0df81b6fe072a220e9a8ba8a", CPU_TYPE_ALL_ALL, "valdec.exe", 100, "a"},
	{"509df39fdd7033b0f1af831304d0d6c08b74d5a48e2c038857a78b9dfaa4fb83c6b5c7ea202ba2270c0384607f2316ee", CPU_TYPE_ALL_ALL, "wget.exe", 1140, ""},
	{"572b9448bf4a338ecb9727951fdfcc5a219cc69896695cc96b9f6b083690e339910e41558968264a38992e45f2be152c", CPU_TYPE_ALL_ALL, "wma2wav.exe", 20111001, ""},
	{"53c7ce1de68285b2440e6d36887708d816cc5a16dfb076669b9ad24a84bb9a4f1080c1885259446b58673366d729cf33", CPU_TYPE_ALL_ALL, "wupdate.exe", 20150819, ""},
	{"221efeabe47e9bf65404c4687df156b326b4fd244d910ef937213e6b0169a57350e897140a2e9965822d60aada609f79", CPU_TYPE_ALL_ALL, "wvunpack.exe", 4700, ""},
	{NULL, NULL, NULL, NULL, NULL}
};

////////////////////////////////////////////////////////////
// AAC ENCODERS
////////////////////////////////////////////////////////////

typedef struct
{
	char *const       toolName;
	const char *const fileNames[8];
	const quint32     toolMinVersion;
	const quint32     verDigits;
	const quint32     verShift;
	const char *const verStr;
	const char *const regExpVer;
	const char *const regExpSig;
}
aac_encoder_t;

static const aac_encoder_t g_lamexp_aacenc[] =
{
	{ "NeroAAC",   { "neroAacEnc.exe", "neroAacDec.exe", "neroAacTag.exe",                                NULL }, lamexp_toolver_neroaac(),   4,        10, "v?.?.?.?",   "Package\\s+version:\\s+(\\d)\\.(\\d)\\.(\\d)\\.(\\d)", "Nero\\s+AAC\\s+Encoder" },
	{ "FhgAacEnc", { "fhgaacenc.exe", "enc_fhgaac.dll", "nsutil.dll", "libmp4v2.dll", "libsndfile-1.dll", NULL }, lamexp_toolver_fhgaacenc(), 1, 100000000, "????-??-??", "fhgaacenc version (\\d+) by tmkk",                                         NULL },
	{ "FdkAacEnc", { "fdkaac.exe",                                                                        NULL }, lamexp_toolver_fdkaacenc(), 3,        10, "v?.?.?",     "fdkaac\\s+(\\d)\\.(\\d)\\.(\\d)",                                          NULL },
	{ "QAAC",      { "qaac.exe", "libsoxr.dll", "libsoxconvolver.dll",                                    NULL }, lamexp_toolver_qaacenc(),   2,       100, "v?.??",      "qaac (\\d)\\.(\\d+)",                                                      NULL },
	{ "QAACx64",   { "qaac64.exe", "libsoxr64.dll", "libsoxconvolver64.dll",                              NULL }, lamexp_toolver_qaacenc(),   2,       100, "v?.??",      "qaac (\\d)\\.(\\d+)",                                                      NULL },
	{ NULL, { NULL }, 0, 0, 0, NULL, NULL, NULL }
};

///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
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
#define CPU_TYPE_X86_AVX 0x00000004UL //x86, with AVX support
#define CPU_TYPE_X64_GEN 0x00000008UL //x64, generic
#define CPU_TYPE_X64_SSE 0x00000010UL //x64, with SSE and SSE2 support - Intel only!
#define CPU_TYPE_X64_AVX 0x00000020UL //x64, with AVX support

/* combined CPU types */
#define CPU_TYPE_X86_NVX (CPU_TYPE_X86_GEN|CPU_TYPE_X86_SSE)
#define CPU_TYPE_X86_SSX (CPU_TYPE_X86_SSE|CPU_TYPE_X86_AVX)
#define CPU_TYPE_X86_ALL (CPU_TYPE_X86_GEN|CPU_TYPE_X86_SSX)
#define CPU_TYPE_X64_NVX (CPU_TYPE_X64_GEN|CPU_TYPE_X64_SSE)
#define CPU_TYPE_X64_SSX (CPU_TYPE_X64_SSE|CPU_TYPE_X64_AVX)
#define CPU_TYPE_X64_ALL (CPU_TYPE_X64_GEN|CPU_TYPE_X64_SSX)
#define CPU_TYPE_ALL_GEN (CPU_TYPE_X86_GEN|CPU_TYPE_X64_GEN)
#define CPU_TYPE_ALL_SSE (CPU_TYPE_X86_SSE|CPU_TYPE_X64_SSE)
#define CPU_TYPE_ALL_SSX (CPU_TYPE_X86_SSX|CPU_TYPE_X64_SSX)
#define CPU_TYPE_ALL_AVX (CPU_TYPE_X86_AVX|CPU_TYPE_X64_AVX)
#define CPU_TYPE_ALL_ALL (CPU_TYPE_X86_ALL|CPU_TYPE_X64_ALL)

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
	{"4870d7abbc3995dc5ca82147ee33f28c3ee7dc1986fbb0131912effe10c1a209d3f983c5dffd963c31707a5ce39611b4", CPU_TYPE_X86_GEN, "aften.i686.exe", 8, ""},
	{"fc4e38b11a0f52b68cca79aa2d71c02180b70eb0e592923dee4f0ccf766f1006642369b2178f6a61d1c2506446cc442d", CPU_TYPE_X86_SSX, "aften.sse2.exe", 8, ""},
	{"b44b1c0e1a8f4633c9bc2e43d0a83ea5edc890e1cccc0054c812d0e64170e5292dc1dfd7430da61b73150831aa16e9c4", CPU_TYPE_X64_ALL, "aften.x64.exe",  8, ""},
	{"4558728999a05f12fe88566e8308cba0ada200814c2a1bfe2507f49faf8f3994b0d52a829148f5c6321d24faa2718439", CPU_TYPE_ALL_ALL, "avs2wav.exe", 13, ""},
	{"07834b1b8ecac2f3db79ad048005eca3a284cb759e15d399eb1a560a403122e20db5f08e60319759b8463aeef2410d02", CPU_TYPE_ALL_ALL, "dcaenc.exe", 20120419, ""},
	{ "5c4a5cdd708b5857bcb064558be81a2dfb16401e796b296f6eee7c63042acbeae12e1e2f1f3d0fd096eaf73201b54e10", CPU_TYPE_ALL_GEN, "faad.i686.exe", 270, "" },
	{ "72447794cf411e1e4ce71facf5f60023f001d203894cf40185a0ee13e144e93d72ac99f2ed30a9168450ce5bf882f99f", CPU_TYPE_ALL_SSX, "faad.sse2.exe", 270, "" },
	{"735654150f5d123660aa8493d322a7181cfd6d6c3e584a53c94c27a5163659b6aab3dd84230f684f8cec3cb565223482", CPU_TYPE_X86_GEN, "flac.x86-i686.exe", 132, ""},
	{"2ee22f174131615cf342cb3031a4c93adfff0997d84eb1b6794c133c8f19d109c43e67a3f60638705c7afb93427a07f8", CPU_TYPE_X86_SSX, "flac.x86-sse2.exe", 132, ""},
	{"323ba94d1d3a8f82f4e5a6adf859d0f11d7b0c69fa0254e620724f879edd9d7017e3d2fa78ffe0a615539d8798606a46", CPU_TYPE_X64_NVX, "flac.x64-sse2.exe", 132, ""},
	{"db5c8b27f876f797d8e2c7dd6c585cff065174c2a006a1642be2b8339be7d17e49b806534e3c9f2de84e66e997c49df7", CPU_TYPE_X64_AVX, "flac.x64-avx.exe",  132, ""},
	{"59755982390fc95ba6678e1fa0fef52c3b04a71f4265feee8f260a2e3dbd6cd5a5c1518b34ba5cf9d133381740467298", CPU_TYPE_ALL_ALL, "gpgv.exe", 1422, ""},
	{"c66992384b6388a7b68f2659624c150b33ff1cacb0c28c0ab2a3249eca4c31e204953f325403f78c077d1735b275ac12", CPU_TYPE_ALL_ALL, "keyring.gpg", UINT_MAX, ""},
	{"5e95b7d07d4ffc1ed9b2f3ccebd1a13caf6362dad39c594d94a1c15b80ba81b296cb8ce77487bf98c9650ce76f7a845b", CPU_TYPE_X86_GEN, "lame.x86-i686.exe", 31001, "Final"},
	{"cc5809e842fa0d9ceee2de7ebe3ad4d1cbfdc8b1a7aa91fa14ce3f690bd2280ce23648c178a93d41652189fb06c2600b", CPU_TYPE_X86_SSX, "lame.x86-sse2.exe", 31001, "Final"},
	{"578b230ce82a1a588d8ba21163acb3b78b4708fb3ef9f9b685a4fcb71a63f027b33aa3b12acd7815502ad9475d63fc3b", CPU_TYPE_X64_NVX, "lame.x64-sse2.exe", 31001, "Final" },
	{"789e28b1e7dfa43bf4f8a70213fdc77b480e386f7a36c258395240c93d643bae8cee9c8728376e9ec7360e169d5f59ec", CPU_TYPE_X64_AVX, "lame.x64-avx.exe",  31001, "Final" },
	{"c0f6508f8b7ab515b69f55ed9d5eddb9dad22da4e6268c86e93dc23472f6a049f2dd97a0d4c85b75d8938fb999c19736", CPU_TYPE_X86_GEN, "mac.x86-i686.exe", 433, ""},
	{"031cb32078841df55691cdb91c9ed0e0c3113b195fe69c56746b0ba607c11f7c978ebe9500a46580926713b9d71454c4", CPU_TYPE_X86_SSX, "mac.x86-sse2.exe", 433, ""},
	{"8e197a2c759a74ced2acfee18c7cdf37d89534a6568467f886cdebf6c50dba0b79f4f06ee7424c6d6596d94389d03bcc", CPU_TYPE_X64_NVX, "mac.x64-sse2.exe", 433, ""},
	{"0ef10f812dfa91634da213328e6bfb84b6f8b1882b955f1c3173460d60e559a8f5065125c698552301ed0566b2dc87bf", CPU_TYPE_X64_AVX, "mac.x64-avx.exe",  433, ""},
	{"1ce2b535d6db9c28672c709874275726bf8b34e0183ce3cf033ff5e83e8c0517cf35cc90956ddccd01917a9cca7ab6f0", CPU_TYPE_X86_GEN, "mediainfo.i686.exe", 1805, ""},
	{"8b1bd63008aa1ec5eb4e3ce10dba6d6d451b7967a6fc2b1203e95b03d589c716456a2e6bba557f2dbe8b65fc54c4c657", CPU_TYPE_X86_SSX, "mediainfo.sse2.exe", 1805, ""},
	{"5d419df0e0e82532ec0e74203cc026c3ee7d907b381e5b9f844c67cda78ffc9a068522fcc64e3cccec9b4b4b2b27fb5b", CPU_TYPE_X64_ALL, "mediainfo.x64.exe",  1805, ""},
	{"7e6346a057634ff07b2e1f427035324f7f02100cc996425990f87f71d767fce4c7b101588c7d944ba49cb2d7e51c9bdb", CPU_TYPE_ALL_ALL, "mpcdec.exe", 475, ""},
	{"54d45af70bb6f251634dbdd0de10269cac2c2e803d94623a96279b31fb7506e2b68a65fd596b19223ab3026bcbb87c22", CPU_TYPE_ALL_GEN, "mpg123.i686.exe", 12510, ""},
	{"efc53e539aab547e79b4e9be5f19be98896cf2571b9ecfdb70fa05c4972451baf6e9cdf75ec3d5bb420dde3d3165f8bf", CPU_TYPE_ALL_SSE, "mpg123.sse2.exe", 12510, ""},
	{"53bd4d5c570a5a368205cb585759d4d046378918b58acda1c5d720fdb78f44370b71d0f2ee56a24f3a78cfea8d635c00", CPU_TYPE_ALL_AVX, "mpg123.avx.exe",  12510, ""},
	{"dda88fb66a80c362dfa367d07265eee3dcf8ee959191fc7685163fdee694ece7d84000065de217942749b6859d33fa84", CPU_TYPE_ALL_ALL, "mcat.exe", 101, "" },
	{"8e9b65f4bee39deceb303e5fdfe6875872fe450e29151ef3546128e7133a8eb3d14daae446e0c17c56ad3b969294367c", CPU_TYPE_ALL_GEN, "oggdec.i686.exe", 1101, ""},
	{"5fb1d7781de9449eed958a175305a9827b158f69cd83131da6b92cd78ee7ea3dcc47ed6b3ee58e2892fe6cf98a994730", CPU_TYPE_ALL_SSE, "oggdec.sse2.exe", 1101, ""},
	{"1f1f52553703afc11a428a4d5c9211f91cf9d39019ade3e049478e5a853b9a8c0a0d11304a04feb3a37cac6904244bab", CPU_TYPE_ALL_AVX, "oggdec.avx.exe",  1101, ""},
	{"245181321625445ac42fce31d64bf03872e77e2d0dd3c19d6c17ca2771354f096a6040827dd6d00ffd7342c7dd26168e", CPU_TYPE_X86_GEN, "oggenc2.i686.exe", 288135603, "2015"},
	{"512b8efcd1003a0f67220a450d6ea4466194e8fd49fc090a69b15a858db11499acbf98f984530cd5d37b4b6abdd1c6d8", CPU_TYPE_X86_SSX, "oggenc2.sse2.exe", 288135603, "2015"},
	{"a07ef67cba5a00d335d07372baf76d4d0573b425afce71a19c1e04eaabbe3f55e60bdd40af5e428224c91df1823eda08", CPU_TYPE_X64_ALL, "oggenc2.x64.exe",  288135603, "2015"},
	{"89106ad0786cdbbe065b84bee6413025391af9a2ed451bb34b6808c2eb4e32df1abef32f15e365708f8ab0d787ada9f4", CPU_TYPE_ALL_GEN, "opusdec.i686.exe", 20180603, "v1.3-rc-1"},
	{"91751b8f84c6b3a75f8e7190cf14e5a799d227a50637745482b13ae40f38b6294527f6575235bb73edcbb6651e883124", CPU_TYPE_ALL_SSE, "opusdec.sse2.exe", 20180603, "v1.3-rc-1"},
	{"0b2445ce4bc91ea15a3c94693ffa8e53137bf398fbf285cc88d1458d5eb50ad9b19facffa4e38e1481462fed60e29b41", CPU_TYPE_ALL_AVX, "opusdec.avx.exe",  20180603, "v1.3-rc-1"},
	{"1afe8833c94ec3bf56a119fafe494ddfa6e002c8bddb50cfea18a8aa5b12f70afeb4d5be677dc0d02db087757049d57e", CPU_TYPE_ALL_GEN, "opusenc.i686.exe", 20180603, "v1.3-rc-1"},
	{"4f3a4b473c6eedc8c70d278e3dd23c90293ec2e614853482367f55a3b5298dc69f09deedb1d7110160f8dae90f5a03e8", CPU_TYPE_ALL_SSE, "opusenc.sse2.exe", 20180603, "v1.3-rc-1"},
	{"b40a3ab425e19c67f02d81b1fd8d0c09cda8979a76af70d5802bc9a0010ae6728bd918e242e70e02f0efd26b317cb47c", CPU_TYPE_ALL_AVX, "opusenc.avx.exe",  20180603, "v1.3-rc-1"},
	{"155b123845f797226d03c44d649de810b13001b70e55e3775559ee78b75c38c9284475d217a86752820cfe1c22f37f69", CPU_TYPE_X86_GEN, "refalac.i686.exe", 164, ""},
	{"2bdb8a6e4ad05669623173101ccc9287b9d8b6bb5b2062abaadda3e3cceda7040478ef2b404348b52e2232307f8c8f7c", CPU_TYPE_X86_SSX, "refalac.sse2.exe", 164, ""},
	{"3abae08b7be8d7e054bf48b7a7cbb874955a774dc2db69503490d59daf74db8adfa702401eeace6536810b0ac15dea84", CPU_TYPE_X64_ALL, "refalac.x64.exe",  164, ""},
	{"d041b60de6c5c6e77cbad84440db57bbeb021af59dd0f7bebd3ede047d9e2ddc2a0c14179472687ba91063743d23e337", CPU_TYPE_ALL_ALL, "shorten.exe", 361, ""},
	{"3206ebd1b1c6e5db422d7a84117a7ba8256208fc7104a6668d8856c1b6407882f25f1f39e37c8e33affb343300937d2e", CPU_TYPE_ALL_GEN, "sox.i686.exe", 1442, ""},
	{"16a71940aa5e9b393e83cdfb2a4dda4291ea01858f8ba338510013f25f4d7c53b9b5cffb86404ea49ef28e6795182fd5", CPU_TYPE_ALL_SSE, "sox.sse2.exe", 1442, "" },
	{"25585ca9e2e025d82d93341a9db8527eb0b4ce451dade607f9784a79ed30e050ced0824835d5467aa0bf0c6b8fe08612", CPU_TYPE_ALL_AVX, "sox.avx.exe",  1442, "" },
	{"5a4261e1b41a59d1a5bc92e1d2766422a67454d77e06ea29af392811b7b4704e0f3e494ab9cb6375ce9e39257867c5ed", CPU_TYPE_ALL_ALL, "speexdec.exe", 12, ""},
	{"75d4c18dbb74e2dbf7342698428248d45cc4070d5f95da8831ef755e63dcd7ff9c3a760f289e8ef8b5c06b82548edbd8", CPU_TYPE_ALL_ALL, "tag.exe", 100, ""},
	{"a83628880da0b7519ec368a74a92da5a5099d8d46aa0583131f92d7321f47c9e16a1841b2a3fb8ffcca7205ef4b1bb0a", CPU_TYPE_ALL_ALL, "tta.exe", 21, ""},
	{"9e1ade2137ea5cee0ad4657971c314a372df3068594fbe4f77d45b9eb65fa7c69e55027b0df81b6fe072a220e9a8ba8a", CPU_TYPE_ALL_ALL, "valdec.exe", 100, "a"},
	{"0a2c8afb50aac35b80f34be2e2286bbb4f0876c1ad53fed8dc2f679786671eafdc1a244378abcc2c229fc110bb5cdf79", CPU_TYPE_ALL_ALL, "wget.exe", 1180, ""},
	{"572b9448bf4a338ecb9727951fdfcc5a219cc69896695cc96b9f6b083690e339910e41558968264a38992e45f2be152c", CPU_TYPE_ALL_ALL, "wma2wav.exe", 20111001, ""},
	{"5ef85aa6c6521161e19fc9eadd30bac82c3d0eee2374fd6ac543022181f7846ec2198ebe8bc84667e9b92a4a85d07fbb", CPU_TYPE_ALL_ALL, "wupdate.exe", 20171002, ""},
	{"6021b938769b09d05617c7e91e0cb6cc5f9e40c50cb470455afc21d466b37183b8675822b8797cbf98950a5262ec07a6", CPU_TYPE_X86_GEN, "wvunpack.x86-i686.exe", 5010, ""},
	{"fa322e127679ac6b5e833e120b590480b3ffc7ffa875705c04ed698f155dc23b40c8922d2f5f78ad1cc9342306289141", CPU_TYPE_X86_SSX, "wvunpack.x86-sse2.exe", 5010, ""},
	{"d4ca3085aae70160beab778a46a27643f1415bd803ddfecb2791fe964e4bff49ac5a891ef1852f26e867c42ddd6f8806", CPU_TYPE_X64_NVX, "wvunpack.x64-sse2.exe", 5010, ""},
	{"e2c32a765cbec41cc6e662447a1d70d8261753f2c93aca666bafddcc420c4eaee550d3c730123a84565cf43fffa26c35", CPU_TYPE_X64_AVX, "wvunpack.x64-avx.exe",  5010, ""},
	{NULL, NULL, NULL, NULL, NULL}
};

////////////////////////////////////////////////////////////
// AAC ENCODERS
////////////////////////////////////////////////////////////

typedef struct
{
	const char *const toolName;
	const char *const fileNames[5];
	const char *const checkArgs;
	const quint32     toolMinVersion;
	const quint32     verDigits;
	const quint32     verShift;
	const char *const verStr;
	const char *const regExpVer;
	const char *const regExpSig;
	const char *const regExpLib[3];
}
aac_encoder_t;

static const aac_encoder_t g_lamexp_aacenc[] =
{
	{ 
		"NeroAAC",
		{ "neroAacEnc.exe", "neroAacDec.exe", "neroAacTag.exe", NULL },
		"-help",
		lamexp_toolver_neroaac(), 4, 10, "v?.?.?.?",
		"Package\\s+version:\\s+(\\d)\\.(\\d)\\.(\\d)\\.(\\d)",
		"Nero\\s+AAC\\s+Encoder",
		{ NULL }
	},
	{
		"FhgAacEnc",
		{ "fhgaacenc.exe",  "enc_fhgaac.dll", "nsutil.dll", "libmp4v2.dll", NULL },
		NULL,
		lamexp_toolver_fhgaacenc(), 2, 0, "????-??-??",
		"fhgaacenc version (\\d+) by tmkk. Modified by Case (\\d+).",
		NULL,
		{ NULL }
	},
	{ 
		"FdkAacEnc",
		{ "fdkaac.exe", NULL },
		"--help",
		lamexp_toolver_fdkaacenc(), 3, 10, "v?.?.?",
		"fdkaac\\s+(\\d)\\.(\\d)\\.(\\d)",
		NULL,
		{ NULL }
	},
	{
		"QAAC",
		{ "qaac.exe", "libsoxr.dll", "libsoxconvolver.dll", NULL },
		"--check",
		lamexp_toolver_qaacenc(), 2, 100, "v?.??",
		"qaac (\\d)\\.(\\d+)",
		NULL,
		{ "libsoxr-\\d+\\.\\d+\\.\\d+", "libsoxconvolver\\s+\\d+\\.\\d+\\.\\d+" }
	},
	{
		"QAACx64",
		{ "qaac64.exe", "libsoxr64.dll", "libsoxconvolver64.dll", NULL },
		"--check",
		lamexp_toolver_qaacenc(), 2, 100, "v?.??",
		"qaac (\\d)\\.(\\d+)",
		NULL,
		{ "libsoxr-\\d+\\.\\d+\\.\\d+", "libsoxconvolver\\s+\\d+\\.\\d+\\.\\d+" }
	},
	{ NULL, { NULL }, NULL, 0, 0, 0, NULL, NULL, NULL, { NULL } }
};

///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2023 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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
#define CPU_TYPE_X86_SSE 0x00000002UL //x86, with SSE and SSE2 support (SSE2 implies SSE)
#define CPU_TYPE_X86_AVX 0x00000004UL //x86, with AVX and AVX2 support (AVX2 implies AVX)
#define CPU_TYPE_X64_SSE 0x00000008UL //x64, with SSE and SSE2 support (x64 implies SSE2)
#define CPU_TYPE_X64_AVX 0x00000010UL //x64, with AVX and AVX2 support (AVX2 implies AVX)

/* combined CPU types */
#define CPU_TYPE_X86_NVX (CPU_TYPE_X86_GEN|CPU_TYPE_X86_SSE) //x86, generic or SSE2
#define CPU_TYPE_X86_SSX (CPU_TYPE_X86_SSE|CPU_TYPE_X86_AVX) //x86, SSE2 or AVX2
#define CPU_TYPE_X86_ALL (CPU_TYPE_X86_GEN|CPU_TYPE_X86_SSX) //x86, generic or SSE2 or AVX2
#define CPU_TYPE_X64_SSX (CPU_TYPE_X64_SSE|CPU_TYPE_X64_AVX) //x64, SSE2 or AVX2
#define CPU_TYPE_ALL_SSE (CPU_TYPE_X86_SSE|CPU_TYPE_X64_SSE) //any, SSE2
#define CPU_TYPE_ALL_SSX (CPU_TYPE_X86_SSX|CPU_TYPE_X64_SSX) //any, SSE2 or AVX2
#define CPU_TYPE_ALL_AVX (CPU_TYPE_X86_AVX|CPU_TYPE_X64_AVX) //any, AVX2
#define CPU_TYPE_ALL_ALL (CPU_TYPE_X86_ALL|CPU_TYPE_X64_SSX) //any, generic or SSE2 or AVX2

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
	{"b44b1c0e1a8f4633c9bc2e43d0a83ea5edc890e1cccc0054c812d0e64170e5292dc1dfd7430da61b73150831aa16e9c4", CPU_TYPE_X64_SSX, "aften.x64.exe",  8, ""},
	{"4558728999a05f12fe88566e8308cba0ada200814c2a1bfe2507f49faf8f3994b0d52a829148f5c6321d24faa2718439", CPU_TYPE_ALL_ALL, "avs2wav.exe", 13, ""},
	{"5359f53d9543dd4798a4b06dea8ba13771174081fd0eebe5f74c69887d703d1be4ea8a1951bb190e7d7fece093b7d691", CPU_TYPE_ALL_ALL, "curl.exe", 8050, ""},
	{"86ab64f23b2557461356ed6c5c1a032df75f6206f1123743db1239c5085d00efa2af9b3cf90f0220e94da86a84903e22", CPU_TYPE_ALL_ALL, "curl.crt", 8050, ""},
	{"07834b1b8ecac2f3db79ad048005eca3a284cb759e15d399eb1a560a403122e20db5f08e60319759b8463aeef2410d02", CPU_TYPE_ALL_ALL, "dcaenc.exe", 20120419, ""},
	{"5c4a5cdd708b5857bcb064558be81a2dfb16401e796b296f6eee7c63042acbeae12e1e2f1f3d0fd096eaf73201b54e10", CPU_TYPE_X86_GEN, "faad.i686.exe", 270, "" },
	{"72447794cf411e1e4ce71facf5f60023f001d203894cf40185a0ee13e144e93d72ac99f2ed30a9168450ce5bf882f99f", CPU_TYPE_ALL_SSX, "faad.sse2.exe", 270, "" },
	{"ac0d5ebb99abe4f96fc06db19305259d44a757b890c9b40112ad5e83de65f2bd1c5695d1b68883f05febaf5c72212f36", CPU_TYPE_X86_GEN, "flac.x86-i686.exe", 143, ""},
	{"54877e1540ffae4f80376b01885aeb6daabec1d66e6b1352d2975f71b9a8c6c4d7e2247d4cc127ce8ae9ba8e0577289f", CPU_TYPE_X86_SSX, "flac.x86-sse2.exe", 143, ""},
	{"2adbff497b38882f95e274c165292410b23dce2f7507c3806f80d465e62c8338f38ec907037909f92834a85ceae84f2d", CPU_TYPE_X64_SSE, "flac.x64-sse2.exe", 143, ""},
	{"3357672c34feb014fd972356366d5655dd01365e24d3942e63fc036aea7c59df6024ca4c67b0c2589d69a0a622b36b92", CPU_TYPE_X64_AVX, "flac.x64-avx2.exe", 143, ""},
	{"c1b7a380ad9dbb5641de914ecfb9d860dd50159b1cace8ddf36780ffaf9683efec7a42aa7bea796c3ecbf52a4c1d3ee6", CPU_TYPE_X86_GEN, "lame.x86-i686.exe", 31013, "Beta"},
	{"663e7a4c2b946687956d613fe19b831c75a49ff03b46d7f912b695af0275463cc08e58fdc71f28266385b766dce785cc", CPU_TYPE_X86_SSX, "lame.x86-sse2.exe", 31013, "Beta"},
	{"3b8607f8d78760d6d17caf4c9593a70259de2f8d968eb5a83a730182b504a25172a569bc94d7f5b6a19ce92a6fae11b2", CPU_TYPE_X64_SSE, "lame.x64-sse2.exe", 31013, "Beta" },
	{"bb41afdedc81ea7abb5beb451b7451d07abd91ddb7d0e1e6059a4d4482cdcb4ba1c6d5556a470e3adef882f579714311", CPU_TYPE_X64_AVX, "lame.x64-avx2.exe", 31013, "Beta" },
	{"78bb531c2e9b1905404a2c3cabe4dce44b81e9ac700b0b6d48ed9b22c554eed849ffc02654339f54ef4d2104786a50a5", CPU_TYPE_X86_GEN, "mac.x86-i686.exe", 1034, ""},
	{"64d0bf5daf12ba877f924d33746acd128b0f4d0414803c1e39f93d6e5722402dbeae034a49ffa6f2cdc375381a46b2e4", CPU_TYPE_X86_SSX, "mac.x86-sse2.exe", 1034, ""},
	{"a6c4cd31f11621bb43c6398df1d6207346c27b4f03ded34204f7bc3e1f651c73e059a2779194dc0ffa5fa99930c83ffb", CPU_TYPE_X64_SSE, "mac.x64-sse2.exe", 1034, ""},
	{"f555938cee3b00ad7ff32fa6e2afd66e0bf1056b6418948a6c255d57452f7fafc4a7b52695bc52039be3e1cc1c91ac74", CPU_TYPE_X64_AVX, "mac.x64-avx2.exe", 1034, ""},
	{"64982cc28bdc7166b415d9a02c9ae78bf13ec24409c69f2811d797f96e0ba40395d8b5b50be07eb7163d59ce0336a806", CPU_TYPE_X86_GEN, "mediainfo.x86-i686.exe", 23110, ""},
	{"2a600f9ff808186666b242b873a92552276eb111467350552ff67134e73f9dacfbf9b94826bd970c41e7cef2fd00ab3b", CPU_TYPE_X86_SSX, "mediainfo.x86-sse2.exe", 23110, ""},
	{"7926cd4008320273a0cbe7722504277461e9dda5c7b60664eee71626da7f067c26d32cd0016ee0f40b5c40d5990732f6", CPU_TYPE_X64_SSE, "mediainfo.x64-sse2.exe", 23110, ""},
	{"a25449a751c84d632c382bd607fc3ba8e549392d09a99eed3a8a248f35701fbc27bfe104c8513512862c17830ded8bbf", CPU_TYPE_X64_AVX, "mediainfo.x64-avx2.exe", 23110, ""},
	{"7e6346a057634ff07b2e1f427035324f7f02100cc996425990f87f71d767fce4c7b101588c7d944ba49cb2d7e51c9bdb", CPU_TYPE_ALL_ALL, "mpcdec.exe", 475, ""},
	{"6dd88d9c78a90a6e236cc18d636b9d39b7774532e70c9e40f6cdb9dd0556f018a3dce264608d3395a13bc77851c3e481", CPU_TYPE_X86_GEN, "mpg123.x86-i686.exe", 13203, ""},
	{"cb856a4c00e3e7b9d06591186d2a3cec46b7100a09203883da10e732c984872caac466d97965893c005e99dbe3fff23e", CPU_TYPE_X86_SSX, "mpg123.x86-sse2.exe", 13203, ""},
	{"79acf086c3252fd157cd1441cdcc1d875232dfafeb5da27d4ffa6c0af26b02e61592433656d4ffa0718194798c6bf6a0", CPU_TYPE_X64_SSE, "mpg123.x64-sse2.exe", 13203, ""},
	{"553a084723eb56fb5ca851e053725a09806f734cab9bcb0050a146d58c594dfaaa2c81a85ab5ac312d00ae187ea5f207", CPU_TYPE_X64_AVX, "mpg123.x64-avx2.exe", 13203, ""},
	{"8e9b65f4bee39deceb303e5fdfe6875872fe450e29151ef3546128e7133a8eb3d14daae446e0c17c56ad3b969294367c", CPU_TYPE_X86_GEN, "oggdec.i686.exe", 1101, ""},
	{"5fb1d7781de9449eed958a175305a9827b158f69cd83131da6b92cd78ee7ea3dcc47ed6b3ee58e2892fe6cf98a994730", CPU_TYPE_ALL_SSE, "oggdec.sse2.exe", 1101, ""},
	{"1f1f52553703afc11a428a4d5c9211f91cf9d39019ade3e049478e5a853b9a8c0a0d11304a04feb3a37cac6904244bab", CPU_TYPE_ALL_AVX, "oggdec.avx.exe",  1101, ""},
	{"5acb313abc310299987734edc9aa2290c24692284a79cdcac3ec85900284c80297c2a0b95c1f8ac01790dd181fb221db", CPU_TYPE_X86_GEN, "oggenc2.i686.exe", 288137603, "2018"},
	{"e0e91f07a427632a6fe2eed697a50311bf658cb61cb0f26eacf4b64487dac54022ab429587221a0375b6e3486f0e0bd1", CPU_TYPE_X86_SSX, "oggenc2.sse2.exe", 288137603, "2018"},
	{"3630078654f0ed43579d0eb11dba7752892afc361864d1620bcc49f233384c221827bbe88644f4a5b5aceeac4da7fe6c", CPU_TYPE_X64_SSX, "oggenc2.x64.exe",  288137603, "2018"},
	{"f8ab00ce62c10056806b36f3eab1162a594166b1d672cc431f8036a6997805158e7fd35daa1be5699ddc9ba8f20c1d9f", CPU_TYPE_X86_GEN, "opusdec.x86-i686.exe", 20231103, "v1.4.0"},
	{"391c1a1973c80edfca54e721b2e663fa3de9276536c33fa32dbe313fe350ec9f0d96c7412fd405f7441bc98e8481cf88", CPU_TYPE_X86_SSX, "opusdec.x86-sse2.exe", 20231103, "v1.4.0"},
	{"b8e99f0f7aea57e657ac3043f15b85f16cd32f851ad0a29ac0d55ff40516ea08ced3088d3a6f251c351956da86770ed4", CPU_TYPE_X64_SSE, "opusdec.x64-sse2.exe", 20231103, "v1.4.0"},
	{"19818a007947458f30121d6e9a30172c261a19c439914ffb2a212b4a6e967b3d05af4fa945de50318505c4be06bcb026", CPU_TYPE_X64_AVX, "opusdec.x64-avx2.exe", 20231103, "v1.4.0"},
	{"8196e0080ecc356369f30080029c00be84252ff493d91bc9a25c93fdb12d7482219ea2d203c71e5719d87f6a22635328", CPU_TYPE_X86_GEN, "opusenc.x86-i686.exe", 20231103, "v1.4.0"},
	{"7cd4ca062623545e9fb54b58bdf83780471b91622cc2c4307879c1c8647ef0550fbd301371d6db08f1f289f284bb0514", CPU_TYPE_X86_SSX, "opusenc.x86-sse2.exe", 20231103, "v1.4.0"},
	{"61a2d491028ece594a837a4459c937fe0675aaecaa8dab7bf330b04a3950a5908b2f3a0dd196563450d231fcb3118bd4", CPU_TYPE_X64_SSE, "opusenc.x64-sse2.exe", 20231103, "v1.4.0"},
	{"8b1f944ac5a5e8f7f8afc3aa1683f8d51933f5c9a71e522ec2fbd8117dedcdf48b35649b6f6233bd5ab55d9e44117dfa", CPU_TYPE_X64_AVX, "opusenc.x64-avx2.exe", 20231103, "v1.4.0"},
	{"5e9d7b62c1cded2e0ec48439e830c2f50fb3d2fd16b69cf55de6fb95bcbbc8e704f6bced006470a124789f32ffb66dda", CPU_TYPE_X86_GEN, "refalac.i686.exe", 180, ""},
	{"477ebc29f0b9fc94e09e9f65509f379a7b0d9d601268c6cd2eb9ca0ad38deb4af2a40e8eb30d7ccd5fc27fc1f3a70cda", CPU_TYPE_X86_SSX, "refalac.sse2.exe", 180, ""},
	{"cc7a85b5c0c42054a580c9ab5ea004f1ec93c21762eb7a01f2266beb49ef867d2d06731d7c1ff2f28cf6c0472f80db4f", CPU_TYPE_X64_SSX, "refalac.x64.exe",  180, ""},
	{"d041b60de6c5c6e77cbad84440db57bbeb021af59dd0f7bebd3ede047d9e2ddc2a0c14179472687ba91063743d23e337", CPU_TYPE_ALL_ALL, "shorten.exe", 361, ""},
	{"3206ebd1b1c6e5db422d7a84117a7ba8256208fc7104a6668d8856c1b6407882f25f1f39e37c8e33affb343300937d2e", CPU_TYPE_X86_GEN, "sox.i686.exe", 1442, ""},
	{"16a71940aa5e9b393e83cdfb2a4dda4291ea01858f8ba338510013f25f4d7c53b9b5cffb86404ea49ef28e6795182fd5", CPU_TYPE_ALL_SSE, "sox.sse2.exe", 1442, "" },
	{"25585ca9e2e025d82d93341a9db8527eb0b4ce451dade607f9784a79ed30e050ced0824835d5467aa0bf0c6b8fe08612", CPU_TYPE_ALL_AVX, "sox.avx.exe",  1442, "" },
	{"5a4261e1b41a59d1a5bc92e1d2766422a67454d77e06ea29af392811b7b4704e0f3e494ab9cb6375ce9e39257867c5ed", CPU_TYPE_ALL_ALL, "speexdec.exe", 12, ""},
	{"a83628880da0b7519ec368a74a92da5a5099d8d46aa0583131f92d7321f47c9e16a1841b2a3fb8ffcca7205ef4b1bb0a", CPU_TYPE_ALL_ALL, "tta.exe", 21, ""},
	{"9e1ade2137ea5cee0ad4657971c314a372df3068594fbe4f77d45b9eb65fa7c69e55027b0df81b6fe072a220e9a8ba8a", CPU_TYPE_ALL_ALL, "valdec.exe", 100, "a"},
	{"8aed6610e574aaac0673d0801db987b6533a8a4ba468a328d2a3f457e18ac5cff4bd6aef7be40316cf55c6930995babf", CPU_TYPE_ALL_ALL, "verify.exe", 1010, ""},
	{"572b9448bf4a338ecb9727951fdfcc5a219cc69896695cc96b9f6b083690e339910e41558968264a38992e45f2be152c", CPU_TYPE_ALL_ALL, "wma2wav.exe", 20111001, ""},
	{"5ef85aa6c6521161e19fc9eadd30bac82c3d0eee2374fd6ac543022181f7846ec2198ebe8bc84667e9b92a4a85d07fbb", CPU_TYPE_ALL_ALL, "wupdate.exe", 20171002, ""},
	{"6021b938769b09d05617c7e91e0cb6cc5f9e40c50cb470455afc21d466b37183b8675822b8797cbf98950a5262ec07a6", CPU_TYPE_X86_GEN, "wvunpack.x86-i686.exe", 5010, ""},
	{"fa322e127679ac6b5e833e120b590480b3ffc7ffa875705c04ed698f155dc23b40c8922d2f5f78ad1cc9342306289141", CPU_TYPE_X86_SSX, "wvunpack.x86-sse2.exe", 5010, ""},
	{"d4ca3085aae70160beab778a46a27643f1415bd803ddfecb2791fe964e4bff49ac5a891ef1852f26e867c42ddd6f8806", CPU_TYPE_X64_SSE, "wvunpack.x64-sse2.exe", 5010, ""},
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

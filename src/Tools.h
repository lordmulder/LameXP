///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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
	{"4870d7abbc3995dc5ca82147ee33f28c3ee7dc1986fbb0131912effe10c1a209d3f983c5dffd963c31707a5ce39611b4", CPU_TYPE_X86_GEN, "aften.i386.exe", 8, ""},
	{"fc4e38b11a0f52b68cca79aa2d71c02180b70eb0e592923dee4f0ccf766f1006642369b2178f6a61d1c2506446cc442d", CPU_TYPE_X86_SSX, "aften.sse2.exe", 8, ""},
	{"b44b1c0e1a8f4633c9bc2e43d0a83ea5edc890e1cccc0054c812d0e64170e5292dc1dfd7430da61b73150831aa16e9c4", CPU_TYPE_X64_ALL, "aften.x64.exe",  8, ""},
	{"4558728999a05f12fe88566e8308cba0ada200814c2a1bfe2507f49faf8f3994b0d52a829148f5c6321d24faa2718439", CPU_TYPE_ALL_ALL, "avs2wav.exe", 13, ""},
	{"07834b1b8ecac2f3db79ad048005eca3a284cb759e15d399eb1a560a403122e20db5f08e60319759b8463aeef2410d02", CPU_TYPE_ALL_ALL, "dcaenc.exe", 20120419, ""},
	{"7c249f507b96967bedabdd7e631638807a7595ebff58eaaadf63530783d515eda9660bc2b1a0457fddae7e3eaef8a074", CPU_TYPE_ALL_ALL, "elevator.exe", UINT_MAX, ""},
	{"bbc262cfe9c48633e5f1780d30347d7663075cfd7bdc76347cce3b1191d62f788d9b91bc63dffae2f66d1759d5849e92", CPU_TYPE_ALL_ALL, "faad.exe", 27, ""},
	{"36f9b021a8136664fce4c556571270f9151ca11d191b31630b0cad7b28107be2175909b8d28c903cb1c17d260c38d5de", CPU_TYPE_X86_ALL, "flac.i386.exe", 131, ""},
	{"68e08fc34f981d183f243308791b5e158c28903a2052ca239bb9ad3d54d976c9ec7f0bb0b50c0aadfcefb4a1d0f63a18", CPU_TYPE_X64_ALL, "flac.x64.exe" , 131, ""},
	{"f63e97413e3b3ab23fd690fcd11d27ae104333d0201f34f2445a410b4a50dcf505a744ee7cabec66d9415c27261f175a", CPU_TYPE_ALL_ALL, "gpgv.exe", 1421, ""},
	{"c66992384b6388a7b68f2659624c150b33ff1cacb0c28c0ab2a3249eca4c31e204953f325403f78c077d1735b275ac12", CPU_TYPE_ALL_ALL, "keyring.gpg", UINT_MAX, ""},
	{"1dd342ca0aaaa5f84afbf657991d9829fbb4a2794fee36cd60014e1b9028120a8d3577b8a031482e207c4dd266c6b4c3", CPU_TYPE_X86_GEN, "lame.x86-i386.exe", 31002, "Alpha"},
	{"afb30a68e12ab2647f9694eb51d3b22d0f3cef95deda811c714556203dcc0adbe4b18c406c6af67b0d4af09f5033c77b", CPU_TYPE_X86_SSX, "lame.x86-sse2.exe", 31002, "Alpha"},
	{"84b66be0c17ff9b642fdb7f04f26c6621381a02b6297d17b36c9c6afa308b72f1dacb3831b41affe56db6b7fb5dbe9ae", CPU_TYPE_X64_NVX, "lame.x64-sse2.exe", 31002, "Alpha" },
	{"7c8ff93f3c3d89abd36770066c6f2b44f4aecc672e24406991f4035720a42aee2838b4625f9838e088ba05e3cda42d77", CPU_TYPE_X64_AVX, "lame.x64-avx.exe",  31002, "Alpha" },
	{"51af33a33bd9bde6ca6e9c612346b9c138584de59a92bc14a680729066fd849fd83fe668f79e2a06ffe117f39993d10a", CPU_TYPE_ALL_GEN, "mac.i386.exe", 416, ""},
	{"117e60cf4445cec0a777d2636091a13c50be5c6ed28afaa75ab09ab4576942f1a8584b36d3abac9db7595dfb0d90207e", CPU_TYPE_ALL_SSX, "mac.sse2.exe", 416, ""},
	{"d77bbbae80a1374458ee75f162f94352b5e2d781e44a53f30883efb21014f3154482c9bb2bc71b91afcb549d3f7908f7", CPU_TYPE_X86_ALL, "mediainfo.i386.exe", 788, ""},
	{"91b52e79ebaf29c1d12d1d45fc32ab7c36b8ea2b32e10bc1a96c88bc39c5866967635e7a868e97ac6af2def3b30bf866", CPU_TYPE_X64_ALL, "mediainfo.x64.exe",  788, ""},
	{"7e6346a057634ff07b2e1f427035324f7f02100cc996425990f87f71d767fce4c7b101588c7d944ba49cb2d7e51c9bdb", CPU_TYPE_ALL_ALL, "mpcdec.exe", 475, ""},
	{"e54c62b0f4e799e823e66b2e7f7dd630f212a86e8eba1bfacd80eb951347b03165027c6bb1a83b7e584a81a21e5bf049", CPU_TYPE_ALL_GEN, "mpg123.i386.exe", 1234, ""},
	{"b47b47e71311d99cdd544aaa8ee7bbcbb63dcbcf14229df6a50fdd02c8d1d37e923cb778d5699119bdbcc9805e7b16d0", CPU_TYPE_ALL_SSE, "mpg123.sse2.exe", 1234, "" },
	{"d35152f12df9ba03c47c88904632cc3d48cc5dd215d0760be25ff4d925e4a703e36d9db78e43e4bdb0a0dd2a19fe4f18", CPU_TYPE_ALL_AVX, "mpg123.avx.exe",  1234, "" },
	{"1bfc01052abc76d492541f26bdc347f14ece180092e7128438f8f0f58879a7ed2aa66186c23429811ff5587b997cbeb7", CPU_TYPE_X86_ALL, "nc.i386.exe", 112, "" },
	{"e7b19ec88fe025fd907a5ad85ed9c0b13cd33bd7cd04226d4f2944352f11d4026f558a30989f984e3e1d802552d05908", CPU_TYPE_X64_ALL, "nc.x64.exe", 112, "" },
	{"f1f2ea5c9e5539620b706e7af68e543bf7a731afb06ccce3815ab34dad64d697e4d6ffcd187a396619b8b52efe7edf88", CPU_TYPE_ALL_ALL, "oggdec.exe", 1101, ""},
	{"245181321625445ac42fce31d64bf03872e77e2d0dd3c19d6c17ca2771354f096a6040827dd6d00ffd7342c7dd26168e", CPU_TYPE_X86_GEN, "oggenc2.i386.exe", 288135603, "2015"},
	{"512b8efcd1003a0f67220a450d6ea4466194e8fd49fc090a69b15a858db11499acbf98f984530cd5d37b4b6abdd1c6d8", CPU_TYPE_X86_SSX, "oggenc2.sse2.exe", 288135603, "2015"},
	{"a07ef67cba5a00d335d07372baf76d4d0573b425afce71a19c1e04eaabbe3f55e60bdd40af5e428224c91df1823eda08", CPU_TYPE_X64_ALL, "oggenc2.x64.exe",  288135603, "2015"},
	{"a031b91fe6d532f6bb08bb2cab7e9dc48eae4bf81b1aaf9014e71897c2d171934ae4057c208ebacfc652dacd6e41b474", CPU_TYPE_ALL_GEN, "opusdec.i386.exe", 20160911, "v1.1.3"},
	{"ea8a668e82a5733e267a6a1fc2aec269a49b9a93c44d0760c77b5c44eb313d6e989b171399687c27f25e510fd26e861c", CPU_TYPE_ALL_SSE, "opusdec.sse2.exe", 20160911, "v1.1.3"},
	{"3aa54ba9135c231194a35ace0aa0d87267b243c2e8c99ecae025a9eb4704d14970840783bb1a27713aa606a6188a4fd8", CPU_TYPE_ALL_AVX, "opusdec.avx.exe",  20160911, "v1.1.3"},
	{"a2bb8fe42c7764eafe7e4cda660f781bdb3c0556426bb38b294aa8bf11c6da560018762908a15546d3b31a877b70f815", CPU_TYPE_ALL_GEN, "opusenc.i386.exe", 20160911, "v1.1.3"},
	{"0ae947277095be18d741ad4a328633a7d52cdd4112841a22313227cf57ee780a9e95fdc0a2af9014630e7dd9f01a1ea1", CPU_TYPE_ALL_SSE, "opusenc.sse2.exe", 20160911, "v1.1.3"},
	{"57376b6b037771260da5a46e23fd9de76294dc25c5a0078f1d46268b4df046cfa42b04cb370b7be7e6f3cfaed3679f85", CPU_TYPE_ALL_AVX, "opusenc.avx.exe",  20160911, "v1.1.3"},
	{"37745174e146fedbc62671ab86ffddd3edfc9aa0ebfcfd4bff3331a95b6dda50621478c63f7a0ddf9a365e916758b6d4", CPU_TYPE_X86_ALL, "refalac.i386.exe", 161, ""},
	{"b368f0490cefb80b6ff7eaaf8d6e29f253442375caf11a9405b5b21580ee3a9739fa3f406f69de5ef344fa038b1cbe72", CPU_TYPE_X64_ALL, "refalac.x64.exe",  161, ""},
	{"d041b60de6c5c6e77cbad84440db57bbeb021af59dd0f7bebd3ede047d9e2ddc2a0c14179472687ba91063743d23e337", CPU_TYPE_ALL_ALL, "shorten.exe", 361, ""},
	{"6e3f86cc464d84b0039139c9688e3097d0f42b794a5db10954d24fe77929585a0d0dba16cb677cc1b390392c39cdefad", CPU_TYPE_ALL_ALL, "sox.exe", 1442, ""},
	{"5a4261e1b41a59d1a5bc92e1d2766422a67454d77e06ea29af392811b7b4704e0f3e494ab9cb6375ce9e39257867c5ed", CPU_TYPE_ALL_ALL, "speexdec.exe", 12, ""},
	{"75d4c18dbb74e2dbf7342698428248d45cc4070d5f95da8831ef755e63dcd7ff9c3a760f289e8ef8b5c06b82548edbd8", CPU_TYPE_ALL_ALL, "tag.exe", 100, ""},
	{"a83628880da0b7519ec368a74a92da5a5099d8d46aa0583131f92d7321f47c9e16a1841b2a3fb8ffcca7205ef4b1bb0a", CPU_TYPE_ALL_ALL, "tta.exe", 21, ""},
	{"9e1ade2137ea5cee0ad4657971c314a372df3068594fbe4f77d45b9eb65fa7c69e55027b0df81b6fe072a220e9a8ba8a", CPU_TYPE_ALL_ALL, "valdec.exe", 100, "a"},
	{"509df39fdd7033b0f1af831304d0d6c08b74d5a48e2c038857a78b9dfaa4fb83c6b5c7ea202ba2270c0384607f2316ee", CPU_TYPE_ALL_ALL, "wget.exe", 1140, ""},
	{"572b9448bf4a338ecb9727951fdfcc5a219cc69896695cc96b9f6b083690e339910e41558968264a38992e45f2be152c", CPU_TYPE_ALL_ALL, "wma2wav.exe", 20111001, ""},
	{"b88a15dc27bf1aadf8702de5d4f9089599827977abde74901fb07cb94eaacd6df22b2ebe08517d4f461c59a1f75ecfd2", CPU_TYPE_ALL_ALL, "wupdate.exe", 20161008, ""},
	{"9c742cff64c726b6d9abafcf222c24a7e41d4fa0e44ea962917a91e09c15e52aebc859c1ea028c1c1806702850460e2c", CPU_TYPE_X86_ALL, "wvunpack.i386.exe", 4800, ""},
	{"70382b0a9431ec35f419126a5561252e7d5bc05a8a7912c49c54382a3642309372374977ef9186ccd9b733c64a3428bd", CPU_TYPE_X64_ALL, "wvunpack.x64.exe",  4800, ""},
	{NULL, NULL, NULL, NULL, NULL}
};

////////////////////////////////////////////////////////////
// AAC ENCODERS
////////////////////////////////////////////////////////////

typedef struct
{
	const char *const toolName;
	const char *const fileNames[8];
	const char *const checkArgs;
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
	{ "NeroAAC",   { "neroAacEnc.exe", "neroAacDec.exe", "neroAacTag.exe",             NULL }, "-help",   lamexp_toolver_neroaac(),   4,  10, "v?.?.?.?",   "Package\\s+version:\\s+(\\d)\\.(\\d)\\.(\\d)\\.(\\d)", "Nero\\s+AAC\\s+Encoder" },
	{ "FhgAacEnc", { "fhgaacenc.exe",  "enc_fhgaac.dll", "nsutil.dll", "libmp4v2.dll", NULL }, NULL,      lamexp_toolver_fhgaacenc(), 2,   0, "????-??-??", "fhgaacenc version (\\d+) by tmkk. Modified by Case (\\d+).",               NULL },
	{ "FdkAacEnc", { "fdkaac.exe",                                                     NULL }, "--help",  lamexp_toolver_fdkaacenc(), 3,  10, "v?.?.?",     "fdkaac\\s+(\\d)\\.(\\d)\\.(\\d)",                                          NULL },
	{ "QAAC",      { "qaac.exe",       "libsoxr.dll",   "libsoxconvolver.dll",         NULL }, "--check", lamexp_toolver_qaacenc(),   2, 100, "v?.??",      "qaac (\\d)\\.(\\d+)",                                                      NULL },
	{ "QAACx64",   { "qaac64.exe",     "libsoxr64.dll", "libsoxconvolver64.dll",       NULL }, "--check", lamexp_toolver_qaacenc(),   2, 100, "v?.??",      "qaac (\\d)\\.(\\d+)",                                                      NULL },
	{ NULL, { NULL }, NULL, 0, 0, 0, NULL, NULL, NULL }
};

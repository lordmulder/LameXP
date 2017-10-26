///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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
	{"7c249f507b96967bedabdd7e631638807a7595ebff58eaaadf63530783d515eda9660bc2b1a0457fddae7e3eaef8a074", CPU_TYPE_ALL_ALL, "elevator.exe", UINT_MAX, ""},
	{"5c4a5cdd708b5857bcb064558be81a2dfb16401e796b296f6eee7c63042acbeae12e1e2f1f3d0fd096eaf73201b54e10", CPU_TYPE_ALL_GEN, "faad.i686.exe", 27, ""},
	{"72447794cf411e1e4ce71facf5f60023f001d203894cf40185a0ee13e144e93d72ac99f2ed30a9168450ce5bf882f99f", CPU_TYPE_ALL_SSX, "faad.sse2.exe", 27, ""},
	{"735654150f5d123660aa8493d322a7181cfd6d6c3e584a53c94c27a5163659b6aab3dd84230f684f8cec3cb565223482", CPU_TYPE_X86_GEN, "flac.x86-i686.exe", 132, ""},
	{"2ee22f174131615cf342cb3031a4c93adfff0997d84eb1b6794c133c8f19d109c43e67a3f60638705c7afb93427a07f8", CPU_TYPE_X86_SSX, "flac.x86-sse2.exe", 132, ""},
	{"323ba94d1d3a8f82f4e5a6adf859d0f11d7b0c69fa0254e620724f879edd9d7017e3d2fa78ffe0a615539d8798606a46", CPU_TYPE_X64_NVX, "flac.x64-sse2.exe", 132, ""},
	{"db5c8b27f876f797d8e2c7dd6c585cff065174c2a006a1642be2b8339be7d17e49b806534e3c9f2de84e66e997c49df7", CPU_TYPE_X64_AVX, "flac.x64-avx.exe",  132, ""},
	{"c45bcc65f38ecb98adf2e20aafd95341f892a1cca50e402bd299d0b48b71257016f707ae83e4fd302accded26623794a", CPU_TYPE_ALL_ALL, "gpgv.exe", 1422, ""},
	{"c66992384b6388a7b68f2659624c150b33ff1cacb0c28c0ab2a3249eca4c31e204953f325403f78c077d1735b275ac12", CPU_TYPE_ALL_ALL, "keyring.gpg", UINT_MAX, ""},
	{"5e95b7d07d4ffc1ed9b2f3ccebd1a13caf6362dad39c594d94a1c15b80ba81b296cb8ce77487bf98c9650ce76f7a845b", CPU_TYPE_X86_GEN, "lame.x86-i686.exe", 31001, "Final"},
	{"cc5809e842fa0d9ceee2de7ebe3ad4d1cbfdc8b1a7aa91fa14ce3f690bd2280ce23648c178a93d41652189fb06c2600b", CPU_TYPE_X86_SSX, "lame.x86-sse2.exe", 31001, "Final"},
	{"578b230ce82a1a588d8ba21163acb3b78b4708fb3ef9f9b685a4fcb71a63f027b33aa3b12acd7815502ad9475d63fc3b", CPU_TYPE_X64_NVX, "lame.x64-sse2.exe", 31001, "Final" },
	{"789e28b1e7dfa43bf4f8a70213fdc77b480e386f7a36c258395240c93d643bae8cee9c8728376e9ec7360e169d5f59ec", CPU_TYPE_X64_AVX, "lame.x64-avx.exe",  31001, "Final" },
	{"83a04d781f35015b86a048cca6bde2f4e070d8f2ad60ab8925e6f74c63ce0f49e8cc1552814c222b110591746621b229", CPU_TYPE_ALL_GEN, "mac.i686.exe", 425, ""},
	{"0b3c7a9d9f5c6377abcffad09330ee5cbff848ff7d237c16799da383165ca5d512ca66443a5d33eb7b6bb98c025b7056", CPU_TYPE_ALL_SSE, "mac.sse2.exe", 425, ""},
	{"c10298c7ca1b240df05dc88e829a196cb9f37a10824f4c3287a81d21eb134fc0e2b150ed42d7d36d3c6558cba486559a", CPU_TYPE_ALL_AVX, "mac.avx.exe",  425, ""},
	{"83f6356b66bae82cdc1537ee4d7daa83caac7d5f4e954fce8d4bb9b390f28624cce3c61c6b2bed7834f6c3e60b7ed738", CPU_TYPE_X86_GEN, "mediainfo.i686.exe", 798, ""},
	{"1dccbb9af49c7eeab8a13942f5e60f813cff9d352922b7c057b4643b7f67bdb3ada7f0dfaba4cbdd23942de6cb264efb", CPU_TYPE_X86_SSX, "mediainfo.sse2.exe", 798, ""},
	{"a5b6c84252df69ccf1d617d3bcbb58b3ce6793b22fa4b3fa5cdccb0557ac0035dd8ad61916ca3a86599952eb8c037f77", CPU_TYPE_X64_ALL, "mediainfo.x64.exe",  798, ""},
	{"7e6346a057634ff07b2e1f427035324f7f02100cc996425990f87f71d767fce4c7b101588c7d944ba49cb2d7e51c9bdb", CPU_TYPE_ALL_ALL, "mpcdec.exe", 475, ""},
	{"f827c466a6ead57da7564c6602cea0cbd189d8dba69dd56292bcf71a0155492fdb951db4191c956604b3f516fc3b2d18", CPU_TYPE_ALL_GEN, "mpg123.i686.exe", 1257, ""},
	{"0940a21e0ab09c542025357048a922813d10deea8ee33cac4e14617e5ef660ce9047a99842c357d0392fdd744918c79c", CPU_TYPE_ALL_SSE, "mpg123.sse2.exe", 1257, "" },
	{"21346f9e0eff93e78a5422cc629c231322a5b593e365da43a3f5c19006efd3f743b1d6441f103a5476a95fbc712f2b87", CPU_TYPE_ALL_AVX, "mpg123.avx.exe",  1257, "" },
	{"dda88fb66a80c362dfa367d07265eee3dcf8ee959191fc7685163fdee694ece7d84000065de217942749b6859d33fa84", CPU_TYPE_ALL_ALL, "mcat.exe", 101, "" },
	{"f1f2ea5c9e5539620b706e7af68e543bf7a731afb06ccce3815ab34dad64d697e4d6ffcd187a396619b8b52efe7edf88", CPU_TYPE_ALL_ALL, "oggdec.exe", 1101, ""},
	{"245181321625445ac42fce31d64bf03872e77e2d0dd3c19d6c17ca2771354f096a6040827dd6d00ffd7342c7dd26168e", CPU_TYPE_X86_GEN, "oggenc2.i686.exe", 288135603, "2015"},
	{"512b8efcd1003a0f67220a450d6ea4466194e8fd49fc090a69b15a858db11499acbf98f984530cd5d37b4b6abdd1c6d8", CPU_TYPE_X86_SSX, "oggenc2.sse2.exe", 288135603, "2015"},
	{"a07ef67cba5a00d335d07372baf76d4d0573b425afce71a19c1e04eaabbe3f55e60bdd40af5e428224c91df1823eda08", CPU_TYPE_X64_ALL, "oggenc2.x64.exe",  288135603, "2015"},
	{"c0b5ef11dae2118d9c18f22637113dc9d2c6fc0196406112d5ba14c8f026c4b8f402f8208852ab9ad955458493bde5c7", CPU_TYPE_ALL_GEN, "opusdec.i686.exe", 20170726, "v1.2.1"},
	{"917958b6f21089e97c3d1bba47833865a90dc24cb77d92e3b04168e74dd7fb3fbcfdf9946f5fa7a584aa7aa44f28b909", CPU_TYPE_ALL_SSE, "opusdec.sse2.exe", 20170726, "v1.2.1"},
	{"36b26517450aa6d8435faecd259ba797b1b62149be530de8b753dd28040a1c07bb14e22070421a8840d3b48a87a72c80", CPU_TYPE_ALL_AVX, "opusdec.avx.exe",  20170726, "v1.2.1"},
	{"1e7ccc4961820c0366b3102f26970ce9b08d45cf80500e61108f6017d2fbbb92631bb9821a272fab1ccb4a68c8b21ab7", CPU_TYPE_ALL_GEN, "opusenc.i686.exe", 20170726, "v1.2.1"},
	{"e7902fbc3879d61cf1ceacb84ef8806ec0e00eff6b897a6d2cb3c6f7d3982f0167231f3d4a67eb7044254efdb32274a8", CPU_TYPE_ALL_SSE, "opusenc.sse2.exe", 20170726, "v1.2.1"},
	{"948b8a6b2e6a0f52fac4007cfcee33148fad3c33b79910d69462c0dc1ebfc8770b354c4559073f7193c7c94f3620bb3a", CPU_TYPE_ALL_AVX, "opusenc.avx.exe",  20170726, "v1.2.1"},
	{"37745174e146fedbc62671ab86ffddd3edfc9aa0ebfcfd4bff3331a95b6dda50621478c63f7a0ddf9a365e916758b6d4", CPU_TYPE_X86_ALL, "refalac.i686.exe", 161, ""},
	{"b368f0490cefb80b6ff7eaaf8d6e29f253442375caf11a9405b5b21580ee3a9739fa3f406f69de5ef344fa038b1cbe72", CPU_TYPE_X64_ALL, "refalac.x64.exe",  161, ""},
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
	{"9c742cff64c726b6d9abafcf222c24a7e41d4fa0e44ea962917a91e09c15e52aebc859c1ea028c1c1806702850460e2c", CPU_TYPE_X86_ALL, "wvunpack.i686.exe", 4800, ""},
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

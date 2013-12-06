///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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
	{"ce33d2d14cc40514886bfb9a1f14e1b1a8ec4a88b2bfebe521bed55fc34cb648f5c1c12be67aa5fc955da68b6c7e3722", CPU_TYPE_ALL_ALL, "gpgv.exe", 1415, ""},
	{"19c9dbe9089491c1f59ae48016d95d4336c4d3743577db4e782d8b59eca3b2bda6ed8f92f9004f88f434935b79e4974b", CPU_TYPE_ALL_ALL, "gpgv.gpg", UINT_MAX, ""},
	{"53cfab3896a47d48f523315f475fa07856d468ad1aefcc8cce19c18cdf509e2f92840dab92a442995df36d941cb7a6ca", CPU_TYPE_ALL_GEN, "lame.i386.exe", 3995, "Final"},
	{"9511e7ef2ad10de05386eedf7f14d637edab894a53dacd2f8f15c6f8ed582f12c25fb5bf88438e62c46b8eb92e7634b2", CPU_TYPE_ALL_SSE, "lame.sse2.exe", 3995, "Final"},
	{"fdbeb978025b9a5345300f37bb56446c31c7db594cf29407afdcc9ce20f4a5cec6eb8c03962c247d4f45b83c465ac705", CPU_TYPE_ALL_ALL, "mac.exe", 412, ""},
	{"f2bbdda1d0a66aae7f2e81bbc95aa7b45d52476e3ef97cf9e4e5066590428782c1d3661a93073deecc1287f1669b04de", CPU_TYPE_X86_ALL, "mediainfo.i386.exe", 765, ""},
	{"eb71c459ce5f6c61fe549cd45df7b7738f93c16c353cec78632ad422d085d69594d10fdcbce7127c354ac250bedbbebb", CPU_TYPE_X64_ALL, "mediainfo.x64.exe",  765, ""},
	{"7e6346a057634ff07b2e1f427035324f7f02100cc996425990f87f71d767fce4c7b101588c7d944ba49cb2d7e51c9bdb", CPU_TYPE_ALL_ALL, "mpcdec.exe", 475, ""},
	{"8f844060ed60b4d3fb8c2f2f28e13304a69fa97cde3859ee5c62fef46aea6f500e4dc74114ded901ee9231bbb5d2af64", CPU_TYPE_ALL_ALL, "mpg123.exe", 1160, ""},
	{"75c39861ac82d7fc8392fbddff9f5b785e7abee29f8c1843706a9b225966e05ef4be2a160f98f389beaf5238c5de121c", CPU_TYPE_ALL_ALL, "oggdec.exe", UINT_MAX, ""},
	{"8b68461f38410421be30cc895e94e63184daa6f2cb20eb110b66b376b48141838a09bc920efeb1c49de79dd0770ce41b", CPU_TYPE_X86_GEN, "oggenc2.i386.exe", 287603, "Beta"},
	{"20648f83cc637cada481143d48c437ced8423e9a0aae01dbce860cd97fb1ce4000e314f3a5395d1eafd8e154a8e74d08", CPU_TYPE_X86_SSE, "oggenc2.sse2.exe", 287603, "Beta"},
	{"53766251d69e0695ae7d0cbc4f19aafdce90bfebde0815e2216a5b34955f151271d97d23a00be5ce280a502b718753cc", CPU_TYPE_X64_ALL, "oggenc2.x64.exe",  287603, "Beta"},
	{"d218922000861a855706bc2c7f1c74c1cfcb814c4a9ed9460af32edb61514a577f261c64b4ed402ddf174cc6824a6507", CPU_TYPE_ALL_ALL, "opusdec.exe", 20130722, "v1.1-beta"},
	{"03265bef954f8e6abe6d33c4a44be8d32ebe2d7c32288b5ca7290ac3a2e0e9ef33eb54fe510714faa4a4a3ff26cb833c", CPU_TYPE_ALL_GEN, "opusenc.i386.exe", 20130722, "v1.1-beta"},
	{"06c2b2583f86ace1b82288b6525d27c66536b9d4cb11e46605101ea528b0d9a29338d715bdbcb971820138c05b767957", CPU_TYPE_ALL_SSE, "opusenc.sse2.exe", 20130722, "v1.1-beta"},
	{"bdfa8dec142b6327a33af6bb314d7beb924588d1b73f2ef3f46b31fa6046fe2f4e64ca78b025b7eb9290a78320e2aa57", CPU_TYPE_ALL_ALL, "refalac.exe", 56, ""},
	{"d041b60de6c5c6e77cbad84440db57bbeb021af59dd0f7bebd3ede047d9e2ddc2a0c14179472687ba91063743d23e337", CPU_TYPE_ALL_ALL, "shorten.exe", 361, ""},
	{"cf988bfbb53e77a1dcaefbd5c08789abb4d67cc210723f1f8ba7850f17d34ebb7d0c426b67b963e7d2290a2744865244", CPU_TYPE_ALL_ALL, "sox.exe", 1441, ""},
	{"5a4261e1b41a59d1a5bc92e1d2766422a67454d77e06ea29af392811b7b4704e0f3e494ab9cb6375ce9e39257867c5ed", CPU_TYPE_ALL_ALL, "speexdec.exe", 12, ""},
	{"75d4c18dbb74e2dbf7342698428248d45cc4070d5f95da8831ef755e63dcd7ff9c3a760f289e8ef8b5c06b82548edbd8", CPU_TYPE_ALL_ALL, "tag.exe", 100, ""},
	{"a83628880da0b7519ec368a74a92da5a5099d8d46aa0583131f92d7321f47c9e16a1841b2a3fb8ffcca7205ef4b1bb0a", CPU_TYPE_ALL_ALL, "tta.exe", 21, ""},
	{"9e1ade2137ea5cee0ad4657971c314a372df3068594fbe4f77d45b9eb65fa7c69e55027b0df81b6fe072a220e9a8ba8a", CPU_TYPE_ALL_ALL, "valdec.exe", 100, "a"},
	{"509df39fdd7033b0f1af831304d0d6c08b74d5a48e2c038857a78b9dfaa4fb83c6b5c7ea202ba2270c0384607f2316ee", CPU_TYPE_ALL_ALL, "wget.exe", 1140, ""},
	{"572b9448bf4a338ecb9727951fdfcc5a219cc69896695cc96b9f6b083690e339910e41558968264a38992e45f2be152c", CPU_TYPE_ALL_ALL, "wma2wav.exe", 20111001, ""},
	{"9c1846718eb7a738f1cac4805a23ba4c52682f7b8a00d4f1a72edf770dda1f04b9d6f74d779a28bd0ef2ed8341f9ef9f", CPU_TYPE_ALL_ALL, "wupdate.exe", 20131016, ""},
	{"b0a564e842f2cda6b67fdd09aaa66294c775bb3eb419debf311fe54eb76ea468e99917dc3ad8ed4734fcdb59034a13cb", CPU_TYPE_ALL_ALL, "wvunpack.exe", 4601, ""},
	{NULL, NULL, NULL, NULL, NULL}
};

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
	{"4870d7abbc3995dc5ca82147ee33f28c3ee7dc1986fbb0131912effe10c1a209d3f983c5dffd963c31707a5ce39611b4", CPU_TYPE_X86_GEN, "aften.i386.exe", 8},
	{"fc4e38b11a0f52b68cca79aa2d71c02180b70eb0e592923dee4f0ccf766f1006642369b2178f6a61d1c2506446cc442d", CPU_TYPE_X86_SSE, "aften.sse2.exe", 8},
	{"d053d2a87f31a6a8107d9963b0b5f76c4c89083ee315064b86b3b88577659add167cd408bebfd188937b9314ad49ee41", CPU_TYPE_X64_ALL, "aften.x64.exe",  8},
	{"4558728999a05f12fe88566e8308cba0ada200814c2a1bfe2507f49faf8f3994b0d52a829148f5c6321d24faa2718439", CPU_TYPE_ALL_ALL, "avs2wav.exe", 13},
	{"07834b1b8ecac2f3db79ad048005eca3a284cb759e15d399eb1a560a403122e20db5f08e60319759b8463aeef2410d02", CPU_TYPE_ALL_ALL, "dcaenc.exe", 20120419},
	{"7c249f507b96967bedabdd7e631638807a7595ebff58eaaadf63530783d515eda9660bc2b1a0457fddae7e3eaef8a074", CPU_TYPE_ALL_ALL, "elevator.exe", UINT_MAX},
	{"bbc262cfe9c48633e5f1780d30347d7663075cfd7bdc76347cce3b1191d62f788d9b91bc63dffae2f66d1759d5849e92", CPU_TYPE_ALL_ALL, "faad.exe", 27},
	{"4f3a905b5b77ea49a7b302ab0a028ab115270e1b285caafbc35013ffc1de64bdec4488b561ef497c4befae952759a9dd", CPU_TYPE_ALL_ALL, "flac.exe", 121},
	{"52e213df29da215c59e82cd4fefb290aa2842280383fd59ffaa06cb2c58f1081b0dbd7b6e57f69fe3a872b6e7dd0c1cf", CPU_TYPE_ALL_ALL, "gpgv.exe", 1413},
	{"19c9dbe9089491c1f59ae48016d95d4336c4d3743577db4e782d8b59eca3b2bda6ed8f92f9004f88f434935b79e4974b", CPU_TYPE_ALL_ALL, "gpgv.gpg", UINT_MAX},
	{"53cfab3896a47d48f523315f475fa07856d468ad1aefcc8cce19c18cdf509e2f92840dab92a442995df36d941cb7a6ca", CPU_TYPE_ALL_GEN, "lame.i386.exe", 3995},
	{"9511e7ef2ad10de05386eedf7f14d637edab894a53dacd2f8f15c6f8ed582f12c25fb5bf88438e62c46b8eb92e7634b2", CPU_TYPE_ALL_SSE, "lame.sse2.exe", 3995},
	{"c844c21fc734465bb7e40f05b892d1ca93ca4746c493501c07c12a2413d66866a5f9a8091efe7de88bd4da9eb0939f66", CPU_TYPE_ALL_ALL, "mac.exe", 411},
	{"aeb7917151bf9ce4e30dbff78fcdd4b050b55a3721d8e0ce01092460dd1ed8f8f6e881fbab47953c0d692fcad4cbb2c4", CPU_TYPE_X86_ALL, "mediainfo.i386.exe", 761},
	{"acc4875b240979b0fad0cca0ad951b7eb10418c6a7db208ed25c3c9868fe9cd1cca1f10b12b72c026bfbd0837b63dcb8", CPU_TYPE_X64_ALL, "mediainfo.x64.exe",  761},
	{"7e6346a057634ff07b2e1f427035324f7f02100cc996425990f87f71d767fce4c7b101588c7d944ba49cb2d7e51c9bdb", CPU_TYPE_ALL_ALL, "mpcdec.exe", 475},
	{"bcd609cddf15c940035de4db774f0bfc5066d4633a8353becca72674a2b4f28b158d43faac9655ee15c50181079f1caa", CPU_TYPE_ALL_ALL, "mpg123.exe", 1144},
	{"75c39861ac82d7fc8392fbddff9f5b785e7abee29f8c1843706a9b225966e05ef4be2a160f98f389beaf5238c5de121c", CPU_TYPE_ALL_ALL, "oggdec.exe", UINT_MAX},
	{"8b68461f38410421be30cc895e94e63184daa6f2cb20eb110b66b376b48141838a09bc920efeb1c49de79dd0770ce41b", CPU_TYPE_X86_GEN, "oggenc2.i386.exe", 287603},
	{"20648f83cc637cada481143d48c437ced8423e9a0aae01dbce860cd97fb1ce4000e314f3a5395d1eafd8e154a8e74d08", CPU_TYPE_X86_SSE, "oggenc2.sse2.exe", 287603},
	{"e1da48055a57bae41d6a1a0dc08b86831c121e85c07aa60aae4196997b166a08cfb7265d9f0f289f445ad73bce28d81f", CPU_TYPE_X64_ALL, "oggenc2.x64.exe",  287603},
	{"d4c081ca0807b9b0e8f01c7a68f2cf2985308f614a045193ced0b51ead0b06628642f607679b7e9e48c9b7b59a8fc651", CPU_TYPE_ALL_ALL, "opusdec.exe", 20130124},
	{"915eb675678e24fd7351032e60b5e49f3457b37e748d185f37a212625f4c421fb3369adf4ef504dd6aaa9f1bed702ba7", CPU_TYPE_ALL_ALL, "opusenc.exe", 20130124},
	{"bdfa8dec142b6327a33af6bb314d7beb924588d1b73f2ef3f46b31fa6046fe2f4e64ca78b025b7eb9290a78320e2aa57", CPU_TYPE_ALL_ALL, "refalac.exe", 56},
	{"d041b60de6c5c6e77cbad84440db57bbeb021af59dd0f7bebd3ede047d9e2ddc2a0c14179472687ba91063743d23e337", CPU_TYPE_ALL_ALL, "shorten.exe", 361},
	{"81633b4808ff13e25249aac038ed97570aaeea2bbffacf0f3905514064d1b82d141a6b04f63f1a913e4d09b2d892aea0", CPU_TYPE_ALL_ALL, "sox.exe", 1440},
	{"5a4261e1b41a59d1a5bc92e1d2766422a67454d77e06ea29af392811b7b4704e0f3e494ab9cb6375ce9e39257867c5ed", CPU_TYPE_ALL_ALL, "speexdec.exe", 12},
	{"a83628880da0b7519ec368a74a92da5a5099d8d46aa0583131f92d7321f47c9e16a1841b2a3fb8ffcca7205ef4b1bb0a", CPU_TYPE_ALL_ALL, "tta.exe", 21},
	{"c07bbd790e2a9d8b694b44e33d47ee86740306049c8ca6dc0d7ecc73e6b18ad6cd77ad1d5865e4b4928cd8260be37e1d", CPU_TYPE_ALL_ALL, "valdec.exe", 31},
	{"c472d443846fec57b2568c6e690736703497abc22245f08fb0e609189fd1c7aa2d670a7f42d5e05de6d7371de39dcf1a", CPU_TYPE_ALL_ALL, "wget.exe", 1114},
	{"572b9448bf4a338ecb9727951fdfcc5a219cc69896695cc96b9f6b083690e339910e41558968264a38992e45f2be152c", CPU_TYPE_ALL_ALL, "wma2wav.exe", 20111001},
	{"65fd97f357bd779d492012940d6eab1d9b7618b088430a62428d7b184ccbe5f683caa2aac67146c1d6cb07dbff997d79", CPU_TYPE_ALL_ALL, "wupdate.exe", 20120310},
	{"b0a564e842f2cda6b67fdd09aaa66294c775bb3eb419debf311fe54eb76ea468e99917dc3ad8ed4734fcdb59034a13cb", CPU_TYPE_ALL_ALL, "wvunpack.exe", 4601},
	{NULL, NULL, NULL, NULL}
};

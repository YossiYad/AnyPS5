#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
void* APS5_VABI sceCesUcsProfileInitEucJpCp51932(void*);
void* APS5_VABI sceCesUcsProfileInitSJis1997Cp932(void*);
const std::uint8_t* APS5_VABI sceCesRefersUcsProfileCp1252();
int APS5_VABI sceCesEucJpToUcs2(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*);
int APS5_VABI sceCesEucJpToUtf16(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesEucJpToUtf16be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesEucJpToUtf16le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesEucJpToUtf32(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesEucJpToUtf32be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesEucJpToUtf32le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesMbcToUcs2(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*);
int APS5_VABI sceCesMbcToUtf8(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcToUtf16be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcToUtf32le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesSbcToUcs2(const std::uint8_t*, std::uint8_t, std::uint16_t*);
int APS5_VABI sceCesSbcToUtf16le(const std::uint8_t*, std::uint8_t, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesSbcToUtf32be(const std::uint8_t*, std::uint8_t, std::uint32_t*);
int APS5_VABI sceCesUcs2ToEucJp(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUcs2ToMbc(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUcs2ToSbc(std::uint16_t, const std::uint8_t*, std::uint8_t*);
int APS5_VABI sceCesUtf16beToEucJp(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16leToMbc(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16ToSbc(const std::uint16_t*, std::uint32_t, std::uint32_t*, const std::uint8_t*, std::uint8_t*);
int APS5_VABI sceCesUtf32ToEucJp(std::uint32_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32leToEucJp(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32beToMbc(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32leToSbc(const std::uint32_t*, const std::uint8_t*, std::uint8_t*);
int APS5_VABI sceCesUtf8ToEucJp(const std::uint8_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf8ToMbc(const std::uint8_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
}

namespace {

constexpr int InvalidParameter = static_cast<int>(0x805C0001u);
constexpr int InvalidSource = static_cast<int>(0x805C0010u);
constexpr int SourceEnd = static_cast<int>(0x805C0011u);
constexpr int InvalidEncode = static_cast<int>(0x805C0014u);
constexpr int Unassigned = static_cast<int>(0x805C0020u);
constexpr int InvalidDestination = static_cast<int>(0x805C0030u);
constexpr int DestinationEnd = static_cast<int>(0x805C0031u);

void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "%s\n", what);
    std::abort();
}

alignas(8) std::uint8_t eucSheet[256];
alignas(8) std::uint8_t sjisSheet[256];
const void* euc = nullptr;
const void* sjis = nullptr;
const std::uint8_t* cp1252 = nullptr;

void CheckMultibyteToUnicode() {
    const std::uint8_t a[] = {0xA4, 0xA2};
    std::uint32_t used = 0;
    std::uint32_t produced = 0;
    std::uint16_t utf16[2] = {};
    Require(sceCesEucJpToUtf16be(euc, a, 2, &used, utf16, 2, &produced) == 0 && used == 2 && produced == 1, "EUC-JP to UTF-16BE failed");
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(utf16);
    Require(bytes[0] == 0x30 && bytes[1] == 0x42, "EUC-JP to UTF-16BE is not big-endian U+3042");
    Require(sceCesEucJpToUtf16le(euc, a, 2, &used, utf16, 2, &produced) == 0 && bytes[0] == 0x42 && bytes[1] == 0x30, "EUC-JP to UTF-16LE is not little-endian U+3042");
    Require(sceCesEucJpToUtf16(euc, a, 2, &used, utf16, 2, &produced) == 0 && utf16[0] == 0x3042, "EUC-JP to native UTF-16 differs");
    std::uint32_t utf32 = 0;
    Require(sceCesEucJpToUtf32(euc, a, 2, &used, &utf32) == 0 && utf32 == 0x3042, "EUC-JP to native UTF-32 differs");
    Require(sceCesEucJpToUtf32be(euc, a, 2, &used, &utf32) == 0 && utf32 == 0x42300000u, "EUC-JP to UTF-32BE is not big-endian");
    const std::uint8_t supplementary[] = {0x8F, 0xB0, 0xA1};
    Require(sceCesEucJpToUtf32le(euc, supplementary, 3, &used, &utf32) == 0 && utf32 == 0x4E02 && used == 3, "JIS X 0212 row 16 cell 1 is not U+4E02");
    const std::uint8_t kana[] = {0x8E, 0xB1};
    Require(sceCesEucJpToUtf32le(euc, kana, 2, &used, &utf32) == 0 && utf32 == 0xFF71, "half-width katakana is not U+FF71");

    Require(sceCesEucJpToUtf16le(sjis, a, 2, &used, utf16, 2, &produced) == InvalidParameter, "a Shift-JIS profile was taken as EUC-JP");
    Require(sceCesEucJpToUtf16le(euc, nullptr, 2, &used, utf16, 2, &produced) == InvalidSource, "a null EUC-JP source was taken");
    Require(sceCesEucJpToUtf16le(euc, a, 0, &used, utf16, 2, &produced) == SourceEnd, "an empty EUC-JP source was taken");
    Require(sceCesEucJpToUtf16le(euc, a, 1, &used, utf16, 2, &produced) == SourceEnd, "half a two-byte character was taken");
    Require(sceCesEucJpToUtf16le(euc, a, 2, &used, nullptr, 2, &produced) == InvalidDestination, "a null UTF-16 target was taken");
    Require(sceCesEucJpToUtf16le(euc, a, 2, &used, utf16, 0, &produced) == DestinationEnd, "a full UTF-16 target was written");

    const std::uint8_t sjisA[] = {0x82, 0xA0};
    std::uint16_t ucs2 = 0;
    Require(sceCesMbcToUcs2(sjis, sjisA, 2, &used, &ucs2) == 0 && ucs2 == 0x3042 && used == 2, "Shift-JIS through Mbc is not U+3042");
    Require(sceCesMbcToUtf32le(euc, a, 2, &used, &utf32) == 0 && utf32 == 0x3042, "EUC-JP through Mbc is not U+3042");
    std::uint8_t utf8[4] = {};
    Require(sceCesMbcToUtf8(sjis, sjisA, 2, &used, utf8, 4, &produced) == 0 && produced == 3 && utf8[0] == 0xE3 && utf8[1] == 0x81 && utf8[2] == 0x82, "Shift-JIS to UTF-8 differs");
    Require(sceCesMbcToUtf8(sjis, sjisA, 2, &used, utf8, 2, &produced) == DestinationEnd, "a short UTF-8 target was written");
    const std::uint8_t euro = 0x80;
    Require(sceCesMbcToUtf16be(cp1252, &euro, 1, &used, utf16, 1, &produced) == 0 && bytes[0] == 0x20 && bytes[1] == 0xAC, "CP1252 through Mbc is not U+20AC");
    Require(sceCesMbcToUcs2(reinterpret_cast<const void*>(&euro), sjisA, 2, &used, &ucs2) == InvalidParameter, "an unknown profile was taken by Mbc");

    Require(sceCesSbcToUcs2(cp1252, 0x80, &ucs2) == 0 && ucs2 == 0x20AC, "CP1252 0x80 is not U+20AC");
    Require(sceCesSbcToUtf16le(cp1252, 0x41, utf16, 1, &produced) == 0 && utf16[0] == 0x41 && produced == 1, "CP1252 0x41 is not U+0041");
    Require(sceCesSbcToUtf32be(cp1252, 0x80, &utf32) == 0 && utf32 == 0xAC200000u, "CP1252 to UTF-32BE is not big-endian");
    Require(sceCesSbcToUcs2(static_cast<const std::uint8_t*>(euc), 0x41, &ucs2) == InvalidParameter, "an EUC-JP profile was taken as single-byte");
    Require(sceCesSbcToUcs2(cp1252, 0x41, nullptr) == InvalidDestination, "a null UCS-2 target was taken");
}

void CheckUnicodeToMultibyte() {
    std::uint8_t out[4] = {};
    std::uint32_t used = 0;
    std::uint32_t produced = 0;
    Require(sceCesUcs2ToEucJp(0x3042, euc, out, 4, &produced) == 0 && produced == 2 && out[0] == 0xA4 && out[1] == 0xA2, "U+3042 is not EUC-JP A4 A2");
    Require(sceCesUtf32ToEucJp(0x4E02, euc, out, 4, &produced) == 0 && produced == 3 && out[0] == 0x8F && out[1] == 0xB0 && out[2] == 0xA1, "U+4E02 is not EUC-JP 8F B0 A1");
    Require(sceCesUcs2ToMbc(0x3042, sjis, out, 4, &produced) == 0 && produced == 2 && out[0] == 0x82 && out[1] == 0xA0, "U+3042 is not Shift-JIS 82 A0");
    std::uint8_t sbc = 0;
    Require(sceCesUcs2ToSbc(0x20AC, cp1252, &sbc) == 0 && sbc == 0x80, "U+20AC is not CP1252 0x80");
    Require(sceCesUcs2ToSbc(0x3042, cp1252, &sbc) == Unassigned, "U+3042 was given a CP1252 byte");
    Require(sceCesUcs2ToEucJp(0x3042, euc, out, 1, &produced) == DestinationEnd, "a short EUC-JP target was written");
    Require(sceCesUcs2ToEucJp(0xD800, euc, out, 4, &produced) == InvalidEncode, "a surrogate UCS-2 value was encoded");
    Require(sceCesUcs2ToEucJp(0x3042, sjis, out, 4, &produced) == InvalidParameter, "a Shift-JIS profile was taken as EUC-JP");
    Require(sceCesUcs2ToEucJp(0x3042, euc, nullptr, 4, &produced) == InvalidDestination, "a null EUC-JP target was taken");
    Require(sceCesUtf32ToEucJp(0x110000, euc, out, 4, &produced) == InvalidEncode, "a value past U+10FFFF was encoded");
    Require(sceCesUtf32ToEucJp(0x1F600, euc, out, 4, &produced) == Unassigned, "U+1F600 was given an EUC-JP sequence");

    const std::uint8_t beA[] = {0x30, 0x42};
    std::uint16_t utf16be[1];
    std::memcpy(utf16be, beA, sizeof(beA));
    Require(sceCesUtf16beToEucJp(utf16be, 1, &used, euc, out, 4, &produced) == 0 && used == 1 && out[0] == 0xA4, "UTF-16BE U+3042 is not EUC-JP A4 A2");
    const std::uint16_t lone[] = {0xDC00};
    const std::uint16_t high[] = {0xD83D};
    const std::uint16_t pair[] = {0xD83D, 0xDE00};
    Require(sceCesUtf16leToMbc(lone, 1, &used, sjis, out, 4, &produced) == InvalidEncode, "a lone low surrogate was decoded");
    Require(sceCesUtf16leToMbc(high, 1, &used, sjis, out, 4, &produced) == SourceEnd, "a high surrogate at the end was decoded");
    Require(sceCesUtf16leToMbc(pair, 2, &used, sjis, out, 4, &produced) == Unassigned, "U+1F600 was given a Shift-JIS sequence");
    Require(sceCesUtf16leToMbc(pair, 0, &used, sjis, out, 4, &produced) == SourceEnd, "an empty UTF-16 source was taken");
    Require(sceCesUtf16leToMbc(nullptr, 1, &used, sjis, out, 4, &produced) == InvalidSource, "a null UTF-16 source was taken");
    const std::uint16_t euroUnit[] = {0x20AC};
    Require(sceCesUtf16ToSbc(euroUnit, 1, &used, cp1252, &sbc) == 0 && sbc == 0x80 && used == 1, "native UTF-16 U+20AC is not CP1252 0x80");

    const std::uint32_t leA = 0x3042;
    Require(sceCesUtf32leToEucJp(&leA, euc, out, 4, &produced) == 0 && out[0] == 0xA4 && out[1] == 0xA2, "UTF-32LE U+3042 is not EUC-JP A4 A2");
    const std::uint8_t beBytes[] = {0x00, 0x00, 0x30, 0x42};
    std::uint32_t beA32 = 0;
    std::memcpy(&beA32, beBytes, sizeof(beA32));
    Require(sceCesUtf32beToMbc(&beA32, sjis, out, 4, &produced) == 0 && out[0] == 0x82 && out[1] == 0xA0, "UTF-32BE U+3042 is not Shift-JIS 82 A0");
    Require(sceCesUtf32beToMbc(nullptr, sjis, out, 4, &produced) == InvalidSource, "a null UTF-32 source was taken");
    const std::uint32_t leEuro = 0x20AC;
    Require(sceCesUtf32leToSbc(&leEuro, cp1252, &sbc) == 0 && sbc == 0x80, "UTF-32LE U+20AC is not CP1252 0x80");

    const std::uint8_t utf8A[] = {0xE3, 0x81, 0x82};
    Require(sceCesUtf8ToEucJp(utf8A, 3, &used, euc, out, 4, &produced) == 0 && used == 3 && produced == 2 && out[0] == 0xA4, "UTF-8 U+3042 is not EUC-JP A4 A2");
    Require(sceCesUtf8ToMbc(utf8A, 2, &used, sjis, out, 4, &produced) == SourceEnd, "a cut UTF-8 sequence was decoded");
    Require(sceCesUtf8ToMbc(utf8A, 3, &used, cp1252, out, 4, &produced) == Unassigned, "U+3042 was given a CP1252 byte through Mbc");
}

void CheckWholeEucJpTable() {
    std::uint32_t decoded = 0;
    for (std::uint32_t lead = 0xA1; lead <= 0xFE; ++lead) {
        for (std::uint32_t trail = 0xA1; trail <= 0xFE; ++trail) {
            const std::uint8_t sequence[] = {static_cast<std::uint8_t>(lead), static_cast<std::uint8_t>(trail)};
            std::uint32_t used = 0;
            std::uint16_t ucs2 = 0;
            const int reference = sceCesEucJpToUcs2(euc, sequence, 2, &used, &ucs2);
            std::uint16_t utf16 = 0;
            std::uint32_t produced = 0;
            const int result = sceCesEucJpToUtf16le(euc, sequence, 2, &used, &utf16, 1, &produced);
            Require(result == reference, "EUC-JP to UTF-16LE and to UCS-2 disagree on success");
            if (result != 0) continue;
            Require(utf16 == ucs2, "EUC-JP to UTF-16LE and to UCS-2 disagree on the character");
            std::uint8_t encoded[4] = {};
            Require(sceCesUcs2ToEucJp(ucs2, euc, encoded, 4, &produced) == 0, "a decoded character did not encode back");
            std::uint32_t again = 0;
            Require(sceCesEucJpToUtf32le(euc, encoded, produced, &used, &again) == 0 && again == ucs2 && used == produced, "a re-encoded character decodes differently");
            ++decoded;
        }
    }
    Require(decoded > 7000, "the EUC-JP table decoded too few characters");
}

}

int main() {
    euc = sceCesUcsProfileInitEucJpCp51932(eucSheet);
    sjis = sceCesUcsProfileInitSJis1997Cp932(sjisSheet);
    cp1252 = sceCesRefersUcsProfileCp1252();
    Require(euc && sjis && cp1252, "a profile could not be made");
    CheckMultibyteToUnicode();
    CheckUnicodeToMultibyte();
    CheckWholeEucJpTable();
    std::puts("CES single-character conversion tests passed");
    return 0;
}

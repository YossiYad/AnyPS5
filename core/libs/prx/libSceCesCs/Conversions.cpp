#include <bit>
#include <cstdint>
#include <cstring>
#include "prx/libc/include/general/VabiMacros.hpp"
#include "Codec.hpp"

namespace {

using namespace Ces;

constexpr bool NATIVE_BIG_ENDIAN = std::endian::native == std::endian::big;

bool isMbc(const void* profile) {
    return isEucJp(profile) || isSjis(profile) || isSbc(profile);
}

template<bool BigEndian>
struct Utf16Target {
    uint16_t* target;
    uint32_t capacity;
    uint32_t* targetLength;
    const void* buffer() const { return target; }
    int write(uint32_t code) const {
        const uint32_t units = code < 0x10000 ? 1 : 2;
        if (capacity < units) return CES_ERROR_DST_BUFFER_END;
        if (units == 1) storeUnit<uint16_t, BigEndian>(target, code);
        else {
            storeUnit<uint16_t, BigEndian>(target, 0xD800 | (code - 0x10000) >> 10);
            storeUnit<uint16_t, BigEndian>(target + 1, 0xDC00 | ((code - 0x10000) & 0x3FF));
        }
        if (targetLength) *targetLength = units;
        return 0;
    }
};

template<bool BigEndian>
struct Utf32Target {
    uint32_t* target;
    const void* buffer() const { return target; }
    int write(uint32_t code) const {
        storeUnit<uint32_t, BigEndian>(target, code);
        return 0;
    }
};

struct Ucs2Target {
    uint16_t* target;
    const void* buffer() const { return target; }
    int write(uint32_t code) const {
        if (code > 0xFFFF) return CES_ERROR_UNASSIGNED_CODE;
        *target = static_cast<uint16_t>(code);
        return 0;
    }
};

struct Utf8Target {
    uint8_t* target;
    uint32_t capacity;
    uint32_t* targetLength;
    const void* buffer() const { return target; }
    int write(uint32_t code) const {
        uint8_t encoded[3];
        const uint32_t length = encodeUtf8(code, encoded);
        if (capacity < length) return CES_ERROR_DST_BUFFER_END;
        std::memcpy(target, encoded, length);
        if (targetLength) *targetLength = length;
        return 0;
    }
};

template<typename Target>
int fromMultibyte(bool known, const void* profile, const uint8_t* source, uint32_t sourceMax, uint32_t* sourceLength, const Target& target) {
    if (!known) return CES_ERROR_INVALID_PARAMETER;
    if (!source) return CES_ERROR_INVALID_SRC_BUFFER;
    if (sourceMax == 0) return CES_ERROR_SRC_BUFFER_END;
    if (!target.buffer()) return CES_ERROR_INVALID_DST_BUFFER;
    uint32_t length = 0;
    uint32_t code = 0;
    if (const int result = decodeMbcs(profile, source, sourceMax, length, code); result != 0) return result;
    if (const int result = target.write(code); result != 0) return result;
    if (sourceLength) *sourceLength = length;
    return 0;
}

template<typename Target>
int fromSingleByte(const uint8_t* profile, uint8_t sbc, const Target& target) {
    if (!isSbc(profile)) return CES_ERROR_INVALID_PARAMETER;
    if (!target.buffer()) return CES_ERROR_INVALID_DST_BUFFER;
    return target.write(sbcToUnicode(profile, sbc));
}

struct Decoded {
    int result;
    uint32_t code;
    uint32_t length;
};

template<bool BigEndian>
Decoded decodeUtf16(const uint16_t* source, uint32_t available) {
    uint32_t code = loadUnit<uint16_t, BigEndian>(source);
    if (code >= 0xDC00 && code <= 0xDFFF) return {CES_ERROR_INVALID_ENCODE, 0, 0};
    if (code < 0xD800 || code > 0xDBFF) return {0, code, 1};
    if (available < 2) return {CES_ERROR_SRC_BUFFER_END, 0, 0};
    const uint32_t low = loadUnit<uint16_t, BigEndian>(source + 1);
    if (low < 0xDC00 || low > 0xDFFF) return {CES_ERROR_INVALID_ENCODE, 0, 0};
    return {0, 0x10000 + ((code - 0xD800) << 10) + low - 0xDC00, 2};
}

Decoded decodeUtf8Scalar(const uint8_t* source, uint32_t available) {
    Decoded decoded{};
    decoded.result = decodeUtf8(source, available, decoded.length, decoded.code);
    return decoded;
}

struct MultibyteTarget {
    const void* profile;
    uint8_t* target;
    uint32_t capacity;
    uint32_t* targetLength;
    const void* buffer() const { return target; }
    int write(uint32_t code) const {
        uint8_t encoded[3];
        uint32_t length = 0;
        if (const int result = encodeMbcs(profile, code, encoded, length); result != 0) return result;
        if (capacity < length) return CES_ERROR_DST_BUFFER_END;
        std::memcpy(target, encoded, length);
        if (targetLength) *targetLength = length;
        return 0;
    }
};

struct SingleByteTarget {
    const uint8_t* profile;
    uint8_t* target;
    const void* buffer() const { return target; }
    int write(uint32_t code) const {
        return unicodeToSbc(profile, code, *target) ? 0 : CES_ERROR_UNASSIGNED_CODE;
    }
};

template<typename Source, typename Decode, typename Target>
int fromUnicodeBuffer(bool known, const Source* source, uint32_t sourceMax, uint32_t* sourceLength, const Target& target, Decode decode) {
    if (!known) return CES_ERROR_INVALID_PARAMETER;
    if (!source) return CES_ERROR_INVALID_SRC_BUFFER;
    if (sourceMax == 0) return CES_ERROR_SRC_BUFFER_END;
    if (!target.buffer()) return CES_ERROR_INVALID_DST_BUFFER;
    const Decoded decoded = decode(source, sourceMax);
    if (decoded.result != 0) return decoded.result;
    if (const int result = target.write(decoded.code); result != 0) return result;
    if (sourceLength) *sourceLength = decoded.length;
    return 0;
}

template<typename Target>
int fromUnicodeScalar(bool known, uint32_t code, const Target& target) {
    if (!known) return CES_ERROR_INVALID_PARAMETER;
    if (!target.buffer()) return CES_ERROR_INVALID_DST_BUFFER;
    if (!isScalar(code)) return CES_ERROR_INVALID_ENCODE;
    return target.write(code);
}

template<bool BigEndian, typename Target>
int fromUtf32Unit(bool known, const uint32_t* source, const Target& target) {
    if (!known) return CES_ERROR_INVALID_PARAMETER;
    if (!source) return CES_ERROR_INVALID_SRC_BUFFER;
    if (!target.buffer()) return CES_ERROR_INVALID_DST_BUFFER;
    const uint32_t code = loadUnit<uint32_t, BigEndian>(source);
    if (!isScalar(code)) return CES_ERROR_INVALID_ENCODE;
    return target.write(code);
}

}

extern "C" {

int APS5_VABI sceCesEucJpToUtf16(const void* profile, const uint8_t* euc, uint32_t eucMax, uint32_t* eucSize, uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len) {
    return fromMultibyte(isEucJp(profile), profile, euc, eucMax, eucSize, Utf16Target<NATIVE_BIG_ENDIAN>{utf16, utf16Max, utf16Len});
}

int APS5_VABI sceCesEucJpToUtf16be(const void* profile, const uint8_t* euc, uint32_t eucMax, uint32_t* eucSize, uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len) {
    return fromMultibyte(isEucJp(profile), profile, euc, eucMax, eucSize, Utf16Target<true>{utf16, utf16Max, utf16Len});
}

int APS5_VABI sceCesEucJpToUtf16le(const void* profile, const uint8_t* euc, uint32_t eucMax, uint32_t* eucSize, uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len) {
    return fromMultibyte(isEucJp(profile), profile, euc, eucMax, eucSize, Utf16Target<false>{utf16, utf16Max, utf16Len});
}

int APS5_VABI sceCesEucJpToUtf32(const void* profile, const uint8_t* euc, uint32_t eucMax, uint32_t* eucSize, uint32_t* utf32) {
    return fromMultibyte(isEucJp(profile), profile, euc, eucMax, eucSize, Utf32Target<NATIVE_BIG_ENDIAN>{utf32});
}

int APS5_VABI sceCesEucJpToUtf32be(const void* profile, const uint8_t* euc, uint32_t eucMax, uint32_t* eucSize, uint32_t* utf32) {
    return fromMultibyte(isEucJp(profile), profile, euc, eucMax, eucSize, Utf32Target<true>{utf32});
}

int APS5_VABI sceCesEucJpToUtf32le(const void* profile, const uint8_t* euc, uint32_t eucMax, uint32_t* eucSize, uint32_t* utf32) {
    return fromMultibyte(isEucJp(profile), profile, euc, eucMax, eucSize, Utf32Target<false>{utf32});
}

int APS5_VABI sceCesMbcToUcs2(const void* profile, const uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen, uint16_t* ucs2) {
    return fromMultibyte(isMbc(profile), profile, mbc, mbcMax, mbcLen, Ucs2Target{ucs2});
}

int APS5_VABI sceCesMbcToUtf8(const void* profile, const uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen, uint8_t* utf8, uint32_t utf8Max, uint32_t* utf8Len) {
    return fromMultibyte(isMbc(profile), profile, mbc, mbcMax, mbcLen, Utf8Target{utf8, utf8Max, utf8Len});
}

int APS5_VABI sceCesMbcToUtf16(const void* profile, const uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen, uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len) {
    return fromMultibyte(isMbc(profile), profile, mbc, mbcMax, mbcLen, Utf16Target<NATIVE_BIG_ENDIAN>{utf16, utf16Max, utf16Len});
}

int APS5_VABI sceCesMbcToUtf16be(const void* profile, const uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen, uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len) {
    return fromMultibyte(isMbc(profile), profile, mbc, mbcMax, mbcLen, Utf16Target<true>{utf16, utf16Max, utf16Len});
}

int APS5_VABI sceCesMbcToUtf16le(const void* profile, const uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen, uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len) {
    return fromMultibyte(isMbc(profile), profile, mbc, mbcMax, mbcLen, Utf16Target<false>{utf16, utf16Max, utf16Len});
}

int APS5_VABI sceCesMbcToUtf32(const void* profile, const uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen, uint32_t* utf32) {
    return fromMultibyte(isMbc(profile), profile, mbc, mbcMax, mbcLen, Utf32Target<NATIVE_BIG_ENDIAN>{utf32});
}

int APS5_VABI sceCesMbcToUtf32be(const void* profile, const uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen, uint32_t* utf32) {
    return fromMultibyte(isMbc(profile), profile, mbc, mbcMax, mbcLen, Utf32Target<true>{utf32});
}

int APS5_VABI sceCesMbcToUtf32le(const void* profile, const uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen, uint32_t* utf32) {
    return fromMultibyte(isMbc(profile), profile, mbc, mbcMax, mbcLen, Utf32Target<false>{utf32});
}

int APS5_VABI sceCesSbcToUcs2(const uint8_t* profile, uint8_t sbc, uint16_t* ucs2) {
    return fromSingleByte(profile, sbc, Ucs2Target{ucs2});
}

int APS5_VABI sceCesSbcToUtf16(const uint8_t* profile, uint8_t sbc, uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len) {
    return fromSingleByte(profile, sbc, Utf16Target<NATIVE_BIG_ENDIAN>{utf16, utf16Max, utf16Len});
}

int APS5_VABI sceCesSbcToUtf16be(const uint8_t* profile, uint8_t sbc, uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len) {
    return fromSingleByte(profile, sbc, Utf16Target<true>{utf16, utf16Max, utf16Len});
}

int APS5_VABI sceCesSbcToUtf16le(const uint8_t* profile, uint8_t sbc, uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len) {
    return fromSingleByte(profile, sbc, Utf16Target<false>{utf16, utf16Max, utf16Len});
}

int APS5_VABI sceCesSbcToUtf32(const uint8_t* profile, uint8_t sbc, uint32_t* utf32) {
    return fromSingleByte(profile, sbc, Utf32Target<NATIVE_BIG_ENDIAN>{utf32});
}

int APS5_VABI sceCesSbcToUtf32be(const uint8_t* profile, uint8_t sbc, uint32_t* utf32) {
    return fromSingleByte(profile, sbc, Utf32Target<true>{utf32});
}

int APS5_VABI sceCesSbcToUtf32le(const uint8_t* profile, uint8_t sbc, uint32_t* utf32) {
    return fromSingleByte(profile, sbc, Utf32Target<false>{utf32});
}

int APS5_VABI sceCesUcs2ToEucJp(uint16_t ucs2, const void* profile, uint8_t* euc, uint32_t eucMax, uint32_t* eucSize) {
    return fromUnicodeScalar(isEucJp(profile), ucs2, MultibyteTarget{profile, euc, eucMax, eucSize});
}

int APS5_VABI sceCesUcs2ToMbc(uint16_t ucs2, const void* profile, uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen) {
    return fromUnicodeScalar(isMbc(profile), ucs2, MultibyteTarget{profile, mbc, mbcMax, mbcLen});
}

int APS5_VABI sceCesUcs2ToSbc(uint16_t ucs2, const uint8_t* profile, uint8_t* sbc) {
    return fromUnicodeScalar(isSbc(profile), ucs2, SingleByteTarget{profile, sbc});
}

int APS5_VABI sceCesUtf16ToEucJp(const uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len, const void* profile, uint8_t* euc, uint32_t eucMax, uint32_t* eucSize) {
    return fromUnicodeBuffer(isEucJp(profile), utf16, utf16Max, utf16Len, MultibyteTarget{profile, euc, eucMax, eucSize}, decodeUtf16<NATIVE_BIG_ENDIAN>);
}

int APS5_VABI sceCesUtf16beToEucJp(const uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len, const void* profile, uint8_t* euc, uint32_t eucMax, uint32_t* eucSize) {
    return fromUnicodeBuffer(isEucJp(profile), utf16, utf16Max, utf16Len, MultibyteTarget{profile, euc, eucMax, eucSize}, decodeUtf16<true>);
}

int APS5_VABI sceCesUtf16leToEucJp(const uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len, const void* profile, uint8_t* euc, uint32_t eucMax, uint32_t* eucSize) {
    return fromUnicodeBuffer(isEucJp(profile), utf16, utf16Max, utf16Len, MultibyteTarget{profile, euc, eucMax, eucSize}, decodeUtf16<false>);
}

int APS5_VABI sceCesUtf16ToMbc(const uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len, const void* profile, uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen) {
    return fromUnicodeBuffer(isMbc(profile), utf16, utf16Max, utf16Len, MultibyteTarget{profile, mbc, mbcMax, mbcLen}, decodeUtf16<NATIVE_BIG_ENDIAN>);
}

int APS5_VABI sceCesUtf16beToMbc(const uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len, const void* profile, uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen) {
    return fromUnicodeBuffer(isMbc(profile), utf16, utf16Max, utf16Len, MultibyteTarget{profile, mbc, mbcMax, mbcLen}, decodeUtf16<true>);
}

int APS5_VABI sceCesUtf16leToMbc(const uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len, const void* profile, uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen) {
    return fromUnicodeBuffer(isMbc(profile), utf16, utf16Max, utf16Len, MultibyteTarget{profile, mbc, mbcMax, mbcLen}, decodeUtf16<false>);
}

int APS5_VABI sceCesUtf16ToSbc(const uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len, const uint8_t* profile, uint8_t* sbc) {
    return fromUnicodeBuffer(isSbc(profile), utf16, utf16Max, utf16Len, SingleByteTarget{profile, sbc}, decodeUtf16<NATIVE_BIG_ENDIAN>);
}

int APS5_VABI sceCesUtf16beToSbc(const uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len, const uint8_t* profile, uint8_t* sbc) {
    return fromUnicodeBuffer(isSbc(profile), utf16, utf16Max, utf16Len, SingleByteTarget{profile, sbc}, decodeUtf16<true>);
}

int APS5_VABI sceCesUtf16leToSbc(const uint16_t* utf16, uint32_t utf16Max, uint32_t* utf16Len, const uint8_t* profile, uint8_t* sbc) {
    return fromUnicodeBuffer(isSbc(profile), utf16, utf16Max, utf16Len, SingleByteTarget{profile, sbc}, decodeUtf16<false>);
}

int APS5_VABI sceCesUtf32ToEucJp(uint32_t utf32, const void* profile, uint8_t* euc, uint32_t eucMax, uint32_t* eucSize) {
    return fromUnicodeScalar(isEucJp(profile), utf32, MultibyteTarget{profile, euc, eucMax, eucSize});
}

int APS5_VABI sceCesUtf32beToEucJp(const uint32_t* utf32, const void* profile, uint8_t* euc, uint32_t eucMax, uint32_t* eucSize) {
    return fromUtf32Unit<true>(isEucJp(profile), utf32, MultibyteTarget{profile, euc, eucMax, eucSize});
}

int APS5_VABI sceCesUtf32leToEucJp(const uint32_t* utf32, const void* profile, uint8_t* euc, uint32_t eucMax, uint32_t* eucSize) {
    return fromUtf32Unit<false>(isEucJp(profile), utf32, MultibyteTarget{profile, euc, eucMax, eucSize});
}

int APS5_VABI sceCesUtf32ToMbc(uint32_t utf32, const void* profile, uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen) {
    return fromUnicodeScalar(isMbc(profile), utf32, MultibyteTarget{profile, mbc, mbcMax, mbcLen});
}

int APS5_VABI sceCesUtf32beToMbc(const uint32_t* utf32, const void* profile, uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen) {
    return fromUtf32Unit<true>(isMbc(profile), utf32, MultibyteTarget{profile, mbc, mbcMax, mbcLen});
}

int APS5_VABI sceCesUtf32leToMbc(const uint32_t* utf32, const void* profile, uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen) {
    return fromUtf32Unit<false>(isMbc(profile), utf32, MultibyteTarget{profile, mbc, mbcMax, mbcLen});
}

int APS5_VABI sceCesUtf32ToSbc(uint32_t utf32, const uint8_t* profile, uint8_t* sbc) {
    return fromUnicodeScalar(isSbc(profile), utf32, SingleByteTarget{profile, sbc});
}

int APS5_VABI sceCesUtf32beToSbc(const uint32_t* utf32, const uint8_t* profile, uint8_t* sbc) {
    return fromUtf32Unit<true>(isSbc(profile), utf32, SingleByteTarget{profile, sbc});
}

int APS5_VABI sceCesUtf32leToSbc(const uint32_t* utf32, const uint8_t* profile, uint8_t* sbc) {
    return fromUtf32Unit<false>(isSbc(profile), utf32, SingleByteTarget{profile, sbc});
}

int APS5_VABI sceCesUtf8ToEucJp(const uint8_t* utf8, uint32_t utf8Max, uint32_t* utf8Len, const void* profile, uint8_t* euc, uint32_t eucMax, uint32_t* eucSize) {
    return fromUnicodeBuffer(isEucJp(profile), utf8, utf8Max, utf8Len, MultibyteTarget{profile, euc, eucMax, eucSize}, decodeUtf8Scalar);
}

int APS5_VABI sceCesUtf8ToMbc(const uint8_t* utf8, uint32_t utf8Max, uint32_t* utf8Len, const void* profile, uint8_t* mbc, uint32_t mbcMax, uint32_t* mbcLen) {
    return fromUnicodeBuffer(isMbc(profile), utf8, utf8Max, utf8Len, MultibyteTarget{profile, mbc, mbcMax, mbcLen}, decodeUtf8Scalar);
}

}

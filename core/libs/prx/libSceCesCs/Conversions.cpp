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

}

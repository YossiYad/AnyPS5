#ifndef CORE_LIBS_PRX_LIBSCECESCS_CODEC_HPP
#define CORE_LIBS_PRX_LIBSCECESCS_CODEC_HPP

#include <cstdint>

namespace Ces {

inline constexpr int CES_ERROR_INVALID_PARAMETER = static_cast<int>(0x805C0001);
inline constexpr int CES_ERROR_INVALID_SRC_BUFFER = static_cast<int>(0x805C0010);
inline constexpr int CES_ERROR_SRC_BUFFER_END = static_cast<int>(0x805C0011);
inline constexpr int CES_ERROR_INVALID_ENCODE = static_cast<int>(0x805C0014);
inline constexpr int CES_ERROR_UNASSIGNED_CODE = static_cast<int>(0x805C0020);
inline constexpr int CES_ERROR_INVALID_DST_BUFFER = static_cast<int>(0x805C0030);
inline constexpr int CES_ERROR_DST_BUFFER_END = static_cast<int>(0x805C0031);

uint32_t sbcToUnicode(const uint8_t* profile, uint8_t sbc);
bool unicodeToSbc(const uint8_t* profile, uint32_t code, uint8_t& sbc);
int decodeUtf8(const uint8_t* utf8, uint32_t utf8max, uint32_t& length, uint32_t& code);
uint32_t encodeUtf8(uint32_t code, uint8_t* utf8);
bool isEucJp(const void* profile);
bool isSjis(const void* profile);
bool isSbc(const void* profile);
int decodeMbcs(const void* profile, const uint8_t* mbcs, uint32_t mbcsMax, uint32_t& length, uint32_t& code);
int encodeMbcs(const void* profile, uint32_t code, uint8_t* mbcs, uint32_t& length);

inline bool isScalar(uint32_t code) {
    return code <= 0x10FFFF && (code < 0xD800 || code > 0xDFFF);
}

template<typename Unit, bool BigEndian>
uint32_t loadUnit(const Unit* source) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(source);
    uint32_t code = 0;
    for (uint32_t i = 0; i < sizeof(Unit); ++i) code |= uint32_t(bytes[i]) << (8 * (BigEndian ? sizeof(Unit) - 1 - i : i));
    return code;
}

template<typename Unit, bool BigEndian>
void storeUnit(Unit* target, uint32_t code) {
    auto* bytes = reinterpret_cast<uint8_t*>(target);
    for (uint32_t i = 0; i < sizeof(Unit); ++i) bytes[i] = static_cast<uint8_t>(code >> (8 * (BigEndian ? sizeof(Unit) - 1 - i : i)));
}

}

#endif

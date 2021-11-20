#ifndef BYTESWAPPING_H
#define BYTESWAPPING_H

#include <cstdint>
#define BYTESWAP_DECORATION static inline
#define BYTESWAP_CONSTEXPR constexpr

BYTESWAP_DECORATION BYTESWAP_CONSTEXPR
uint16_t swap_u16(uint16_t x)
{
  return (x >> 8) | (x << 8);
}

BYTESWAP_DECORATION BYTESWAP_CONSTEXPR
uint32_t swap_u32(uint32_t x)
{
  x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
  return (x << 16) | (x >> 16);
}

BYTESWAP_DECORATION BYTESWAP_CONSTEXPR
uint64_t swap_u64(uint64_t x)
{
  x = ((x << 8) & 0xFF00FF00FF00FF00ULL) | ((x >> 8) & 0x00FF00FF00FF00FFULL);
  x = ((x << 16) & 0xFFFF0000FFFF0000ULL) | ((x >> 16) & 0x0000FFFF0000FFFFULL);
  return (x << 32) | (x >> 32);
}

BYTESWAP_DECORATION
int16_t swap_i16(int16_t x)
{
    uint16_t const tmp = swap_u16(reinterpret_cast<uint16_t&>(x));
    return reinterpret_cast<int16_t const&>(tmp);
}

BYTESWAP_DECORATION
int32_t swap_i32(int32_t x)
{
    uint32_t const tmp = swap_u32(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<int32_t const&>(tmp);
}

BYTESWAP_DECORATION
int64_t swap_i64(int64_t x)
{
    uint64_t const tmp = swap_u64(reinterpret_cast<uint64_t&>(x));
    return reinterpret_cast<int64_t const&>(tmp);
}

BYTESWAP_DECORATION
float swap_f32(float x)
{
  uint32_t const tmp = swap_u32(reinterpret_cast<uint32_t&>(x));
  return reinterpret_cast<float const&>(tmp);
}

BYTESWAP_DECORATION
double swap_f64(double x)
{
  uint64_t const tmp = swap_u64(reinterpret_cast<uint64_t&>(x));
  return reinterpret_cast<double const&>(tmp);
}

#undef BYTESWAP_DECORATION

#endif // BYTESWAPPING_H

#pragma once

#include <cstdint>

namespace ImNBT
{

enum class TAG : uint8_t
{
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    Byte_Array = 7,
    String = 8,
    List = 9,
    Compound = 10,
    Int_Array = 11,
    Long_Array = 12,
    INVALID = 0xCC
};

union TagPayload
{
    int8_t byte_;
    int16_t short_;
    int32_t int_;
    int64_t long_;
    float float_;
    double double_;
    struct
    {
        int32_t byte_array_length_;
        size_t byte_array_pool_index;
    };
    struct
    {
        int16_t string_length_;
        size_t string_pool_index;
    };
    struct
    {
        TAG list_type_;
        int32_t list_length_;
    };
};

struct DataTag
{
    TAG type_;
    TagPayload data_;
};

} // namespace ImNBT

#pragma once

#include <cstdint>
#include <string_view>

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

struct TagPayload
{
    union
    {
        int8_t byte_;
        int16_t short_;
        int32_t int_;
        int64_t long_;
        float float_;
        double double_;
        struct TagPayloadByteArray
        {
            int32_t length_;
            size_t pool_index_;
        } byte_array_;
        struct TagPayloadString
        {
            int16_t length_;
            size_t pool_index_;
        } string_;
        struct TagPayloadList
        {
            TAG type_;
            int32_t length_;
            size_t pool_index_;
        } list_;
        struct TagPayloadCompound
        {
            
        } compound_;
    };
};

struct DataTag : TagPayload
{
    DataTag(TAG type) : type(type) {}
    DataTag() = default;

    TAG type;
};

struct NamedDataTag : DataTag
{
    std::string_view name;
};

} // namespace ImNBT

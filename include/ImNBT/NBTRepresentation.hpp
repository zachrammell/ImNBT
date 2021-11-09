#pragma once

#include <cstdint>
#include <set>
#include <string_view>
#include <vector>

namespace ImNBT
{

class DataStore;

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
    struct ByteArray
    {
        int32_t length_;
        size_t pool_index_;
    };
    struct IntArray
    {
        int32_t length_;
        size_t pool_index_;
    };
    struct LongArray
    {
        int32_t length_;
        size_t pool_index_;
    };
    struct String
    {
        int16_t length_;
        size_t pool_index_;
    };
    struct List
    {
        TAG type_;
        int32_t length_;
        size_t pool_index_;
    };
    struct Compound
    {
        size_t pool_index_;
    };

    union
    {
        int8_t byte_;
        int16_t short_;
        int32_t int_;
        int64_t long_;
        float float_;
        double double_;
        ByteArray byte_array_;
        String string_;
        List list_;
        Compound compound_;
        IntArray int_array_;
        LongArray long_array_;
    };
};

struct DataTag : TagPayload
{
    DataTag(TAG type) : type(type) {}
    DataTag() = default;

    TAG type;
#if defined(DEBUG)
    DataStore* dataStore;
#endif
};

struct NamedDataTag : DataTag
{
    //std::string_view name;
};

struct NamedDataTagIndex
{
    uint64_t idx;
};

class DataStore
{
public:
    std::vector<int8_t> byteListBuffer;
    std::vector<int16_t> shortListBuffer;
    std::vector<int32_t> intListBuffer;
    std::vector<int64_t> longListBuffer;
    std::vector<float> floatListBuffer;
    std::vector<double> doubleListBuffer;
    std::vector<char> stringBuffer;

    // sets of indices into namedTags
    std::vector<std::set<NamedDataTagIndex>> compoundStorage;

    std::vector<NamedDataTag> namedTags;
};

} // namespace ImNBT

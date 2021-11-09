#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace ImNBT
{

using StringView = std::string_view;

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
        int32_t count_;
        size_t poolIndex_;
    };
    struct IntArray
    {
        int32_t count_;
        size_t poolIndex_;
    };
    struct LongArray
    {
        int32_t count_;
        size_t poolIndex_;
    };
    struct String
    {
        int16_t length_;
        size_t poolIndex_;
    };
    struct List
    {
        TAG elementType_;
        int32_t count_;
        size_t poolIndex_;
    };
    struct Compound
    {
        size_t poolIndex_;
    };

    union
    {
        int8_t byte_;
        int16_t short_;
        int32_t int_;
        int64_t long_;
        float float_;
        double double_;
        ByteArray byteArray_;
        String string_;
        List list_;
        Compound compound_;
        IntArray intArray_;
        LongArray longArray_;
    };
};

struct DataTag : TagPayload
{
    DataTag(TAG type) : type(type) {}
    DataTag() = default;

    TAG type;
#if defined(DEBUG)
    void* dataStore;
#endif
};

struct NamedDataTag : DataTag
{
    std::string_view name;
};

struct NamedDataTagIndex
{
    uint64_t idx;
    NamedDataTagIndex(uint64_t i) : idx(i) {}
    NamedDataTagIndex() = default;
    operator uint64_t&()
    {
        return idx;
    }
    bool operator<(NamedDataTagIndex const& rhs) const
    {
        return idx < rhs.idx;
    }
};

struct DataStore
{
    std::vector<int8_t> byteBuffer;
    std::vector<int16_t> shortBuffer;
    std::vector<int32_t> intBuffer;
    std::vector<int64_t> longBuffer;
    std::vector<float> floatBuffer;
    std::vector<double> doubleBuffer;
    std::vector<char> stringBuffer;

    // sets of indices into namedTags
    std::vector<std::vector<NamedDataTagIndex>> compoundStorage;

    std::vector<NamedDataTag> namedTags;

    NamedDataTagIndex AddNamedDataTag(TAG type, StringView name);
};

} // namespace ImNBT

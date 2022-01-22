#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace ImNBT
{

using StringView = std::string_view;
#define ImNBT_ALL_TYPES byte, int16_t, int32_t, int64_t, float, double, char, TagPayload::ByteArray, TagPayload::IntArray, TagPayload::LongArray, TagPayload::String, TagPayload::List, TagPayload::Compound

struct byte
{
  int8_t b;
  byte() = default;
  byte(int8_t i) : b(i) {}
  byte& operator=(int8_t i)
  {
    b = i;
    return *this;
  }
  operator int8_t&() { return b; }
  operator int8_t const &() const { return b; }
};

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
    uint16_t length_;
    size_t poolIndex_;
  };
  struct List
  {
    TAG elementType_ = TAG::End;
    int32_t count_ = 0;
    size_t poolIndex_ = std::numeric_limits<size_t>::max();
  };
  struct Compound
  {
    size_t storageIndex_ = std::numeric_limits<size_t>::max();
  };

  template<typename T>
  T& As()
  {
    return std::get<T>(data_);
  }

  template<typename T>
  T const& As() const
  {
    return std::get<T>(data_);
  }

  template<typename T>
  void Set(T val = T{})
  {
    data_.emplace<T>(val);
  }

private:
  std::variant<ImNBT_ALL_TYPES> data_;
};

class DataTag
{
public:
  DataTag(TAG type) : type(type) {}
  DataTag() = default;

  TagPayload payload;
  TAG type = TAG::INVALID;
};

class NamedDataTag
{
public:
  StringView GetName() const;
  void SetName(StringView inName);

private:
  std::string name;

public:
  DataTag dataTag;
};

namespace Internal
{
bool IsContainer(TAG t);

struct NamedDataTagIndex
{
  uint64_t idx;
  NamedDataTagIndex() = default;
  NamedDataTagIndex(uint64_t i) : idx(i) {}
  operator uint64_t&() { return idx; }
  operator uint64_t const &() const { return idx; }
  bool operator<(NamedDataTagIndex const& rhs) const { return idx < rhs.idx; }
};

/**
 * Pools are the backing storage of lists/arrays
 */

template<typename... Ts>
struct Pools
{
  std::tuple<std::vector<Ts>...> pools;

  template<typename T>
  std::vector<T>& Pool()
  {
    return std::get<std::vector<T>>(pools);
  }

  void Clear()
  {
    (std::get<std::vector<Ts>>(pools).clear(), ...);
  }
};

using AllPools = Internal::Pools<ImNBT_ALL_TYPES>;

} // namespace Internal

struct DataStore : Internal::AllPools
{
  // sets of indices into namedTags
  std::vector<std::vector<Internal::NamedDataTagIndex>> compoundStorage;

  std::vector<NamedDataTag> namedTags;

  Internal::NamedDataTagIndex AddNamedDataTag(TAG type, StringView name);

  void Clear();
};

#undef ImNBT_ALL_TYPES

} // namespace ImNBT

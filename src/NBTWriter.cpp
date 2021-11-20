#include <ImNBT/NBTWriter.hpp>

#include "byteswapping.h"

#include <cassert>
#include <iostream>

namespace ImNBT
{

template<typename T, std::enable_if_t<(sizeof(T) <= sizeof(T*)), bool> = true>
void Store(std::vector<uint8_t>& v, T data)
{
  auto size = v.size();
  v.resize(size + sizeof(T));
  std::memcpy(v.data() + size, &data, sizeof(T));
}

template<typename T, std::enable_if_t<(sizeof(T) > sizeof(T*)), bool> = true>
void Store(std::vector<uint8_t>& v, T& data)
{
  auto size = v.size();
  v.resize(size + sizeof(T));
  std::memcpy(v.data() + size, &data, sizeof(T));
}

template<typename T>
void StoreRange(std::vector<uint8_t>& v, T* data, size_t count)
{
  auto size = v.size();
  v.resize(size + sizeof(T) * count);
  std::memcpy(v.data() + size, data, sizeof(T) * count);
}

// private implementations

bool Writer::OutputBinaryFileUncompressed(StringView filepath)
{
  FILE* file = fopen(filepath.data(), "wb");
  if (!file)
  {
    return false;
  }
  std::vector<uint8_t> data;
  if (!OutputBinary(data))
  {
    fclose(file);
    return false;
  }
  auto written = fwrite(data.data(), sizeof(uint8_t), data.size(), file);
  fclose(file);
  return written == data.size();
}

bool Writer::OutputBinary(std::vector<uint8_t>& out)
{
  if (!Finalized())
    return false;
  auto const& root = dataStore.namedTags[0];
  OutputBinaryTag(out, root);
  return true;
}

void Writer::OutputBinaryTag(std::vector<uint8_t>& out, NamedDataTag const& tag)
{
  Store(out, tag.type);
  OutputBinaryStr(out, tag.name);
  OutputBinaryPayload(out, tag);
}

void Writer::OutputBinaryStr(std::vector<uint8_t>& out, StringView str)
{
  uint16_t const lenBigEndian = swap_u16(static_cast<int16_t>(str.length()));
  Store(out, lenBigEndian);
  StoreRange(out, str.data(), str.length());
}

void Writer::OutputBinaryPayload(std::vector<uint8_t>& out, DataTag const& tag)
{
  switch (tag.type)
  {
    case TAG::Byte: {
      Store(out, tag.As<byte>());
    }
    break;
    case TAG::Short: {
      Store(out, swap_i16(tag.As<int16_t>()));
    }
    break;
    case TAG::Int: {
      Store(out, swap_i32(tag.As<int32_t>()));
    }
    break;
    case TAG::Long: {
      Store(out, swap_i64(tag.As<int64_t>()));
    }
    break;
    case TAG::Float: {
      Store(out, swap_f32(tag.As<float>()));
    }
    break;
    case TAG::Double: {
      Store(out, swap_f64(tag.As<double>()));
    }
    break;
    case TAG::Byte_Array: {
      auto& byteArray = tag.As<TagPayload::ByteArray>();
      Store(out, swap_i32(byteArray.count_));
      StoreRange(out, dataStore.Pool<byte>().data() + byteArray.poolIndex_, byteArray.count_);
    }
    break;
    case TAG::Int_Array: {
      auto& intArray = tag.As<TagPayload::IntArray>();
      auto intPool = dataStore.Pool<int32_t>().data() + intArray.poolIndex_;
      Store(out, swap_i32(intArray.count_));
      for (int i = 0; i < intArray.count_; ++i)
      {
        Store(out, swap_i32(intPool[i]));
      }
    }
    break;
    case TAG::Long_Array: {
      auto& longArray = tag.As<TagPayload::LongArray>();
      auto longPool = dataStore.Pool<int64_t>().data() + longArray.poolIndex_;
      Store(out, swap_i32(longArray.count_));
      for (int i = 0; i < longArray.count_; ++i)
      {
        Store(out, swap_i64(longPool[i]));
      }
    }
    break;
    case TAG::String: {
      auto& string = tag.As<TagPayload::String>();
      Store(out, swap_u16(string.length_));
      StoreRange(out, dataStore.Pool<char>().data() + string.poolIndex_, string.length_);
    }
    break;
    case TAG::List: {
      auto& list = tag.As<TagPayload::List>();
      if (list.count_ == 0)
      {
        Store(out, TAG::End);
        Store(out, list.count_);
        return;
      }
      Store(out, list.elementType_);
      Store(out, swap_u32(list.count_));
      switch (list.elementType_)
      {
        case TAG::Byte: {
          StoreRange(out, dataStore.Pool<byte>().data() + list.poolIndex_, list.count_);
        }
        break;
        case TAG::Short: {
          auto const* shortPool = dataStore.Pool<int16_t>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(int16_t) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_i16(shortPool[i]));
          }
        }
        break;
        case TAG::Int: {
          auto const* intPool = dataStore.Pool<int32_t>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(int32_t) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_i32(intPool[i]));
          }
        }
        break;
        case TAG::Long: {
          auto const* longPool = dataStore.Pool<int64_t>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(int64_t) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_i64(longPool[i]));
          }
        }
        break;
        case TAG::Float: {
          auto const* floatPool = dataStore.Pool<float>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(float) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_f32(floatPool[i]));
          }
        }
        break;
        case TAG::Double: {
          auto const* doublePool = dataStore.Pool<double>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(double) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_f64(doublePool[i]));
          }
        }
        break;
        case TAG::Byte_Array: {
          auto const* byteArrayPool = dataStore.Pool<TagPayload::ByteArray>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            auto const& byteArray = byteArrayPool[i];
            Store(out, swap_i32(byteArray.count_));
            StoreRange(out, dataStore.Pool<byte>().data() + byteArray.poolIndex_, byteArray.count_);
          }
        }
        break;
        case TAG::Int_Array: {
          auto const* intArrayPool = dataStore.Pool<TagPayload::IntArray>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            auto const& intArray = intArrayPool[i];
            auto const* intPool = dataStore.Pool<int32_t>().data() + intArray.poolIndex_;
            Store(out, swap_i32(intArray.count_));
            for (int j = 0; j < intArray.count_; ++j)
            {
              Store(out, swap_i32(intPool[j]));
            }
          }
        }
        break;
        case TAG::Long_Array: {
          auto const* longArrayPool = dataStore.Pool<TagPayload::LongArray>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            auto const& longArray = longArrayPool[i];
            auto const* longPool = dataStore.Pool<int64_t>().data() + longArray.poolIndex_;
            Store(out, swap_i32(longArray.count_));
            for (int j = 0; j < longArray.count_; ++j)
            {
              Store(out, swap_i64(longPool[j]));
            }
          }
        }
        break;
        case TAG::String: {
          auto const* stringPool = dataStore.Pool<TagPayload::String>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            auto& string = stringPool[i];
            Store(out, swap_u16(string.length_));
            StoreRange(out, dataStore.Pool<char>().data() + string.poolIndex_, string.length_);
          }
        }
        break;
        case TAG::List: {
          auto const* listPool = dataStore.Pool<TagPayload::List>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            DataTag sublistTag;
            sublistTag.type = TAG::List;
            sublistTag.Set(listPool[i]);
            OutputBinaryPayload(out, sublistTag);
          }
        }
        break;
        case TAG::Compound: {
          auto const* compoundPool = dataStore.Pool<TagPayload::Compound>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            DataTag subcompoundTag;
            subcompoundTag.type = TAG::Compound;
            subcompoundTag.Set(compoundPool[i]);
            OutputBinaryPayload(out, subcompoundTag);
          }
        }
        break;
      }
    }
    break;
    case TAG::Compound: {
      auto& compound = tag.As<TagPayload::Compound>();
      for (auto namedTagIndex : dataStore.compoundStorage[compound.storageIndex_])
      {
        OutputBinaryTag(out, dataStore.namedTags[namedTagIndex]);
      }
      Store(out, TAG::End);
    }
    break;
  }
}

} // namespace ImNBT

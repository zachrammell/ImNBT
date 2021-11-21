#include <ImNBT/NBTReader.hpp>

#include "byteswapping.h"

#include "zlib.h"

#include <array>
#include <cassert>
#include <iostream>

namespace ImNBT
{

bool Reader::ImportFile(StringView filepath)
{
  if (ImportBinaryFileUncompressed(filepath))
    return true;
  if (ImportBinaryFile(filepath))
    return true;
  if (ImportTextFile(filepath))
    return true;
  return false;
}

bool Reader::ImportTextFile(StringView filepath)
{
  if (!ImportUncompressedFile(filepath))
    return false;

  return ParseTextStream();
}

bool Reader::ImportBinaryFile(StringView filepath)
{
  gzFile infile = gzopen(filepath.data(), "rb");
  if (!infile) return false;

  std::vector<uint8_t> fileData;
  std::array<uint8_t, 8192> inputBuffer{};
  bool reading = true;
  while (reading)
  {
    size_t const bytesRead = gzfread(inputBuffer.data(), sizeof(uint8_t), inputBuffer.size(), infile);
    if (bytesRead < inputBuffer.size())
    {
      if (gzeof(infile))
      {
        reading = false;
      }
      int err;
      gzerror(infile, &err);
      if (err)
        return false;
    }
    fileData.insert(fileData.end(), inputBuffer.begin(), inputBuffer.begin() + bytesRead);
  }
  gzclose(infile);

  memoryStream.SetContents(std::move(fileData));
  return ParseBinaryStream();
}

bool Reader::ImportBinaryFileUncompressed(StringView filepath)
{
  if (!ImportUncompressedFile(filepath))
    return false;

  return ParseBinaryStream();
}

bool Reader::OpenCompound(StringView name)
{
  if (!HandleNesting(name, TAG::Compound))
  {
    return false;
  }
  return OpenContainer(TAG::Compound, name);
}

void Reader::CloseCompound()
{
  ContainerInfo& container = containers.top();
  if (container.type == TAG::Compound)
  {
    containers.pop();
    return;
  }
  assert(!"Reader : Compound Close Mismatch - Attempted to close a compound when a compound was not open.\n");
}

bool Reader::OpenList(StringView name)
{
  if (!HandleNesting(name, TAG::List))
  {
    return false;
  }
  return OpenContainer(TAG::List, name);
}

int32_t Reader::ListSize()
{
  ContainerInfo& container = containers.top();
  if (container.type == TAG::List)
  {
    return container.Count(dataStore);
  }
  assert(!"Reader : Invalid List Size Read - Attempted to read a list's size when a list was not open.\n");
  return -1;
}

void Reader::CloseList()
{
  ContainerInfo& container = containers.top();
  if (container.type == TAG::List)
  {
    containers.pop();
    return;
  }
  assert(!"Reader : List Close Mismatch - Attempted to close a list when a list was not open.\n");
}

int8_t Reader::ReadByte(StringView name)
{
  HandleNesting(name, TAG::Byte);
  return ReadValue<byte>(TAG::Byte, name);
}

int16_t Reader::ReadShort(StringView name)
{
  HandleNesting(name, TAG::Short);
  return ReadValue<int16_t>(TAG::Short, name);
}

int32_t Reader::ReadInt(StringView name)
{
  HandleNesting(name, TAG::Int);
  return ReadValue<int32_t>(TAG::Int, name);
}

int64_t Reader::ReadLong(StringView name)
{
  HandleNesting(name, TAG::Long);
  return ReadValue<int64_t>(TAG::Long, name);
}

float Reader::ReadFloat(StringView name)
{
  HandleNesting(name, TAG::Float);
  return ReadValue<float>(TAG::Float, name);
}

double Reader::ReadDouble(StringView name)
{
  HandleNesting(name, TAG::Double);
  return ReadValue<double>(TAG::Double, name);
}

std::vector<int8_t> Reader::ReadByteArray(StringView name)
{
  HandleNesting(name, TAG::Byte_Array);
  TagPayload::ByteArray byteArray = ReadValue<TagPayload::ByteArray>(TAG::Byte_Array, name);
  std::vector<int8_t> ret{
    dataStore.Pool<byte>().data() +
        byteArray.poolIndex_,
    dataStore.Pool<byte>().data() +
        byteArray.count_
  };
  return ret;
}

std::vector<int32_t> Reader::ReadIntArray(StringView name)
{
  HandleNesting(name, TAG::Int_Array);
  TagPayload::IntArray intArray = ReadValue<TagPayload::IntArray>(TAG::Int_Array, name);
  std::vector<int32_t> ret{
    dataStore.Pool<int32_t>().data() +
        intArray.poolIndex_,
    dataStore.Pool<int32_t>().data() +
        intArray.count_
  };
  return ret;
}

std::vector<int64_t> Reader::ReadLongArray(StringView name)
{
  HandleNesting(name, TAG::Long_Array);
  TagPayload::LongArray longArray = ReadValue<TagPayload::LongArray>(TAG::Long_Array, name);
  std::vector<int64_t> ret{
    dataStore.Pool<int64_t>().data() +
        longArray.poolIndex_,
    dataStore.Pool<int64_t>().data() +
        longArray.count_
  };
  return ret;
}

StringView Reader::ReadString(StringView name)
{
  HandleNesting(name, TAG::String);
  TagPayload::String string = ReadValue<TagPayload::String>(TAG::String, name);
  return StringView{
    dataStore.Pool<char>().data() + string.poolIndex_,
    static_cast<size_t>(string.length_)
  };
}

//Optional<int8_t> Reader::MaybeReadByte(StringView name)
//{
//  HandleNesting(name, TAG::Byte);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  if (found != named_tags_.end())
//  {
//    assert(found->second.type == TAG::Byte);
//    return std::make_optional(found->second.As<byte>());
//  }
//  return std::nullopt;
//}
//
//Optional<int16_t> Reader::MaybeReadShort(StringView name)
//{
//  HandleNesting(name, TAG::Short);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  if (found != named_tags_.end())
//  {
//    assert(found->second.type == TAG::Short);
//    return std::make_optional(found->second.As<int16_t>());
//  }
//  return std::nullopt;
//}
//
//Optional<int32_t> Reader::MaybeReadInt(StringView name)
//{
//  HandleNesting(name, TAG::Int);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  if (found != named_tags_.end())
//  {
//    assert(found->second.type == TAG::Int);
//    return std::make_optional(found->second.As<int32_t>());
//  }
//  return std::nullopt;
//}
//
//Optional<int64_t> Reader::MaybeReadLong(StringView name)
//{
//  HandleNesting(name, TAG::Long);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  if (found != named_tags_.end())
//  {
//    assert(found->second.type == TAG::Long);
//    return std::make_optional(found->second.As<int64_t>());
//  }
//  return std::nullopt;
//}
//
//Optional<float> Reader::MaybeReadFloat(StringView name)
//{
//  HandleNesting(name, TAG::Float);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  if (found != named_tags_.end())
//  {
//    assert(found->second.type == TAG::Float);
//    return std::make_optional(found->second.As<float>());
//  }
//  return std::nullopt;
//}
//
//Optional<double> Reader::MaybeReadDouble(StringView name)
//{
//  HandleNesting(name, TAG::Double);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  if (found != named_tags_.end())
//  {
//    assert(found->second.type == TAG::Double);
//    return std::make_optional(found->second.As<double>());
//  }
//  return std::nullopt;
//}
//
//Optional<std::vector<int8_t>> Reader::MaybeReadByteArray(StringView name)
//{
//  HandleNesting(name, TAG::Byte_Array);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  if (found != named_tags_.end())
//  {
//    assert(found->second.type == TAG::Byte_Array);
//    std::vector<int8_t> ret{
//      byte_array_pool_.data() +
//          found->second.As<TagPayload::ByteArray>().poolIndex_,
//      byte_array_pool_.data() +
//          found->second.As<TagPayload::ByteArray>().count_
//    };
//    return std::make_optional(ret);
//  }
//  return std::nullopt;
//}
//
//Optional<StringView> Reader::MaybeReadString(StringView name)
//{
//  HandleNesting(name, TAG::String);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  if (found != named_tags_.end())
//  {
//    assert(found->second.type == TAG::String);
//    return std::make_optional<StringView>(
//        string_pool_.data() + found->second.As<TagPayload::String>().poolIndex_,
//        static_cast<size_t>(found->second.As<TagPayload::String>().length_));
//  }
//  return std::nullopt;
//}

template<>
int8_t Reader::Read(StringView name)
{
  return ReadByte(name);
}
template<>
int16_t Reader::Read(StringView name)
{
  return ReadShort(name);
}
template<>
int32_t Reader::Read(StringView name)
{
  return ReadInt(name);
}
template<>
int64_t Reader::Read(StringView name)
{
  return ReadLong(name);
}
template<>
float Reader::Read(StringView name)
{
  return ReadFloat(name);
}
template<>
double Reader::Read(StringView name)
{
  return ReadDouble(name);
}
template<>
std::vector<int8_t> Reader::Read(StringView name)
{
  return ReadByteArray(name);
}
template<>
std::vector<int32_t> Reader::Read(StringView name)
{
  return ReadIntArray(name);
}
template<>
std::vector<int64_t> Reader::Read(StringView name)
{
  return ReadLongArray(name);
}
template<>
StringView Reader::Read(StringView name)
{
  return ReadString(name);
}
//
//template<>
//Optional<int8_t> Reader::MaybeRead(StringView name)
//{
//  return MaybeReadByte(name);
//}
//template<>
//Optional<int16_t> Reader::MaybeRead(StringView name)
//{
//  return MaybeReadShort(name);
//}
//template<>
//Optional<int32_t> Reader::MaybeRead(StringView name)
//{
//  return MaybeReadInt(name);
//}
//template<>
//Optional<int64_t> Reader::MaybeRead(StringView name)
//{
//  return MaybeReadLong(name);
//}
//template<>
//Optional<float> Reader::MaybeRead(StringView name)
//{
//  return MaybeReadFloat(name);
//}
//template<>
//Optional<double> Reader::MaybeRead(StringView name)
//{
//  return MaybeReadDouble(name);
//}

bool Reader::ImportUncompressedFile(StringView filepath)
{
  FILE* infile = fopen(filepath.data(), "rb");
  if (!infile) return false;

  // get file size
  if (fseek(infile, 0, SEEK_END))
    return false;
  size_t const fileSize = ftell(infile);
  rewind(infile);

  std::vector<uint8_t> fileData(fileSize, {});
  size_t const bytesRead = fread(fileData.data(), sizeof(uint8_t), fileSize, infile);
  if (bytesRead != fileSize)
    return false;

  memoryStream.SetContents(std::move(fileData));

  return true;
}

bool Reader::ParseTextStream()
{
  return true;
}

bool Reader::ParseBinaryStream()
{
  // parse root tag
  TAG const type = RetrieveBinaryTag();
  assert(type == TAG::Compound && "root compound tag exists.");
  Begin(RetrieveBinaryStr());

  do {
  } while (ParseBinaryNamedTag() != TAG::End);

  return true;
}

TAG Reader::ParseBinaryNamedTag()
{
  TAG const type = RetrieveBinaryTag();
  if (type == TAG::End)
    return type;
  auto const name = RetrieveBinaryStr();
  ParseBinaryPayload(type, name);
  return type;
}

bool Reader::ParseBinaryPayload(TAG type, StringView name)
{
  switch (type)
  {
    case TAG::Byte:
      WriteByte(memoryStream.Retrieve<byte>(), name);
      break;
    case TAG::Short:
      WriteShort(swap_i16(memoryStream.Retrieve<int16_t>()), name);
      break;
    case TAG::Int:
      WriteInt(swap_i32(memoryStream.Retrieve<int32_t>()), name);
      break;
    case TAG::Long:
      WriteLong(swap_i64(memoryStream.Retrieve<int64_t>()), name);
      break;
    case TAG::Float:
      WriteFloat(swap_f32(memoryStream.Retrieve<float>()), name);
      break;
    case TAG::Double:
      WriteDouble(swap_f64(memoryStream.Retrieve<double>()), name);
      break;
    case TAG::String:
      WriteString(RetrieveBinaryStr(), name);
      break;
    case TAG::Byte_Array: {
      auto const count = RetrieveBinaryArrayLen();
      WriteByteArray(memoryStream.RetrieveRangeView<int8_t>(count), count, name);
    }
    break;
    case TAG::Int_Array: {
      auto const count = RetrieveBinaryArrayLen();
      WriteIntArray(memoryStream.RetrieveRangeView<int32_t>(count), count, name);
    }
    break;
    case TAG::Long_Array: {
      auto const count = RetrieveBinaryArrayLen();
      WriteLongArray(memoryStream.RetrieveRangeView<int64_t>(count), count, name);
    }
    break;
    case TAG::List: {
      if (BeginList(name))
      {
        auto const elementType = RetrieveBinaryTag();
        auto const count = RetrieveBinaryArrayLen();
        for (int i = 0; i < count; ++i)
        {
          ParseBinaryPayload(elementType);
        }
        EndList();
      }
      else
        return false;
    }
    break;
    case TAG::Compound: {
      if (BeginCompound(name))
      {
        do {
        } while (ParseBinaryNamedTag() != TAG::End);
        EndCompound();
      }
      else
        return false;
    }
  }
  return true;
}

TAG Reader::RetrieveBinaryTag()
{
  return memoryStream.Retrieve<TAG>();
}

int32_t Reader::RetrieveBinaryArrayLen()
{
  return swap_i32(memoryStream.Retrieve<int32_t>());
}

std::string Reader::RetrieveBinaryStr()
{
  auto const len = swap_i16(memoryStream.Retrieve<int16_t>());
  std::string str(memoryStream.RetrieveRangeView<char>(len), len);
  return str;
}

bool Reader::HandleNesting(StringView name, TAG t)
{
  auto& container = containers.top();
  // Lists have strict requirements
  if (container.Type() == TAG::List)
  {
    if (!name.empty())
    {
      // problems, lists cannot have named tags
      assert(!"Reader : List Named Read - Attempted to read named tag from a list.");
      return false;
    }
    if (container.ElementType(dataStore) != t)
    {
      if (container.Count(dataStore) != 0)
      {
        assert(!"Reader : List Type Mismatch - Attempted to read the wrong type from a list.");
        return false;
      }
    }
    else
    {
      if (container.currentIndex >= container.Count(dataStore))
      {
        assert(!"Reader : List Overread - Attempted to read too many items from a list.");
        return false;
      }
    }
  }
  else if (container.Type() == TAG::Compound)
  {
    if (name.empty())
    {
      // bad, compound tags must have names
      assert(!"Reader : Compound Unnamed Read - Attempted to read unnamed tag from a compound.");
      return false;
    }
  }
  return true;
}

bool Reader::OpenContainer(TAG t, StringView name)
{
  auto& container = containers.top();
  if (container.Type() == TAG::List)
  {
    ContainerInfo newContainer{};
    newContainer.named = false;
    newContainer.Type() = t;
    newContainer.PoolIndex(dataStore) = container.currentIndex - 1;
    containers.push(newContainer);

    return true;
  }
  if (container.Type() == TAG::Compound)
  {
    for (auto tagIndex : dataStore.compoundStorage[container.Storage(dataStore)])
    {
      // TODO: if this ever becomes a performance issue, look at changing the vector to a set
      if (dataStore.namedTags[tagIndex].name == name)
      {
        ContainerInfo newContainer{};
        newContainer.named = true;
        newContainer.type = t;
        newContainer.namedContainer.tagIndex = tagIndex;
        containers.push(newContainer);

        return true;
      }
    }
  }
  return false;
}

template<typename T>
T& Reader::ReadValue(TAG t, StringView name)
{
  ContainerInfo& container = containers.top();
  if (container.type == TAG::List)
  {
    return (dataStore.Pool<T>().data() + container.PoolIndex(dataStore))[container.currentIndex - 1];
  }
  if (container.type == TAG::Compound)
  {
    // TODO: if this ever becomes a performance issue, look at changing the vector to a set
    for (auto tagIndex : dataStore.compoundStorage[container.Storage(dataStore)])
    {
      auto& tag = dataStore.namedTags[tagIndex];
      if (tag.name == name)
      {
        assert(tag.type == t);
        return tag.As<T>();
      }
    }
  }
  assert(!"Internal Error - This should never happen");
  return T{};
}

} // namespace ImNBT

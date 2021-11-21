#include <ImNBT/NBTReader.hpp>

#include "byteswapping.h"

#include "zlib.h"

#include <array>
#include <cassert>
#include <iostream>

namespace ImNBT
{

//void Reader::EnterRoot()
//{
//  AddToCurrentName(root_name);
//  auto const found = named_tags_.find(current_name_);
//  if (found != named_tags_.end())
//  {
//    nesting_info_.emplace(
//        NestingInfo{ TAG::End, NestingInfo::ContainerType::Compound, 0, 0 });
//    return;
//  }
//  PopLatestName();
//}

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

//bool Reader::OpenCompound(StringView name)
//{
//  if (!HandleNesting(name, TAG::Compound))
//  {
//    return false;
//  }
//  auto const found = named_tags_.find(current_name_);
//  if (found != named_tags_.end())
//  {
//    nesting_info_.emplace(
//        NestingInfo{ TAG::End, NestingInfo::ContainerType::Compound, 0, 0 });
//    return true;
//  }
//  PopLatestName();
//  return false;
//}
//
//void Reader::CloseCompound()
//{
//  NestingInfo& nesting = nesting_info_.top();
//  if (nesting.container_type == NestingInfo::ContainerType::Compound)
//  {
//    nesting_info_.pop();
//    PopLatestName();
//    return;
//  }
//  assert(!"Reader : Compound Close Mismatch - Attempted to close a compound when a compound was not open.\n");
//}
//
//bool Reader::OpenList(StringView name)
//{
//  if (!HandleNesting(name, TAG::List))
//  {
//    return false;
//  }
//  auto const found = named_tags_.find(current_name_);
//  if (found != named_tags_.end())
//  {
//    nesting_info_.emplace(
//        NestingInfo{ found->second.As<TagPayload::List>().elementType_,
//                     NestingInfo::ContainerType::List,
//                     found->second.As<TagPayload::List>().count_, 0 });
//    return true;
//  }
//  PopLatestName();
//  return false;
//}
//
//int32_t Reader::ListSize()
//{
//  NestingInfo& nesting = nesting_info_.top();
//  if (nesting.container_type == NestingInfo::ContainerType::List)
//  {
//    return nesting.length;
//  }
//  assert(!"Reader : Invalid List Size Read - Attempted to read a list's size when a list was not open.\n");
//  return -1;
//}
//
//void Reader::CloseList()
//{
//  NestingInfo& nesting = nesting_info_.top();
//  if (nesting.container_type == NestingInfo::ContainerType::List)
//  {
//    nesting_info_.pop();
//    PopLatestName();
//    return;
//  }
//  assert(!"Reader : List Close Mismatch - Attempted to close a list when a list was not open.\n");
//}
//
//int8_t Reader::ReadByte(StringView name)
//{
//  HandleNesting(name, TAG::Byte);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  assert(found->second.type == TAG::Byte);
//  return found->second.As<byte>();
//}
//
//int16_t Reader::ReadShort(StringView name)
//{
//  HandleNesting(name, TAG::Short);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  assert(found->second.type == TAG::Short);
//  return found->second.As<int16_t>();
//}
//
//int32_t Reader::ReadInt(StringView name)
//{
//  HandleNesting(name, TAG::Int);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  assert(found->second.type == TAG::Int);
//  return found->second.As<int32_t>();
//}
//
//int64_t Reader::ReadLong(StringView name)
//{
//  HandleNesting(name, TAG::Long);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  assert(found->second.type == TAG::Long);
//  return found->second.As<int64_t>();
//}
//
//float Reader::ReadFloat(StringView name)
//{
//  HandleNesting(name, TAG::Float);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  assert(found->second.type == TAG::Float);
//  return found->second.As<float>();
//}
//
//double Reader::ReadDouble(StringView name)
//{
//  HandleNesting(name, TAG::Double);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  assert(found->second.type == TAG::Double);
//  return found->second.As<double>();
//}
//
//std::vector<int8_t> Reader::ReadByteArray(StringView name)
//{
//  HandleNesting(name, TAG::Byte_Array);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  assert(found->second.type == TAG::Byte_Array);
//  std::vector<int8_t> ret{
//    byte_array_pool_.data() +
//        found->second.As<TagPayload::ByteArray>().poolIndex_,
//    byte_array_pool_.data() +
//        found->second.As<TagPayload::ByteArray>().count_
//  };
//  return ret;
//}
//
//StringView Reader::ReadString(StringView name)
//{
//  HandleNesting(name, TAG::String);
//  auto const found = named_tags_.find(current_name_);
//  PopLatestName();
//  assert(found->second.type == TAG::String);
//  return StringView{
//    string_pool_.data() + found->second.As<TagPayload::String>().poolIndex_,
//    static_cast<size_t>(found->second.As<TagPayload::String>().length_)
//  };
//}
//
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

//template<>
//int8_t Reader::Read(StringView name)
//{
//  return ReadByte(name);
//}
//template<>
//int16_t Reader::Read(StringView name)
//{
//  return ReadShort(name);
//}
//template<>
//int32_t Reader::Read(StringView name)
//{
//  return ReadInt(name);
//}
//template<>
//int64_t Reader::Read(StringView name)
//{
//  return ReadLong(name);
//}
//template<>
//float Reader::Read(StringView name)
//{
//  return ReadFloat(name);
//}
//template<>
//double Reader::Read(StringView name)
//{
//  return ReadDouble(name);
//}
//template<>
//std::vector<int8_t> Reader::Read(StringView name)
//{
//  return ReadByteArray(name);
//}
//template<>
//StringView Reader::Read(StringView name)
//{
//  return ReadString(name);
//}
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

//bool Reader::HandleNesting(StringView name, TAG t)
//{
//  NestingInfo& nesting = nesting_info_.top();
//  // Lists have strict requirements
//  if (nesting.container_type == NestingInfo::ContainerType::List)
//  {
//    if (!name.empty())
//    {
//      // problems, lists cannot have named tags
//      assert(!"Reader : List Named Read - Attempted to read named tag from a list.");
//      return false;
//    }
//    if (nesting.data_type != t)
//    {
//      if (nesting.length != 0)
//      {
//        assert(!"Reader : List Type Mismatch - Attempted to read the wrong type from a list.");
//        return false;
//      }
//    }
//    else
//    {
//      if (nesting.list_index >= nesting.length)
//      {
//        assert(!"Reader : List Overread - Attempted to read too many items from a list.");
//        return false;
//      }
//      AddIndexToCurrentName(nesting.list_index);
//      ++(nesting.list_index);
//    }
//  }
//  else if (nesting.container_type == NestingInfo::ContainerType::Compound)
//  {
//    if (name.empty())
//    {
//      // bad, compound tags must have names
//      assert(!"Reader : Compound Unnamed Read - Attempted to read unnamed tag from a compound.");
//      return false;
//    }
//    AddToCurrentName(name);
//  }
//  return true;
//}
//
//void Reader::ParseDataTree()
//{
//  // parse root tag
//  TAG const type = ReadTag();
//  assert(type == TAG::Compound && "root compound tag exists.");
//  root_name = "root:" + ReadName();
//
//  AddToCurrentName(root_name);
//
//  ParseDataTagUnnamed(type);
//
//  PopLatestName();
//
//  current_name_.clear();
//}
//
//DataTag& Reader::ParseDataTag()
//{
//  std::string name;
//
//  TAG const type = ReadTag();
//  if (type != TAG::End)
//  {
//    name = ReadName();
//  }
//  AddToCurrentName(name);
//  DataTag& tag = ParseDataTagUnnamed(type);
//  PopLatestName();
//
//  return tag;
//}
//
//DataTag& Reader::ParseDataTagUnnamed(TAG type)
//{
//  DataTag data_tag{ type };
//  switch (data_tag.type)
//  {
//    case TAG::List: {
//      ++parsing_nesting_depth_;
//      auto elementType = ReadTag();
//      auto count = ReadArrayLen();
//      data_tag.Set<TagPayload::List>();
//      data_tag.As<TagPayload::List>().elementType_ = elementType;
//      data_tag.As<TagPayload::List>().count_ = count;
//      // read 'count' unnamed elements
//      for (int i = 0; i < count; ++i)
//      {
//        AddIndexToCurrentName(i);
//        named_tags_.emplace(current_name_, ParseDataTagUnnamed(elementType));
//        PopLatestName();
//      }
//      --parsing_nesting_depth_;
//    }
//    break;
//    case TAG::Compound: {
//      ++parsing_nesting_depth_;
//      do
//      {
//      } while (ParseDataTag().type != TAG::End);
//    }
//    break;
//    case TAG::End: {
//      --parsing_nesting_depth_;
//    }
//    break;
//    case TAG::Byte: {
//      int8_t val;
//      fread(&val, sizeof(val), 1, infile_);
//      data_tag.Set<byte>(byte{ val });
//    }
//    break;
//    case TAG::Short: {
//      int16_t val;
//      fread(&val, sizeof(val), 1, infile_);
//      data_tag.Set<int16_t>(swap_i16(val));
//    }
//    break;
//    case TAG::Int: {
//      int32_t val;
//      fread(&val, sizeof(val), 1, infile_);
//      data_tag.Set<int32_t>(swap_i32(val));
//    }
//    break;
//    case TAG::Long: {
//      int64_t val;
//      fread(&val, sizeof(val), 1, infile_);
//      data_tag.Set<int64_t>(swap_i64(val));
//    }
//    break;
//    case TAG::Float: {
//      float val;
//      fread(&val, sizeof(val), 1, infile_);
//      data_tag.Set<float>(swap_f32(val));
//    }
//    break;
//    case TAG::Double: {
//      double val;
//      fread(&val, sizeof(val), 1, infile_);
//      data_tag.Set<double>(swap_f64(val));
//    }
//    break;
//    case TAG::String: {
//      int16_t const length = ReadStrLen();
//      size_t const insertion_point = string_pool_.size();
//      string_pool_.resize(insertion_point + length + 1, '\0');
//      fread(string_pool_.data() + insertion_point, sizeof(char), length, infile_);
//      data_tag.Set<TagPayload::String>();
//      data_tag.As<TagPayload::String>().poolIndex_ = insertion_point;
//      data_tag.As<TagPayload::String>().length_ = length;
//    }
//    break;
//    case TAG::Byte_Array: {
//      int32_t const length = ReadArrayLen();
//      size_t const insertion_point = byte_array_pool_.size();
//      byte_array_pool_.resize(insertion_point + length + 1, 0);
//      fread(byte_array_pool_.data() + insertion_point, sizeof(char), length,
//            infile_);
//      data_tag.Set<TagPayload::ByteArray>();
//      data_tag.As<TagPayload::ByteArray>().poolIndex_ = insertion_point;
//      data_tag.As<TagPayload::ByteArray>().count_ = length;
//    }
//    break;
//    default:
//      break;
//  }
//
//  auto placed = named_tags_.emplace(current_name_, data_tag);
//
//  return placed.first->second;
//}
//
//TAG Reader::ReadTag()
//{
//  TAG t;
//  fread(&t, sizeof(t), 1, infile_);
//  return t;
//}
//
//std::string Reader::ReadName()
//{
//  int16_t const length = ReadStrLen();
//  if (length != 0)
//  {
//    std::string name(static_cast<size_t>(length) + 1, '\0');
//    fread(name.data(), sizeof(std::string::value_type), length, infile_);
//    return name;
//  }
//  return "";
//}
//
//void Reader::AddToCurrentName(StringView name)
//{
//  if (!(current_name_.empty()))
//  {
//    current_name_ += 0x1F;
//  }
//  current_name_ += name.data();
//}
//
//void Reader::AddIndexToCurrentName(int32_t index)
//{
//  std::string indexed;
//  indexed.append("[");
//  indexed.append(std::to_string(index));
//  indexed.append("]");
//  AddToCurrentName(indexed);
//}
//
//void Reader::PopLatestName()
//{
//  size_t const last_dot = current_name_.rfind(0x1F);
//  if (last_dot != std::string::npos)
//  {
//    current_name_.resize(last_dot);
//  }
//  else
//  {
//    current_name_.clear();
//  }
//}

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

int32_t Reader::RetrieveBinaryArrayLen()
{
  return swap_i32(memoryStream.Retrieve<int32_t>());
}

TAG Reader::RetrieveBinaryTag()
{
  return memoryStream.Retrieve<TAG>();
}

std::string Reader::RetrieveBinaryStr()
{
  auto const len = swap_i16(memoryStream.Retrieve<int16_t>());
  std::string str(memoryStream.RetrieveRangeView<char>(len), len);
  return str;
}

} // namespace ImNBT

#include <ImNBT/NBTReader.hpp>

#include "byteswapping.h"

#include <cassert>
#include <iostream>

namespace ImNBT
{

Reader::Reader(StringView filepath) : infile_ {nullptr}
{
  infile_ = fopen(filepath.data(), "rb");
  assert(infile_);

  ParseDataTree();

  EnterRoot();
}

Reader::~Reader()
{
  CloseCompound();
  fclose(infile_);
}

void Reader::EnterRoot()
{
  AddToCurrentName(root_name);
  auto const found = named_tags_.find(current_name_);
  if (found != named_tags_.end())
  {
    nesting_info_.emplace(NestingInfo {TAG::End, NestingInfo::ContainerType::Compound, 0, 0});
    return;
  }
  PopLatestName();
}

bool Reader::OpenCompound(StringView name)
{
  if (!HandleNesting(name, TAG::Compound))
  {
    return false;
  }
  auto const found = named_tags_.find(current_name_);
  if (found != named_tags_.end())
  {
    nesting_info_.emplace(NestingInfo {TAG::End, NestingInfo::ContainerType::Compound, 0, 0});
    return true;
  }
  PopLatestName();
  return false;
}

void Reader::CloseCompound()
{
  NestingInfo& nesting = nesting_info_.top();
  if (nesting.container_type == NestingInfo::ContainerType::Compound)
  {
    nesting_info_.pop();
    PopLatestName();
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
  auto const found = named_tags_.find(current_name_);
  if (found != named_tags_.end())
  {
    nesting_info_.emplace(
          NestingInfo{ found->second.list_.elementType_, NestingInfo::ContainerType::List, found->second.list_.count_, 0 });
    return true;
  }
  PopLatestName();
  return false;
}

int32_t Reader::ListSize()
{
  NestingInfo& nesting = nesting_info_.top();
  if (nesting.container_type == NestingInfo::ContainerType::List)
  {
    return nesting.length;
  }
  assert(!"Reader : Invalid List Size Read - Attempted to read a list's size when a list was not open.\n");
  return -1;
}

void Reader::CloseList()
{
  NestingInfo& nesting = nesting_info_.top();
  if (nesting.container_type == NestingInfo::ContainerType::List)
  {
    nesting_info_.pop();
    PopLatestName();
    return;
  }
  assert(!"Reader : List Close Mismatch - Attempted to close a list when a list was not open.\n");
}

int8_t Reader::ReadByte(StringView name)
{
  HandleNesting(name, TAG::Byte);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type == TAG::Byte);
  return found->second.byte_;
}

int16_t Reader::ReadShort(StringView name)
{
  HandleNesting(name, TAG::Short);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type == TAG::Short);
  return found->second.short_;
}

int32_t Reader::ReadInt(StringView name)
{
  HandleNesting(name, TAG::Int);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type == TAG::Int);
  return found->second.int_;
}

int64_t Reader::ReadLong(StringView name)
{
  HandleNesting(name, TAG::Long);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type == TAG::Long);
  return found->second.long_;
}

float Reader::ReadFloat(StringView name)
{
  HandleNesting(name, TAG::Float);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type == TAG::Float);
  return found->second.float_;
}

double Reader::ReadDouble(StringView name)
{
  HandleNesting(name, TAG::Double);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type == TAG::Double);
  return found->second.double_;
}

std::vector<int8_t> Reader::ReadByteArray(StringView name)
{
  HandleNesting(name, TAG::Byte_Array);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type == TAG::Byte_Array);
  std::vector<int8_t> ret {
      byte_array_pool_.data() + found->second.byteArray_.poolIndex_,
      byte_array_pool_.data() + found->second.byteArray_.count_
  };
  return ret;
}

StringView Reader::ReadString(StringView name)
{
  HandleNesting(name, TAG::String);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type == TAG::String);
  return StringView {
      string_pool_.data() + found->second.string_.poolIndex_,
      static_cast<size_t>(found->second.string_.length_)
  };
}

Optional<int8_t> Reader::MaybeReadByte(StringView name)
{
  HandleNesting(name, TAG::Byte);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  if (found != named_tags_.end())
  {
    assert(found->second.type == TAG::Byte);
      return std::make_optional(found->second.byte_);
  }
  return std::nullopt;
}

Optional<int16_t> Reader::MaybeReadShort(StringView name)
{
  HandleNesting(name, TAG::Short);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  if (found != named_tags_.end())
  {
    assert(found->second.type == TAG::Short);
      return std::make_optional(found->second.short_);
  }
  return std::nullopt;
}

Optional<int32_t> Reader::MaybeReadInt(StringView name)
{
  HandleNesting(name, TAG::Int);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  if (found != named_tags_.end())
  {
    assert(found->second.type == TAG::Int);
      return std::make_optional(found->second.int_);
  }
  return std::nullopt;
}

Optional<int64_t> Reader::MaybeReadLong(StringView name)
{
  HandleNesting(name, TAG::Long);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  if (found != named_tags_.end())
  {
    assert(found->second.type == TAG::Long);
      return std::make_optional(found->second.long_);
  }
  return std::nullopt;
}

Optional<float> Reader::MaybeReadFloat(StringView name)
{
  HandleNesting(name, TAG::Float);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  if (found != named_tags_.end())
  {
    assert(found->second.type == TAG::Float);
      return std::make_optional(found->second.float_);
  }
  return std::nullopt;
}

Optional<double> Reader::MaybeReadDouble(StringView name)
{
  HandleNesting(name, TAG::Double);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  if (found != named_tags_.end())
  {
    assert(found->second.type == TAG::Double);
      return std::make_optional(found->second.double_);
  }
  return std::nullopt;
}

Optional<std::vector<int8_t>> Reader::MaybeReadByteArray(StringView name)
{
  HandleNesting(name, TAG::Byte_Array);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  if (found != named_tags_.end())
  {
    assert(found->second.type == TAG::Byte_Array);
    std::vector<int8_t> ret {
      byte_array_pool_.data() + found->second.byteArray_.poolIndex_,
      byte_array_pool_.data() + found->second.byteArray_.count_
    };
    return std::make_optional(ret);
  }
  return std::nullopt;
}

Optional<StringView> Reader::MaybeReadString(StringView name)
{
  HandleNesting(name, TAG::String);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  if (found != named_tags_.end())
  {
    assert(found->second.type == TAG::String);
    return std::make_optional<StringView>(
        string_pool_.data() + found->second.string_.poolIndex_,
        static_cast<size_t>(found->second.string_.length_));
  }
  return std::nullopt;
}

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
StringView Reader::Read(StringView name)
{
  return ReadString(name);
}

template<>
Optional<int8_t> Reader::MaybeRead(StringView name)
{
  return MaybeReadByte(name);
}
template<>
Optional<int16_t> Reader::MaybeRead(StringView name)
{
  return MaybeReadShort(name);
}
template<>
Optional<int32_t> Reader::MaybeRead(StringView name)
{
  return MaybeReadInt(name);
}
template<>
Optional<int64_t> Reader::MaybeRead(StringView name)
{
  return MaybeReadLong(name);
}
template<>
Optional<float> Reader::MaybeRead(StringView name)
{
  return MaybeReadFloat(name);
}
template<>
Optional<double> Reader::MaybeRead(StringView name)
{
  return MaybeReadDouble(name);
}

bool Reader::HandleNesting(StringView name, TAG t)
{
  NestingInfo& nesting = nesting_info_.top();
  // Lists have strict requirements
  if (nesting.container_type == NestingInfo::ContainerType::List)
  {
    if (!name.empty())
    {
      // problems, lists cannot have named tags
      assert(!"Reader : List Named Read - Attempted to read named tag from a list.");
      return false;
    }
    if (nesting.data_type != t)
    {
      if (nesting.length != 0)
      {
          assert(!"Reader : List Type Mismatch - Attempted to read the wrong type from a list.");
          return false;
      }
    }
    else
    {
      if (nesting.list_index >= nesting.length)
      {
        assert(!"Reader : List Overread - Attempted to read too many items from a list.");
        return false;
      }
      AddIndexToCurrentName(nesting.list_index);
      ++(nesting.list_index);
    }
  }
  else if (nesting.container_type == NestingInfo::ContainerType::Compound)
  {
    if (name.empty())
    {
      // bad, compound tags must have names
      assert(!"Reader : Compound Unnamed Read - Attempted to read unnamed tag from a compound.");
      return false;
    }
    AddToCurrentName(name);
  }
  return true;
}

void Reader::ParseDataTree()
{
  // parse root tag
  TAG const type = ReadTag();
  assert(type == TAG::Compound && "root compound tag exists.");
  root_name = "root:" + ReadName();

  AddToCurrentName(root_name);

  ParseDataTagUnnamed(type);

  PopLatestName();

  current_name_.clear();
}

DataTag& Reader::ParseDataTag()
{
  std::string name;

  TAG const type = ReadTag();
  if (type != TAG::End)
  {
    name = ReadName();
  }
  AddToCurrentName(name);
  DataTag& tag = ParseDataTagUnnamed(type);
  PopLatestName();

  return tag;
}

DataTag& Reader::ParseDataTagUnnamed(TAG type)
{
  DataTag data_tag {type};
  switch (data_tag.type)
  {
  case TAG::List:
  {
    ++parsing_nesting_depth_;
    data_tag.list_.elementType_ = ReadTag();
    data_tag.list_.count_ = ReadArrayLen();
    // read len unnamed elements
    for (int i = 0; i < data_tag.list_.count_; ++i)
    {
      AddIndexToCurrentName(i);
        named_tags_.emplace(current_name_, ParseDataTagUnnamed(data_tag.list_.elementType_));
      PopLatestName();
    }
    --parsing_nesting_depth_;
  }
  break;
  case TAG::Compound:
  {
    ++parsing_nesting_depth_;
    do
    {
    } while (ParseDataTag().type != TAG::End);
  }
  break;
  case TAG::End:
  {
    --parsing_nesting_depth_;
  }
  break;
  case TAG::Byte:
  {
    int8_t val;
    fread(&val, sizeof(val), 1, infile_);
    data_tag.byte_ = val;
  }
  break;
  case TAG::Short:
  {
    int16_t val;
    fread(&val, sizeof(val), 1, infile_);
    data_tag.short_ = swap_i16(val);
  }
  break;
  case TAG::Int:
  {
    int32_t val;
    fread(&val, sizeof(val), 1, infile_);
    data_tag.int_ = swap_i32(val);
  }
  break;
  case TAG::Long:
  {
    int64_t val;
    fread(&val, sizeof(val), 1, infile_);
    data_tag.long_ = swap_i64(val);
  }
  break;
  case TAG::Float:
  {
    float val;
    fread(&val, sizeof(val), 1, infile_);
    data_tag.float_ = swap_f32(val);
  }
  break;
  case TAG::Double:
  {
    double val;
    fread(&val, sizeof(val), 1, infile_);
    data_tag.double_ = swap_f64(val);
  }
  break;
  case TAG::String:
  {
    int16_t const length = ReadStrLen();
    size_t const insertion_point = string_pool_.size();
    string_pool_.resize(insertion_point + length + 1, '\0');
    fread(string_pool_.data() + insertion_point, sizeof(char), length, infile_);
    data_tag.string_.poolIndex_ = insertion_point;
    data_tag.string_.length_ = length;
  }
  break;
  case TAG::Byte_Array:
  {
    int32_t const length = ReadArrayLen();
    size_t const insertion_point = byte_array_pool_.size();
    byte_array_pool_.resize(insertion_point + length + 1, 0);
    fread(byte_array_pool_.data() + insertion_point, sizeof(char), length, infile_);
    data_tag.byteArray_.poolIndex_ = insertion_point;
    data_tag.byteArray_.count_ = length;
  }
  break;
  default:
    break;
  }

  auto placed = named_tags_.emplace(current_name_, data_tag);

  return placed.first->second;
}

TAG Reader::ReadTag()
{
  TAG t;
  fread(&t, sizeof(t), 1, infile_);
  return t;
}

std::string Reader::ReadName()
{
  int16_t const length = ReadStrLen();
  if (length != 0)
  {
    std::string name(static_cast<size_t>(length) + 1, '\0');
    fread(name.data(), sizeof(std::string::value_type), length, infile_);
    return name;
  }
  return "";
}

void Reader::AddToCurrentName(StringView name)
{
  if (!(current_name_.empty()))
  {
    current_name_ += 0x1F;
  }
  current_name_ += name.data();
}

void Reader::AddIndexToCurrentName(int32_t index)
{
  std::string indexed;
  indexed.append("[");
  indexed.append(std::to_string(index));
  indexed.append("]");
  AddToCurrentName(indexed);
}

void Reader::PopLatestName()
{
  size_t const last_dot = current_name_.rfind(0x1F);
  if (last_dot != std::string::npos)
  {
    current_name_.resize(last_dot);
  }
  else
  {
    current_name_.clear();
  }
}

int16_t Reader::ReadStrLen()
{
  int16_t len;
  fread(&len, sizeof(len), 1, infile_);
  return swap_i16(len);
}

int32_t Reader::ReadArrayLen()
{
  int32_t len;
  fread(&len, sizeof(len), 1, infile_);
  return swap_i32(len);
}

} // namespace ImNBT

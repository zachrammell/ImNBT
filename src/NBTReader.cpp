#include <ImNBT/NBTReader.hpp>

#include "byteswapping.h"

#include <cassert>
#include <iostream>

namespace ImNBT
{

Reader::Reader(StringView filepath) : infile_ {nullptr}
{
  infile_ = fopen(filepath.data(), "rb");
  if (!infile_)
  {
    // TODO: actual error handling
    std::cerr << "Couldn't open NBT file " << filepath << std::endl;
    return;
  }

  std::clog << "Loading NBT file " << filepath << std::endl;

  ParseDataTree();

  std::clog << "Parsed NBT file " << filepath << std::endl;

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
  std::cerr << "Reader : Compound Close Mismatch - Attempted to close a compound when a list was not open.\n";
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
      NestingInfo {found->second.list_type_, NestingInfo::ContainerType::List, found->second.list_length_, 0});
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
  std::cerr << "Reader : Invalid List Size Read - Attempted to read list size when a list was not open.\n";
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
  std::cerr << "Reader : List Close Mismatch - Attempted to close a list when a list was not open.\n";
}

int8_t Reader::ReadByte(StringView name)
{
  HandleNesting(name, TAG::Byte);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type_ == TAG::Byte);
  return found->second.byte_;
}

int16_t Reader::ReadShort(StringView name)
{
  HandleNesting(name, TAG::Short);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type_ == TAG::Short);
  return found->second.short_;
}

int32_t Reader::ReadInt(StringView name)
{
  HandleNesting(name, TAG::Int);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type_ == TAG::Int);
  return found->second.int_;
}

int64_t Reader::ReadLong(StringView name)
{
  HandleNesting(name, TAG::Long);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type_ == TAG::Long);
  return found->second.long_;
}

float Reader::ReadFloat(StringView name)
{
  HandleNesting(name, TAG::Float);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type_ == TAG::Float);
  return found->second.float_;
}

double Reader::ReadDouble(StringView name)
{
  HandleNesting(name, TAG::Double);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type_ == TAG::Double);
  return found->second.double_;
}

std::vector<int8_t> Reader::ReadByteArray(StringView name)
{
  HandleNesting(name, TAG::Byte_Array);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type_ == TAG::Byte_Array);
  std::vector<int8_t> ret {
    byte_array_pool_.begin() + found->second.byte_array_pool_index,
    byte_array_pool_.begin() + found->second.byte_array_length_};
  return ret;
}

StringView Reader::ReadString(StringView name)
{
  HandleNesting(name, TAG::String);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  assert(found->second.type_ == TAG::String);
  return StringView {
    string_pool_.data() + found->second.string_pool_index,
    static_cast<size_t>(found->second.string_length_)};
}

Optional<int8_t> Reader::MaybeReadByte(StringView name)
{
  HandleNesting(name, TAG::Byte);
  auto const found = named_tags_.find(current_name_);
  PopLatestName();
  if (found != named_tags_.end())
  {
    assert(found->second.type_ == TAG::Byte);
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
    assert(found->second.type_ == TAG::Short);
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
    assert(found->second.type_ == TAG::Int);
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
    assert(found->second.type_ == TAG::Long);
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
    assert(found->second.type_ == TAG::Float);
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
    assert(found->second.type_ == TAG::Double);
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
    assert(found->second.type_ == TAG::Byte_Array);
    std::vector<int8_t> ret {
      byte_array_pool_.begin() + found->second.byte_array_pool_index,
      byte_array_pool_.begin() + found->second.byte_array_length_};
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
    assert(found->second.type_ == TAG::String);
    return std::make_optional<StringView>(
      string_pool_.data() + found->second.string_pool_index,
      static_cast<size_t>(found->second.string_length_));
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
      assert(0);
    }
    if (nesting.data_type != t)
    {
      if (nesting.length == 0)
      {
      }
      else
      {
      }
    }
    else
    {
      if (nesting.list_index >= nesting.length)
      {
        // read too many from list
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
      assert(0);
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

Reader::DataTag& Reader::ParseDataTag()
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

Reader::DataTag& Reader::ParseDataTagUnnamed(TAG type)
{
  DataTag data_tag {type};
  switch (data_tag.type_)
  {
  case TAG::List:
  {
    ++parsing_nesting_depth_;
    data_tag.list_type_ = ReadTag();
    data_tag.list_length_ = ReadArrayLen();
    // read len unnamed elements
    for (int i = 0; i < data_tag.list_length_; ++i)
    {
      AddIndexToCurrentName(i);
      named_tags_.emplace(current_name_, ParseDataTagUnnamed(data_tag.list_type_));
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
    } while (ParseDataTag().type_ != TAG::End);
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
    fread_s(&val, sizeof(val), sizeof(val), 1, infile_);
    data_tag.byte_ = val;
  }
  break;
  case TAG::Short:
  {
    int16_t val;
    fread_s(&val, sizeof(val), sizeof(val), 1, infile_);
    data_tag.short_ = swap_i16(val);
  }
  break;
  case TAG::Int:
  {
    int32_t val;
    fread_s(&val, sizeof(val), sizeof(val), 1, infile_);
    data_tag.int_ = swap_i32(val);
  }
  break;
  case TAG::Long:
  {
    int64_t val;
    fread_s(&val, sizeof(val), sizeof(val), 1, infile_);
    data_tag.long_ = swap_i64(val);
  }
  break;
  case TAG::Float:
  {
    float val;
    fread_s(&val, sizeof(val), sizeof(val), 1, infile_);
    data_tag.float_ = swap_f32(val);
  }
  break;
  case TAG::Double:
  {
    double val;
    fread_s(&val, sizeof(val), sizeof(val), 1, infile_);
    data_tag.double_ = swap_f64(val);
  }
  break;
  case TAG::String:
  {
    int16_t const length = ReadStrLen();
    size_t const insertion_point = string_pool_.size();
    string_pool_.resize(insertion_point + length + 1, '\0');
    fread_s(string_pool_.data() + insertion_point, length, sizeof(char), length, infile_);
    data_tag.string_pool_index = insertion_point;
    data_tag.string_length_ = length;
  }
  break;
  case TAG::Byte_Array:
  {
    int32_t const length = ReadArrayLen();
    size_t const insertion_point = byte_array_pool_.size();
    byte_array_pool_.resize(insertion_point + length + 1, 0);
    fread_s(byte_array_pool_.data() + insertion_point, length, sizeof(char), length, infile_);
    data_tag.byte_array_pool_index = insertion_point;
    data_tag.byte_array_length_ = length;
  }
  break;
  default:
    __debugbreak(); // ask Zach to stop being lazy and implement deserialization for the right primitive type
    break;
  }

  auto placed = named_tags_.emplace(current_name_, data_tag);

  return placed.first->second;
}

Reader::TAG Reader::ReadTag()
{
  TAG t;
  fread_s(&t, sizeof(t), sizeof(t), 1, infile_);
  return t;
}

std::string Reader::ReadName()
{
  int16_t const length = ReadStrLen();
  if (length != 0)
  {
    std::string name(static_cast<size_t>(length) + 1, '\0');
    fread_s(name.data(), length, sizeof(std::string::value_type), length, infile_);
    return name;
  }
  return "";
}

void Reader::AddToCurrentName(StringView name)
{
  if (!(current_name_.empty()))
  {
    current_name_ += '.';
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
  size_t const last_dot = current_name_.rfind('.');
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
  fread_s(&len, sizeof(len), sizeof(len), 1, infile_);
  return swap_i16(len);
}

int32_t Reader::ReadArrayLen()
{
  int32_t len;
  fread_s(&len, sizeof(len), sizeof(len), 1, infile_);
  return swap_i32(len);
}

} // namespace Octane

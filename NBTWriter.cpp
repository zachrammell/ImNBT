#include "NBTWriter.hpp"

#include "byteswapping.h"

#include <cassert>
#include <iostream>

namespace ImNBT
{

  Writer::Writer(string_view filepath) : outfile_{nullptr}
  {
    outfile_ = fopen(filepath.data(), "wb");
    if (!outfile_)
    {
      // TODO: actual error handling
      std::cerr << "Couldn't open NBT file " << filepath << std::endl;
      return;
    }
    BeginRoot();
  }

  Writer::~Writer()
  {
    EndCompound();
    if (!nesting_info_.empty())
    {
      std::cerr << "Writer : Mismatched Compound Begin / End - A Compound Tag was not closed.\n";
    }
    fclose(outfile_);
  }

  bool Writer::BeginCompound(string_view name)
  {
    HandleNesting(name, TAG::Compound);
    nesting_info_.emplace(NestingInfo{TAG::End, NestingInfo::ContainerType::Compound, 0, 0});
    if (!name.empty())
    {
      WriteTag(TAG::Compound);
      WriteStr(name);
    }
    return true;
  }

  void Writer::EndCompound()
  {
    NestingInfo &nesting = nesting_info_.top();
    if (nesting.container_type != NestingInfo::ContainerType::Compound)
    {
      std::cerr << "Writer: Mismatched Compound Begin/End - Compound Tag closed without being opened.\n";
    }
    if (nesting.length == 0)
    {
      std::cerr << "Writer: Empty Compound - Compound Tag with no values.\n";
    }
    nesting_info_.pop();
    WriteTag(TAG::End);
  }

  bool Writer::BeginList(string_view name)
  {
    HandleNesting(name, TAG::List);
    //  NestingInfo& nesting = nesting_info_.top();
    if (!name.empty())
    {
      WriteTag(TAG::List);
      WriteStr(name);
    }
    // Store the file offset corresponding to the length, so that when ending the list we can write the length in here
    fpos_t current_file_pos;
    fgetpos(outfile_, &current_file_pos);
    // The following two writes are not valid, they are simply filling space to be written in when the list is finished
    WriteTag(TAG::INVALID);
    WriteArrayLen(0xCCCCCCCC);
    nesting_info_.emplace(NestingInfo{TAG::End, NestingInfo::ContainerType::List, 0, current_file_pos});
    return true;
  }

  void Writer::EndList()
  {
    NestingInfo &nesting = nesting_info_.top();
    if (nesting.container_type != NestingInfo::ContainerType::List)
    {
      std::cerr << "Writer: Mismatched List Begin/End - List Tag closed without being opened.\n";
    }
    if (nesting.length == 0)
    {
      std::cerr << "Writer: Empty List - List Tag with no values.\n";
    }

    fpos_t current_file_pos;
    fgetpos(outfile_, &current_file_pos);

    fsetpos(outfile_, &(nesting.file_pos));
    WriteTag(nesting.data_type);
    WriteArrayLen(nesting.length);

    fsetpos(outfile_, &current_file_pos);

    nesting_info_.pop();
  }

  void Writer::WriteByte(int8_t b, string_view name)
  {
    HandleNesting(name, TAG::Byte);
    if (!name.empty())
    {
      WriteTag(TAG::Byte);
      WriteStr(name);
    }
    fwrite(&b, sizeof(b), 1, outfile_);
  }

  void Writer::WriteShort(int16_t s, string_view name)
  {
    HandleNesting(name, TAG::Short);
    if (!name.empty())
    {
      WriteTag(TAG::Short);
      WriteStr(name);
    }
    uint16_t const s_big_endian = swap_u16(s);
    fwrite(&s_big_endian, sizeof(s_big_endian), 1, outfile_);
  }

  void Writer::WriteInt(int32_t i, string_view name)
  {
    HandleNesting(name, TAG::Int);
    if (!name.empty())
    {
      WriteTag(TAG::Int);
      WriteStr(name);
    }
    uint32_t const i_big_endian = swap_u32(i);
    fwrite(&i_big_endian, sizeof(i_big_endian), 1, outfile_);
  }

  void Writer::WriteLong(int64_t l, string_view name)
  {
    HandleNesting(name, TAG::Long);
    if (!name.empty())
    {
      WriteTag(TAG::Long);
      WriteStr(name);
    }
    uint64_t const l_big_endian = swap_u64(l);
    fwrite(&l_big_endian, sizeof(l_big_endian), 1, outfile_);
  }

  void Writer::WriteFloat(float f, string_view name)
  {
    HandleNesting(name, TAG::Float);
    if (!name.empty())
    {
      WriteTag(TAG::Float);
      WriteStr(name);
    }
    float const f_big_endian = swap_f32(f);
    fwrite(&f_big_endian, sizeof(f_big_endian), 1, outfile_);
  }

  void Writer::WriteDouble(double d, string_view name)
  {
    HandleNesting(name, TAG::Double);
    if (!name.empty())
    {
      WriteTag(TAG::Double);
      WriteStr(name);
    }
    double const d_big_endian = swap_f64(d);
    fwrite(&d_big_endian, sizeof(d_big_endian), 1, outfile_);
  }

  void Writer::WriteByteArray(int8_t const *array, int32_t length, string_view name)
  {
    HandleNesting(name, TAG::Byte_Array);
    if (!name.empty())
    {
      WriteTag(TAG::Byte_Array);
      WriteStr(name);
    }
    WriteArrayLen(length);
    fwrite(&array, sizeof(array[0]), length, outfile_);
  }

  void Writer::WriteString(string_view str, string_view name)
  {
    HandleNesting(name, TAG::String);
    // Tag and name information is only written for named tags
    if (!name.empty())
    {
      WriteTag(TAG::String);
      WriteStr(name);
    }
    WriteStr(str);
  }

  // Write specializations

  template <>
  void Writer::Write(int8_t b, string_view name)
  {
    WriteByte(b, name);
  }
  template <>
  void Writer::Write(int16_t s, string_view name)
  {
    WriteShort(s, name);
  }
  template <>
  void Writer::Write(int32_t i, string_view name)
  {
    WriteInt(i, name);
  }
  template <>
  void Writer::Write(int64_t l, string_view name)
  {
    WriteLong(l, name);
  }
  template <>
  void Writer::Write(float f, string_view name)
  {
    WriteFloat(f, name);
  }
  template <>
  void Writer::Write(double d, string_view name)
  {
    WriteDouble(d, name);
  }
  template <>
  void Writer::Write(string_view str, string_view name)
  {
    WriteString(str, name);
  }

  // private implementations

  void Writer::WriteTag(TAG t)
  {
    fwrite(&t, sizeof(t), 1, outfile_);
  }

  void Writer::WriteStr(string_view name)
  {
    WriteStrLen(static_cast<uint16_t>(name.size()));
    fwrite(name.data(), sizeof(string_view::value_type), name.size(), outfile_);
  }

  void Writer::WriteStrLen(int16_t len)
  {
    uint16_t const len_big_endian = swap_u16(len);
    fwrite(&len_big_endian, sizeof(len_big_endian), 1, outfile_);
  }

  void Writer::WriteArrayLen(int32_t len)
  {
    uint32_t const len_big_endian = swap_u32(len);
    fwrite(&len_big_endian, sizeof(len_big_endian), 1, outfile_);
  }

  void Writer::BeginRoot()
  {
    nesting_info_.emplace(NestingInfo{TAG::End, NestingInfo::ContainerType::Compound, 0, 0});
    WriteTag(TAG::Compound);
    WriteStr("root");
  }

  void Writer::HandleNesting(string_view name, TAG t)
  {
    NestingInfo &nesting = nesting_info_.top();
    // Lists have strict requirements
    if (nesting.container_type == NestingInfo::ContainerType::List)
    {
      if (!name.empty())
      {
        std::cerr << "Writer: Name Error - Attempted to add a named tag to a List. Lists cannot contain named tags.\n";
      }
      // The list is not exclusively the same type as this tag
      if (nesting.data_type != t)
      {
        // If the list is currently empty, make it into a list of tags of this type
        if (nesting.length == 0)
        {
          nesting.data_type = t;
        }
        else
        {
          std::cerr << "Writer: Type Error - Attempted to add a tag to a list with tags of different type. All tags in a list must be of the same type.\n";
        }
      }
    }
    else if (nesting.container_type == NestingInfo::ContainerType::Compound)
    {
      if (name.empty())
      {
        std::cerr << "Writer: Structure Error - Attempted to add an unnamed tag to a compound. All tags in a compound must be named.\n";
      }
    }
    ++(nesting.length);
  }

} // namespace ImNBT

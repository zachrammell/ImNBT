#include <ImNBT/NBTWriter.hpp>

#include "byteswapping.h"

#include <cassert>
#include <iostream>

namespace ImNBT
{

Writer::Writer(StringView filepath) : outfile_{ nullptr }
{
    outfile_ = fopen(filepath.data(), "wb");
    assert(outfile_);
    BeginRoot();
}

Writer::~Writer()
{
    EndCompound();
    if (!nesting_info_.empty())
    {
        assert(!"Writer : Mismatched Compound Begin / End - A Compound Tag was not closed.\n");
    }
    fclose(outfile_);
}

bool Writer::BeginCompound(StringView name)
{
    HandleNesting(name, TAG::Compound);
    nesting_info_.emplace(NestingInfo{ TAG::End, NestingInfo::ContainerType::Compound, 0, 0 });
    if (!name.empty())
    {
        WriteTag(TAG::Compound);
        WriteStr(name);
    }
    return true;
}

void Writer::EndCompound()
{
    NestingInfo& nesting = nesting_info_.top();
    if (nesting.container_type != NestingInfo::ContainerType::Compound)
    {
        assert(!"Writer: Mismatched Compound Begin/End - Compound Tag closed without being opened.\n");
    }
    if (nesting.length == 0)
    {
        assert(!"Writer: Empty Compound - Compound Tag with no values.\n");
    }
    nesting_info_.pop();
    WriteTag(TAG::End);
}

bool Writer::BeginList(StringView name)
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
    nesting_info_.emplace(NestingInfo{ TAG::End, NestingInfo::ContainerType::List, 0, current_file_pos });
    return true;
}

void Writer::EndList()
{
    NestingInfo& nesting = nesting_info_.top();
    if (nesting.container_type != NestingInfo::ContainerType::List)
    {
        assert(!"Writer: Mismatched List Begin/End - List Tag closed without being opened.\n");
    }
    if (nesting.length == 0)
    {
        nesting.data_type = TAG::End;
    }

    fpos_t current_file_pos;
    fgetpos(outfile_, &current_file_pos);

    fsetpos(outfile_, &(nesting.file_pos));
    WriteTag(nesting.data_type);
    WriteArrayLen(nesting.length);

    fsetpos(outfile_, &current_file_pos);

    nesting_info_.pop();
}

void Writer::WriteByte(int8_t b, StringView name)
{
    HandleNesting(name, TAG::Byte);
    if (!name.empty())
    {
        WriteTag(TAG::Byte);
        WriteStr(name);
    }
    fwrite(&b, sizeof(b), 1, outfile_);
}

void Writer::WriteShort(int16_t s, StringView name)
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

void Writer::WriteInt(int32_t i, StringView name)
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

void Writer::WriteLong(int64_t l, StringView name)
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

void Writer::WriteFloat(float f, StringView name)
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

void Writer::WriteDouble(double d, StringView name)
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

void Writer::WriteByteArray(int8_t const* array, int32_t length, StringView name)
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

void Writer::WriteString(StringView str, StringView name)
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

template<>
void Writer::Write(int8_t value, StringView name)
{
    WriteByte(value, name);
}
template<>
void Writer::Write(int16_t value, StringView name)
{
    WriteShort(value, name);
}
template<>
void Writer::Write(int32_t value, StringView name)
{
    WriteInt(value, name);
}
template<>
void Writer::Write(int64_t value, StringView name)
{
    WriteLong(value, name);
}
template<>
void Writer::Write(float value, StringView name)
{
    WriteFloat(value, name);
}
template<>
void Writer::Write(double value, StringView name)
{
    WriteDouble(value, name);
}
template<>
void Writer::Write(StringView value, StringView name)
{
    WriteString(value, name);
}

// private implementations

void Writer::WriteTag(TAG t)
{
    fwrite(&t, sizeof(t), 1, outfile_);
}

void Writer::WriteStr(StringView name)
{
    WriteStrLen(static_cast<int16_t>(name.size()));
    fwrite(name.data(), sizeof(StringView::value_type), name.size(), outfile_);
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
    nesting_info_.emplace(NestingInfo{ TAG::End, NestingInfo::ContainerType::Compound, 0, 0 });
    WriteTag(TAG::Compound);
    WriteStr("root");
}

void Writer::HandleNesting(StringView name, TAG t)
{
    NestingInfo& nesting = nesting_info_.top();
    if (nesting_info_.size() >= 512)
    {
        assert(!"Writer: Depth Error - Compound and List tags may not be nested beyond a depth of 512");
    }
    // Lists have strict requirements
    if (nesting.container_type == NestingInfo::ContainerType::List)
    {
        if (!name.empty())
        {
            assert(!"Writer: Name Error - Attempted to add a named tag to a List. Lists cannot contain named tags.\n");
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
                assert(!"Writer: Type Error - Attempted to add a tag to a list with tags of different type. All tags in a list must be of the same type.\n");
            }
        }
    }
    else if (nesting.container_type == NestingInfo::ContainerType::Compound)
    {
        if (name.empty())
        {
            assert(!"Writer: Structure Error - Attempted to add an unnamed tag to a compound. All tags in a compound must be named.\n");
        }
    }
    ++(nesting.length);
}

Builder::Builder()
{
    BeginCompound("root");
}

Builder::~Builder()
{
    EndCompound();
}

bool Builder::BeginCompound(StringView name)
{
    NamedDataTagIndex tagIdx = dataStore.AddNamedDataTag(TAG::Compound, name);
    if (!nestingStack.empty())
    {
        NestingInfo& nesting = nestingStack.top();
        NamedDataTag& tag = dataStore.namedTags[nesting.containerIdx];
        if (tag.type == TAG::Compound)
        {
            dataStore.compoundStorage[tag.compound_.poolIndex_].push_back(tagIdx);
        }
    }
    dataStore.compoundStorage.emplace_back();
    NamedDataTag& tag = dataStore.namedTags[tagIdx];
    tag.compound_.poolIndex_ = dataStore.compoundStorage.size() - 1;
    nestingStack.push(NestingInfo{tagIdx, 0});
    return true;
}

void Builder::EndCompound()
{
    NestingInfo& nesting = nestingStack.top();

    nestingStack.pop();
}

bool Builder::BeginList(StringView name)
{
    NamedDataTagIndex tagIdx = dataStore.AddNamedDataTag(TAG::List, name);
    if (!nestingStack.empty())
    {
        NestingInfo& nesting = nestingStack.top();
        NamedDataTag& tag = dataStore.namedTags[nesting.containerIdx];
        if (tag.type == TAG::Compound)
        {
            dataStore.compoundStorage[tag.compound_.poolIndex_].push_back(tagIdx);
        }
    }
    NamedDataTag& tag = dataStore.namedTags[tagIdx.idx];
    tag.list_.count_ = 0;
    tag.list_.elementType_ = TAG::End;
    tag.list_.poolIndex_ = -1;
    nestingStack.push(NestingInfo{tagIdx, tag.list_.count_ });

    return true;
}

void Builder::EndList()
{
    NestingInfo& nesting = nestingStack.top();
    NamedDataTag& tag = dataStore.namedTags[nesting.containerIdx];
    tag.list_.poolIndex_ = nesting.poolIndex;
    nestingStack.pop();
}

void Builder::WriteByte(int8_t b, StringView name)
{
    
}

void Builder::WriteShort(int16_t s, StringView name)
{
    
}

void Builder::WriteInt(int32_t i, StringView name)
{

}

void Builder::WriteLong(int64_t l, StringView name)
{

}

void Builder::WriteFloat(float f, StringView name)
{
    if (!name.empty())
    {
        NamedDataTagIndex tagIdx = dataStore.AddNamedDataTag(TAG::Float, name);
        if (!nestingStack.empty())
        {
            NestingInfo& nesting = nestingStack.top();
            NamedDataTag& tag = dataStore.namedTags[nesting.containerIdx];
            if (tag.type == TAG::Compound)
            {
                dataStore.compoundStorage[tag.compound_.poolIndex_].push_back(tagIdx);
            }
        }
        dataStore.namedTags[tagIdx.idx].float_ = f;
    }
    else
    {
        NestingInfo& nesting = nestingStack.top();
        if (nesting.count == 0)
        {
            nesting.poolIndex = dataStore.floatBuffer.size();
        }
        ++nesting.count;
        dataStore.namedTags[nesting.containerIdx].list_.elementType_ = TAG::Float;
        ++dataStore.namedTags[nesting.containerIdx].list_.count_;
        dataStore.floatBuffer.push_back(f);
    }
    ++(nestingStack.top().count);
}

void Builder::WriteDouble(double d, StringView name)
{

}

void Builder::WriteByteArray(int8_t const* array, int32_t count, StringView name)
{
    
}

void Builder::WriteIntArray(int32_t const* array, int32_t count, StringView name)
{
    
}

void Builder::WriteLongArray(int64_t const* array, int32_t count, StringView name)
{
    
}

void Builder::WriteString(StringView str, StringView name)
{
    
}

} // namespace ImNBT

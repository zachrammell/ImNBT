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

void Writer::WriteIntArray(int32_t const* array, int32_t count, StringView name)
{
    HandleNesting(name, TAG::Int_Array);
    if (!name.empty())
    {
        WriteTag(TAG::Int_Array);
        WriteStr(name);
    }
    WriteArrayLen(count);
    fwrite(&array, sizeof(array[0]), count, outfile_);
}

void Writer::WriteLongArray(int64_t const* array, int32_t count, StringView name)
{
    HandleNesting(name, TAG::Long_Array);
    if (!name.empty())
    {
        WriteTag(TAG::Long_Array);
        WriteStr(name);
    }
    WriteArrayLen(count);
    fwrite(&array, sizeof(array[0]), count, outfile_);
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
    NamedDataTagIndex rootTagIndex = dataStore.AddNamedDataTag<TagPayload::Compound>(TAG::Compound, "root");

    ContainerInfo rootContainer{};
    rootContainer.named = true;
    rootContainer.Type() = TAG::Compound;
    rootContainer.namedContainer.tagIndex = rootTagIndex;

    dataStore.namedTags[rootTagIndex].As<TagPayload::Compound>().storageIndex_ = dataStore.compoundStorage.size();
    dataStore.compoundStorage.emplace_back();

    containers.push(rootContainer);
}

Builder::~Builder()
{
    EndCompound();
}

bool Builder::BeginCompound(StringView name)
{
    return WriteTag(TAG::Compound, name, TagPayload::Compound{});
}

void Builder::EndCompound()
{
    ContainerInfo& container = containers.top();
    assert(container.Type() == TAG::Compound);
    containers.pop();
}

bool Builder::BeginList(StringView name)
{
    return WriteTag(TAG::List, name, TagPayload::List{});
}

void Builder::EndList()
{
    ContainerInfo& container = containers.top();
    assert(container.Type() == TAG::List);
    containers.pop();
}

void Builder::WriteByte(int8_t b, StringView name)
{
    WriteTag(TAG::Byte, name, byte{ b });
}

void Builder::WriteShort(int16_t s, StringView name)
{
    WriteTag(TAG::Short, name, s);
}

void Builder::WriteInt(int32_t i, StringView name)
{
    WriteTag(TAG::Int, name, i);
}

void Builder::WriteLong(int64_t l, StringView name)
{
    WriteTag(TAG::Long, name, l);
}

void Builder::WriteFloat(float f, StringView name)
{
    WriteTag(TAG::Float, name, f);
}

void Builder::WriteDouble(double d, StringView name)
{
    WriteTag(TAG::Double, name, d);
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
    WriteTag(TAG::String, name, TagPayload::String{});
}

TAG& Builder::ContainerInfo::Type()
{
    return type;
}

TAG Builder::ContainerInfo::Type() const
{
    return type;
}

TAG& Builder::ContainerInfo::ElementType(DataStore& ds)
{
    // only Lists have single element types
    assert(Type() == TAG::List);
    if (named)
    {
        return ds.namedTags[namedContainer.tagIndex].As<TagPayload::List>().elementType_;
    }
    return ds.Pool<TagPayload::List>()[anonContainer.poolIndex].elementType_;
}

int32_t Builder::ContainerInfo::Count(DataStore& ds) const
{
    if (named)
    {
        switch (Type())
        {
            case TAG::List:
                return ds.namedTags[namedContainer.tagIndex].As<TagPayload::List>().count_;
            case TAG::Compound:
                return ds.compoundStorage[ds.namedTags[namedContainer.tagIndex].As<TagPayload::Compound>().storageIndex_].size();
            default:
                assert(!"Writer : Internal Type Error - This should never happen.");
                return -1;
        }
    }
    switch (Type())
    {
        case TAG::List:
            return ds.Pool<TagPayload::List>()[anonContainer.poolIndex].count_;
        case TAG::Compound:
            return ds.compoundStorage[ds.Pool<TagPayload::Compound>()[anonContainer.poolIndex].storageIndex_].size();
        default:
            assert(!"Writer : Internal Type Error - This should never happen.");
            return -1;
    }
}

void Builder::ContainerInfo::IncrementCount(DataStore& ds)
{
    // only lists can have their count incremented
    assert(Type() == TAG::List);
    ++ds.namedTags[namedContainer.tagIndex].As<TagPayload::List>().count_;
}

uint64_t Builder::ContainerInfo::Storage(DataStore& ds)
{
    // only compounds can have storage
    assert(Type() == TAG::Compound);
    if (named)
    {
        return ds.namedTags[namedContainer.tagIndex].As<TagPayload::Compound>().storageIndex_;
    }
    return ds.Pool<TagPayload::Compound>()[anonContainer.poolIndex].storageIndex_;
}

bool Builder::HandleNesting(TAG t, StringView name)
{
    if (containers.size() >= 512)
    {
        assert(!"Writer: Depth Error - Compound and List tags may not be nested beyond a depth of 512");
        return false;
    }

    ContainerInfo& container = containers.top();

    // Lists have strict requirements
    if (container.Type() == TAG::List)
    {
        if (!name.empty())
        {
            assert(!"Writer: List Element Named - Attempted to add a named tag to a List. Lists cannot contain named tags.\n");
            return false;
        }
        TAG& elementType = container.ElementType(dataStore);
        // The list is not exclusively the same type as this tag
        if (elementType != t)
        {
            // If the list is currently empty, make it into a list of tags of this type
            if (container.Count(dataStore) == 0)
            {
                elementType = t;
            }
            else
            {
                assert(!"Writer: List Type Mismatch - Attempted to add a tag to a list with tags of different type. All tags in a list must be of the same type.\n");
                return false;
            }
        }
        container.IncrementCount(dataStore);
    }
    else if (container.Type() == TAG::Compound)
    {
        if (name.empty())
        {
            assert(!"Writer: Compound Structure Error - Attempted to add an unnamed tag to a compound. All tags in a compound must be named.\n");
            return false;
        }
    }
    return true;
}

} // namespace ImNBT

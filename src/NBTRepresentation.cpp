#include <ImNBT/NBTRepresentation.hpp>

namespace ImNBT
{

namespace Internal
{

bool IsContainer(TAG t) { return t == TAG::List || t == TAG::Compound; }

} // namespace Internal

StringView NamedDataTag::GetName() const
{
  return { name.data(), name.size() };
}

void NamedDataTag::SetName(StringView inName)
{
  name.assign(inName.data(), inName.size());
}

Internal::NamedDataTagIndex DataStore::AddNamedDataTag(TAG type, StringView name)
{
  NamedDataTag tag;
  tag.dataTag.type = type;
  tag.SetName(name);

  namedTags.push_back(tag);
  return namedTags.size() - 1;
}

void DataStore::Clear()

{
  compoundStorage.clear();
  namedTags.clear();
  Internal::Pools<byte, int16_t, int32_t, int64_t, float, double, char,
                  TagPayload::ByteArray, TagPayload::IntArray,
                  TagPayload::LongArray, TagPayload::String,
                  TagPayload::List, TagPayload::Compound>::Clear();
}


} // namespace ImNBT

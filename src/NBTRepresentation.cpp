#include <ImNBT/NBTRepresentation.hpp>

namespace ImNBT
{

NamedDataTagIndex DataStore::AddNamedDataTag(TAG type, StringView name)
{
    NamedDataTag tag;
    tag.type = type;
    tag.name = name;
#if defined(DEBUG)
    tag.dataStore = this;
#endif
    namedTags.emplace_back(tag);

    return {namedTags.size() - 1};
}

}
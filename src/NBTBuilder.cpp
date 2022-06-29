#include <ImNBT/NBTBuilder.hpp>

#include <cassert>

namespace ImNBT
{

using namespace ImNBT::Internal;

bool Builder::BeginCompound(StringView name)
{
  return WriteTag(TAG::Compound, name, TagPayload::Compound{});
}

void Builder::EndCompound()
{
  ContainerInfo& container = containers.top();
  assert(container.Type() == TAG::Compound);
  if (container.temporaryContainer)
  {
    auto& pool = dataStore.Pool<TagPayload::Compound>();
    int64_t const currentSize = static_cast<int64_t>(pool.size());
    auto& tempPool = container.temporaryContainer->data.Pool<TagPayload::Compound>();
    pool.insert(pool.end(), tempPool.begin(), tempPool.end());
    container.PoolIndex(dataStore) = currentSize;
    assert(container.temporaryContainer == &temporaryContainers.top());
    temporaryContainers.pop();
  }
  TagPayload::Compound compound{ container.Storage(dataStore)};
  containers.pop();
  if (containers.empty())
    return;
  ContainerInfo& parentContainer = containers.top();
  if (parentContainer.temporaryContainer)
  {
    parentContainer.temporaryContainer->data.Pool<TagPayload::Compound>()[parentContainer.currentIndex++] = compound;
  }
}

bool Builder::BeginList(StringView name)
{
  return WriteTag(TAG::List, name, TagPayload::List{});
}

void Builder::EndList()
{
  ContainerInfo& container = containers.top();
  assert(container.Type() == TAG::List);
  if (container.temporaryContainer)
  {
    // TODO: update elements in temporary container data pool
    int64_t currentSize = -1;
    if (container.ElementType(dataStore) == TAG::List)
    {
      auto& pool = dataStore.Pool<TagPayload::List>();
      currentSize = static_cast<int64_t>(pool.size());
      auto& tempPool = container.temporaryContainer->data.Pool<TagPayload::List>();
      pool.insert(pool.end(), tempPool.begin(), tempPool.end());

    }
    else if (container.ElementType(dataStore) == TAG::Compound)
    {
      auto& pool = dataStore.Pool<TagPayload::Compound>();
      currentSize = static_cast<int64_t>(pool.size());
      auto& tempPool = container.temporaryContainer->data.Pool<TagPayload::Compound>();
      pool.insert(pool.end(), tempPool.begin(), tempPool.end());
    }
    container.PoolIndex(dataStore) = currentSize;
    assert(container.temporaryContainer == &temporaryContainers.top());
    temporaryContainers.pop();
  }
  TagPayload::List list{ container.ElementType(dataStore), container.Count(dataStore), container.PoolIndex(dataStore) };
  containers.pop();
  ContainerInfo& parentContainer = containers.top();
  if (parentContainer.temporaryContainer)
  {
    parentContainer.temporaryContainer->data.Pool<TagPayload::List>()[parentContainer.currentIndex++] = list;
  }
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
  WriteTag<TagPayload::ByteArray>(TAG::Byte_Array, name, [this, array, count]() {
    auto& bytePool = dataStore.Pool<byte>();
    TagPayload::ByteArray byteArrayTag{ count, bytePool.size() };
    bytePool.insert(bytePool.end(), array, array + count);
    return byteArrayTag;
  });
}

void Builder::WriteIntArray(int32_t const* array, int32_t count, StringView name)
{
  WriteTag<TagPayload::IntArray>(TAG::Int_Array, name, [this, array, count]() {
    auto& intPool = dataStore.Pool<int32_t>();
    TagPayload::IntArray intArrayTag{ count, intPool.size() };
    intPool.insert(intPool.end(), array, array + count);
    return intArrayTag;
  });
}

void Builder::WriteLongArray(int64_t const* array, int32_t count, StringView name)
{
  WriteTag<TagPayload::LongArray>(TAG::Long_Array, name, [this, array, count]() {
    auto& longPool = dataStore.Pool<int64_t>();
    TagPayload::LongArray longArrayTag{ count, longPool.size() };
    longPool.insert(longPool.end(), array, array + count);
    return longArrayTag;
  });
}

void Builder::WriteString(StringView str, StringView name)
{
  WriteTag<TagPayload::String>(TAG::String, name, [this, str]() {
    auto& stringPool = dataStore.Pool<char>();
    TagPayload::String stringTag{ static_cast<uint16_t>(str.size()), stringPool.size() };
    stringPool.insert(stringPool.end(), str.data(), str.data() + str.size());
    return stringTag;
  });
}

void Builder::Begin(StringView rootName)
{
  NamedDataTagIndex rootTagIndex = dataStore.AddNamedDataTag(TAG::Compound, rootName);

  ContainerInfo rootContainer{};
  rootContainer.named = true;
  rootContainer.Type() = TAG::Compound;
  rootContainer.namedContainer.tagIndex = rootTagIndex;

  TagPayload::Compound c;
  c.storageIndex_ = dataStore.compoundStorage.size();
  dataStore.namedTags[rootTagIndex].dataTag.payload.Set<TagPayload::Compound>(c);
  dataStore.compoundStorage.emplace_back();

  containers.push(rootContainer);
}

void Builder::Finalize()
{
  while (!Finalized())
  {
    switch (containers.top().Type())
    {
      case TAG::List:
        EndList();
        break;
      case TAG::Compound:
        EndCompound();
        break;
      default:
        assert(!"Builder : Internal Type Error - This should never happen.");
    }
  }
}

bool Builder::Finalized() const { return containers.empty(); }

TAG& Builder::ContainerInfo::Type() { return type; }

TAG Builder::ContainerInfo::Type() const { return type; }

TAG& Builder::ContainerInfo::ElementType(DataStore& ds)
{
  // only Lists have single element types
  assert(Type() == TAG::List);
  if (named)
  {
    return ds.namedTags[namedContainer.tagIndex].dataTag.payload.As<TagPayload::List>().elementType_;
  }
  return anonContainer.list.elementType_;
}

int32_t Builder::ContainerInfo::Count(DataStore const& ds) const
{
  if (named)
  {
    switch (Type())
    {
      case TAG::List:
        return ds.namedTags[namedContainer.tagIndex].dataTag.payload.As<TagPayload::List>().count_;
      case TAG::Compound:
        return static_cast<int32_t>(ds.compoundStorage[ds.namedTags[namedContainer.tagIndex].dataTag.payload.As<TagPayload::Compound>().storageIndex_].size());
      default:
        assert(!"Builder : Internal Type Error - This should never happen.");
        return -1;
    }
  }
  switch (Type())
  {
    case TAG::List:
      return anonContainer.list.count_;
    case TAG::Compound:
      return static_cast<int32_t>(ds.compoundStorage[anonContainer.compound.storageIndex_].size());
    default:
      assert(!"Builder : Internal Type Error - This should never happen.");
      return -1;
  }
}

void Builder::ContainerInfo::IncrementCount(DataStore& ds)
{
  // only lists can have their count incremented
  assert(Type() == TAG::List);
  if (named)
  {
    ++ds.namedTags[namedContainer.tagIndex].dataTag.payload.As<TagPayload::List>().count_;
  }
  else
  {
    ++anonContainer.list.count_;
  }
}

uint64_t Builder::ContainerInfo::Storage(DataStore const& ds) const
{
  // only compounds can have storage
  assert(Type() == TAG::Compound);
  if (named)
  {
    return ds.namedTags[namedContainer.tagIndex].dataTag.payload.As<TagPayload::Compound>().storageIndex_;
  }
  return anonContainer.compound.storageIndex_;
}

size_t& Builder::ContainerInfo::PoolIndex(DataStore& ds)
{
  // only compounds can have pool indices
  assert(Type() == TAG::List);
  if (named)
  {
    return ds.namedTags[namedContainer.tagIndex].dataTag.payload.As<TagPayload::List>().poolIndex_;
  }
  return anonContainer.list.poolIndex_;
}

template<typename T, typename Fn>
bool Builder::WriteTag(TAG type, StringView name, Fn valueGetter)
{
  if (containers.size() >= 512)
  {
    assert(!"Builder : Depth Error - Compound and List tags may not be nested beyond a depth of 512");
    return false;
  }
  if (containers.empty())
  {
    assert(!"Builder : Write After Finalized - Attempted to write tags after structure was finalized");
    return false;
  }
  ContainerInfo& container = containers.top();
  if (container.Type() == TAG::Compound)
  {
    NamedDataTagIndex newTagIndex = dataStore.AddNamedDataTag(type, name);
    dataStore.namedTags[newTagIndex].dataTag.payload.Set<T>(valueGetter());
    dataStore.compoundStorage[container.Storage(dataStore)].push_back(newTagIndex);
    if (IsContainer(type))
    {
      ContainerInfo newContainer{};
      newContainer.named = true;
      newContainer.Type() = type;
      newContainer.namedContainer.tagIndex = newTagIndex;
      if (type == TAG::Compound)
      {
        dataStore.namedTags[newTagIndex].dataTag.payload.As<TagPayload::Compound>().storageIndex_ = dataStore.compoundStorage.size();
        dataStore.compoundStorage.emplace_back();
      }
      containers.push(newContainer);
    }
  }
  else if (container.Type() == TAG::List)
  {
    if (!name.empty())
    {
      assert(!"Builder : Name Error - Attempted to add a named tag to a List. Lists cannot contain named tags.\n");
      return false;
    }
    // The list is not exclusively the same type as this tag
    if (container.ElementType(dataStore) != type)
    {
      // If the list is currently empty, make it into a list of tags of this
      // type
      if (container.Count(dataStore) == 0)
      {
        container.ElementType(dataStore) = type;
      }
      else
      {
        assert(!"Builder : Type Error - Attempted to add a tag to a list with tags of different type. All tags in a list must be of the same type.\n");
        return false;
      }
    }
    if (IsContainer(type))
    {
      if (container.Count(dataStore) == 0)
      {
        TemporaryContainer temporaryContainer{};
        temporaryContainer.type = type;
        temporaryContainers.push(temporaryContainer);
        container.temporaryContainer = &temporaryContainers.top();
      }
      if constexpr (std::is_same_v<T, TagPayload::List> || std::is_same_v<T, TagPayload::Compound>)
      {
        container.temporaryContainer->data.Pool<T>().push_back(valueGetter());
      }

      ContainerInfo newContainer{};
      newContainer.named = false;
      newContainer.Type() = type;
      if (type == TAG::Compound)
      {
        newContainer.anonContainer.compound.storageIndex_ = dataStore.compoundStorage.size();
        dataStore.compoundStorage.emplace_back();
      }
      
      containers.push(newContainer);
    }
    else // ordinary data type
    {
      uint64_t poolIndex = dataStore.Pool<T>().size();
      dataStore.Pool<T>().push_back(valueGetter());
      if (container.Count(dataStore) == 0)
      {
        container.PoolIndex(dataStore) = poolIndex;
      }
    }
    container.IncrementCount(dataStore);
  }
  return true;
}

template<typename T, std::enable_if_t<!std::is_invocable_v<T>, bool>>
bool Builder::WriteTag(TAG type, StringView name, T value)
{
  return WriteTag<T>(type, name, [&value]() -> T { return value; });
}

} // namespace ImNBT

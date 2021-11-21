#pragma once

#include "NBTRepresentation.hpp"

#include <stack>
#include <type_traits>

namespace ImNBT
{

class Builder
{
public:
  /*!
   * \brief begins a Compound of other tags.
   * This means that all writes until EndCompound() is called will be written
   * into this compound. Compounds are analogous to structs and contain named
   * tags of any type. Tags inside a compound have the following requirements:
   * they must be named.
   *
   * Usage:
   *
   *  if (BeginCompound("my_compound") {
   *    WriteInt(3, "lives");
   *    WriteFloat(45.6666667f, "rotation");
   *    WriteString("Luigi", "name");
   *    EndCompound();
   *  }
   *
   *  This will create a compound of the following structure (JSON notation):
   *
   *  "my_compound": {
   *    "lives": 3,
   *    "rotation": 45.6666667f,
   *    "name": "Luigi"
   *  }
   *
   * \param name the name to give this compound tag
   * \return true if compound is successfully opened, false otherwise
   */
  bool BeginCompound(StringView name = "");
  /*!
   * \brief Ends writing to a previously started compound. Should only be called
   * if BeginCompound() returned true. After calling, writes will no longer be
   * added to the compound.
   */
  void EndCompound();

  /*!
   * \brief begins a List of other tags.
   *  This means that all writes until EndList() is called will be written into
   * this list. Lists are analogous to arrays, and are intended for grouping
   * data that has an obvious structure or otherwise does not need a name. Tags
   * inside a list have the following requirements: they must all have the same
   * type, and they cannot have names.
   *
   *  Usage:
   *
   *    if (BeginList("my_list") {
   *      WriteInt(1);
   *      WriteInt(2);
   *      WriteInt(3);
   *      EndList();
   *    }
   *
   *  This will create a List with 3 ints: {1, 2, 3}.
   *  Order is relevant: the order that tags are written to the list will be the
   * order they are read out/accessed.
   *
   * \param name the name to give this list object
   * \return true if list is opened for writing, false otherwise
   */
  bool BeginList(StringView name = "");
  /*!
   * \brief Ends writing to a previously started list. Should only be called if
   * BeginList() returned true. After calling, writes will no longer be added to
   * the list.
   */
  void EndList();

  void WriteByte(int8_t b, StringView name = "");
  void WriteShort(int16_t s, StringView name = "");
  void WriteInt(int32_t i, StringView name = "");
  void WriteLong(int64_t l, StringView name = "");
  void WriteFloat(float f, StringView name = "");
  void WriteDouble(double d, StringView name = "");
  void WriteByteArray(int8_t const* array, int32_t length, StringView name = "");
  void WriteIntArray(int32_t const* array, int32_t count, StringView name = "");
  void WriteLongArray(int64_t const* array, int32_t count, StringView name = "");
  void WriteString(StringView str, StringView name = "");

  void Begin(StringView rootName = "");
  void Finalize();
  bool Finalized() const;

protected:
  DataStore dataStore;

  struct ContainerInfo
  {
    bool named;
    TAG type;
    struct NamedContainer
    {
      Internal::NamedDataTagIndex tagIndex;
    };
    struct AnonContainer
    {
      uint64_t poolIndex;
    };
    union
    {
      NamedContainer namedContainer;
      AnonContainer anonContainer;
    };

    TAG& Type();
    TAG Type() const;
    TAG& ElementType(DataStore& ds);
    int32_t Count(DataStore& ds) const;
    void IncrementCount(DataStore& ds);
    uint64_t Storage(DataStore& ds);
    size_t& PoolIndex(DataStore& ds);
  };

  std::stack<ContainerInfo> containers;

  template<typename T, typename Fn>
  bool WriteTag(TAG type, StringView name, Fn valueGetter);

  template<typename T, std::enable_if_t<!std::is_invocable_v<T>, bool> = true>
  bool WriteTag(TAG type, StringView name, T value);
};

} // namespace ImNBT

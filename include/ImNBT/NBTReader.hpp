#pragma once

#include "NBTBuilder.hpp"
#include "NBTRepresentation.hpp"

#include <cassert>
#include <optional>
#include <string>
#include <vector>

namespace ImNBT
{

template<typename T>
using Optional = std::optional<T>;

class Reader : Builder
{
public:
  /*!
   * \brief opens and begins reading data from an NBT file
   * \param filepath path to the NBT file to open and read from
   */
  bool ImportFile(StringView filepath);

  bool ImportTextFile(StringView filepath);
  bool ImportBinaryFile(StringView filepath);
  bool ImportBinaryFileUncompressed(StringView filepath);

  /*!
   * \brief Opens a compound for reading. This means that all reads until CloseCompound() is called will be read from this compound.
   * Compounds are analogous to dictionaries/structs and contain named tags of any type.
   * Tags inside a compound have the following requirements: they must be named.
   *
   * Usage:
   *
   *  if (OpenCompound("my_compound")
   *  {
   *    int lives = ReadInt("lives");
   *    float rotation = ReadFloat("rotation");
   *    StringView name = ReadString("name");
   *    CloseCompound();
   *  }
   *
   *  This would be the appropriate way to read the data documented in BeginCompound()
   *
   * \param name name of compound to try and open
   * \return true if compound exists and can be read from, false otherwise
   */
  bool OpenCompound(StringView name = "");
  /*!
   * \brief Closes the last compound that was opened for reading. Should only be called if OpenCompound() returned true.
   * After calling, reads will no longer be taken from the compound.
   */
  void CloseCompound();

  /*!
   * \brief Opens a list for reading.
   *  This means that all reads until CloseList() is called will be read from this list.
   *  Lists are analogous to arrays, and are intended for grouping data that has an obvious structure or otherwise does not need a name.
   *  Tags inside a list have the following requirements: they must all have the same type, and they cannot have names.
   *
   *  Usage:
   *
   *  if (OpenList("my_list"))
   *  {
   *    vector<int> arr {ListSize()};
   *    for (int i = 0; i < ListSize(); ++i)
   *    {
   *      arr[i] = ReadInt();
   *    }
   *    CloseList();
   *  }
   *
   * \param name name of list to try and open
   * \return true if list exists and can be read from, false otherwise
   */
  bool OpenList(StringView name = "");
  /*!
   * \brief the number of elements in the currently open list.
   * Usage:
   *
   *  if (OpenList("my_list"))
   *  {
   *    vector<int> arr {ListSize()};
   *    for (int i = 0; i < ListSize(); ++i)
   *    {
   *      arr[i] = ReadInt();
   *    }
   *    CloseList();
   *  }
   *
   * \return the number of elements in the currently open list.
   */
  int32_t ListSize();
  /*!
   * \brief Closes the last list that was opened for reading. Should only be called if OpenList() returned true.
   * After calling, reads will no longer be taken from the list.
   */
  void CloseList();

  int8_t ReadByte(StringView name = "");
  int16_t ReadShort(StringView name = "");
  int32_t ReadInt(StringView name = "");
  int64_t ReadLong(StringView name = "");
  float ReadFloat(StringView name = "");
  double ReadDouble(StringView name = "");
  std::vector<int8_t> ReadByteArray(StringView name = "");
  std::vector<int32_t> ReadIntArray(StringView name = "");
  std::vector<int64_t> ReadLongArray(StringView name = "");
  StringView ReadString(StringView name = "");

  Optional<int8_t> MaybeReadByte(StringView name = "");
  Optional<int16_t> MaybeReadShort(StringView name = "");
  Optional<int32_t> MaybeReadInt(StringView name = "");
  Optional<int64_t> MaybeReadLong(StringView name = "");
  Optional<float> MaybeReadFloat(StringView name = "");
  Optional<double> MaybeReadDouble(StringView name = "");
  Optional<std::vector<int8_t>> MaybeReadByteArray(StringView name = "");
  Optional<StringView> MaybeReadString(StringView name = "");

  /*!
   * \brief Specialize MaybeRead on your own type to enable deserialization
   *  It's a generic reader function. for the basic NBT types, it acts exactly like calling the explicit function.
   *  For other types, the behavior is user-defined.
   *  This function is for reading data that is guaranteed to exist.
   *  Do not call it on something that isn't always serialized, use MaybeRead() for that.
   * \tparam T type of data to read (explicitly specialize when calling)
   * \param name name of the data that is guaranteed to be found
   * \return the value of the read data
   */
  template<typename T>
  T Read(StringView name = "");

  /*!
   * \brief This function is not implemented, only specialized! Specialize it on your own type to enable deserialization
   *  It's a generic reader function. for the basic NBT types, it acts exactly like calling the explicit function.
   *  For other types, the behavior is user-defined.
   *  This function is for reading data that is not guaranteed to exist.
   *  If the data cannot be read, it will return a nullopt and you can use something like value_or()
   * \tparam T type of data to read (explicitly specialize when calling)
   * \param name name of the data to try and find
   * \return the value of the read data
   */
  template<typename T>
  Optional<T> MaybeRead(StringView name = "");

private:
  class MemoryStream
  {
    size_t position = 0;
    std::vector<uint8_t> data;

  public:
    void SetContents(std::vector<uint8_t>&& inData)
    {
      Clear();
      data = std::move(inData);
    }

    template<typename T>
    T Retrieve()
    {
      assert(position + sizeof(T) <= data.size());
      T const* valueAddress = reinterpret_cast<T*>(data.data() + position);
      position += sizeof(T);
      return *valueAddress;
    }

    template<typename T>
    T const* RetrieveRangeView(size_t count)
    {
      assert(position + sizeof(T) * count <= data.size());
      T const* valueAddress = reinterpret_cast<T*>(data.data() + position);
      position += sizeof(T) * count;
      return valueAddress;
    }

    void ResetPosition()
    {
      position = 0;
    }

    void Clear()
    {
      ResetPosition();
      data.clear();
    }
  } memoryStream;

  void Clear();

  bool ImportCompressedFile(StringView filepath);
  bool ImportUncompressedFile(StringView filepath);

  bool ParseTextStream();
  bool ParseBinaryStream();

  TAG ParseBinaryNamedTag();
  bool ParseBinaryPayload(TAG type, StringView name = "");

  TAG RetrieveBinaryTag();
  std::string RetrieveBinaryStr();
  int32_t RetrieveBinaryArrayLen();

  bool HandleNesting(StringView name, TAG t);

  bool OpenContainer(TAG t, StringView name);

  template<typename T>
  T& ReadValue(TAG t, StringView name);
};

} // namespace ImNBT

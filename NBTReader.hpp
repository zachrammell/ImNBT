#pragma once

// Includes
#include <cstdio>
#include <optional>
#include <string_view>
#include <vector>
#include <string>
#include <stack>
#include <unordered_map>

namespace ImNBT
{

using std::string_view;

class Reader
{
public:
  /*!
   * \brief opens and begins reading data from an NBT file
   * \param filepath path to the NBT file to open and read from
   */
  Reader(string_view filepath);
  ~Reader();

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
   *    string_view name = ReadString("name");
   *    CloseCompound();
   *  }
   *
   *  This would be the appropriate way to read the data documented in NBTWriter::BeginCompound()
   *
   * \param name name of compound to try and open
   * \return true if compound exists and can be read from, false otherwise
   */
  bool OpenCompound(string_view name);
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
   *      arr[i] = ReadInt("");
   *    }
   *    CloseList();
   *  }
   *
   * \param name name of list to try and open
   * \return true if list exists and can be read from, false otherwise
   */
  bool OpenList(string_view name);
  /*!
   * \brief the number of elements in the currently open list.
   * Usage:
   *
   *  if (OpenList("my_list"))
   *  {
   *    vector<int> arr {ListSize()};
   *    for (int i = 0; i < ListSize(); ++i)
   *    {
   *      arr[i] = ReadInt("");
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

  int8_t ReadByte(string_view name);
  int16_t ReadShort(string_view name);
  int32_t ReadInt(string_view name);
  int64_t ReadLong(string_view name);
  float ReadFloat(string_view name);
  double ReadDouble(string_view name);
  std::vector<int8_t> ReadByteArray(string_view name);
  string_view ReadString(string_view name);

  std::optional<int8_t> MaybeReadByte(string_view name);
  std::optional<int16_t> MaybeReadShort(string_view name);
  std::optional<int32_t> MaybeReadInt(string_view name);
  std::optional<int64_t> MaybeReadLong(string_view name);
  std::optional<float> MaybeReadFloat(string_view name);
  std::optional<double> MaybeReadDouble(string_view name);
  std::optional<std::vector<int8_t>> MaybeReadByteArray(string_view name);
  std::optional<string_view> MaybeReadString(string_view name);

  /*!
   * \brief This function is not implemented, only specialized! Specialize it on your own type to enable deserialization
   *  It's a generic reader function. for the basic NBT types, it acts exactly like calling the explicit function.
   *  For other types, the behavior is user-defined.
   *  This function is for reading data that is guaranteed to exist.
   *  Do not call it on something that isn't always serialized, use MaybeRead() for that.
   * \tparam T type of data to read (explicitly specialize when calling)
   * \param name name of the data that is guaranteed to be found
   * \return the value of the read data
   */
  template<typename T>
  T Read(string_view name);

  /*!
   * \brief This function is not implemented, only specialized! Specialize it on your own type to enable deserialization
   *  It's a generic reader function. for the basic NBT types, it acts exactly like calling the explicit function.
   *  For other types, the behavior is user-defined.
   *  This function is for reading data that is not guaranteed to exist.
   *  If the data cannot be read, it will return a nullopt and you can use something like eastl::optional::value_or()
   * \tparam T type of data to read (explicitly specialize when calling)
   * \param name name of the data to try and find
   * \return the value of the read data
   */
  template<typename T>
  std::optional<T> MaybeRead(string_view name);

private:
  enum class TAG : uint8_t
  {
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    Byte_Array = 7,
    String = 8,
    List = 9,
    Compound = 10,
    INVALID = 0xCC
  };

  struct DataTag
  {
    TAG type_;
    union
    {
      int8_t byte_;
      int16_t short_;
      int32_t int_;
      int64_t long_;
      float float_;
      double double_;
      struct
      {
        int32_t byte_array_length_;
        size_t byte_array_pool_index;
      };
      struct
      {
        int16_t string_length_;
        size_t string_pool_index;
      };
      struct
      {
        TAG list_type_;
        int32_t list_length_;
      };
    };
  };

  std::unordered_map<std::string, DataTag> named_tags_;
  std::string root_name;
  std::string current_name_ = "";
  std::vector<char> string_pool_;
  std::vector<uint8_t> byte_array_pool_;
  size_t parsing_nesting_depth_ = 0;

  struct NestingInfo
  {
    TAG data_type;
    enum class ContainerType
    {
      List,
      Compound
    } container_type;
    int32_t length;
    int32_t list_index;
  };

  std::stack<NestingInfo> nesting_info_;

  FILE* infile_;

  /* READING HELPER FUNCTIONS */

  bool HandleNesting(string_view name, TAG t);
  void EnterRoot();

  /* PARSING FUNCTIONS */

  void ParseDataTree();
  DataTag& ParseDataTag();
  DataTag& ParseDataTagUnnamed(TAG type);

  TAG ReadTag();
  std::string ReadName();

  void AddToCurrentName(string_view name);
  void AddIndexToCurrentName(int32_t index);
  void PopLatestName();

  int16_t ReadStrLen();
  int32_t ReadArrayLen();
};

} // namespace ImNBT

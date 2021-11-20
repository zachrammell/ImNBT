#pragma once

#include "NBTBuilder.hpp"
#include "NBTRepresentation.hpp"

#include <cassert>
#include <cstdio>
#include <stack>
#include <string_view>
#include <type_traits>

namespace ImNBT
{

class Writer : public Builder
{
public:
  enum class PrettyPrint
  {
    Disabled,
    Enabled,
  };

  /*!
   * \brief This function is not implemented, only specialized! Specialize it on your own type to enable serialization.
   * It's a generic writer function. for the basic NBT types, it acts exactly like calling the explicit function.
   * For other types, the behavior is user-defined.
   * \tparam T type of data to write (prefer to explicitly specialize when calling)
   * \param value value to write
   * \param name name of value to write
   */
  template<typename T>
  void Write(T value, StringView name = "");

  bool OutputTextFile(StringView filepath, PrettyPrint prettyPrint = PrettyPrint::Enabled);
  bool OutputBinaryFileUncompressed(StringView filepath);
  bool OutputBinaryFile(StringView filepath);

  bool OutputString(std::string& out, PrettyPrint prettyPrint = PrettyPrint::Disabled);
  bool OutputBinary(std::vector<uint8_t>& out);

private:
  void OutputBinaryTag(std::vector<uint8_t>& out, NamedDataTag const& tag);
  void OutputBinaryStr(std::vector<uint8_t>& out, StringView str);
  void OutputBinaryPayload(std::vector<uint8_t>& out, DataTag const& tag);

  void OutputTextTag(std::ostream& out, NamedDataTag const& tag);
  void OutputTextStr(std::ostream& out, StringView str);
  void OutputTextPayload(std::ostream& out, DataTag const& tag);

  struct TextOutputState
  {
    int depth = 0;
    PrettyPrint prettyPrint;
  } textOutputState {};
};

} // namespace ImNBT

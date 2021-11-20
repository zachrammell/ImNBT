#pragma once

#include "NBTRepresentation.hpp"
#include "NBTBuilder.hpp"

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

    bool OutputTextFile(StringView filepath);
    bool OutputBinaryFileUncompressed(StringView filepath);
    bool OutputBinaryFile(StringView filepath);

    bool OutputString(std::string& out);
    bool OutputBinary(std::vector<uint8_t>& out);

private:
    void OutputBinaryTag(std::vector<uint8_t>& out, NamedDataTag const& tag);
    void OutputBinaryStr(std::vector<uint8_t>& out, StringView str);
    void OutputBinaryPayload(std::vector<uint8_t>& out, DataTag const& tag);

    void WriteTag(TAG t);
    void WriteStr(StringView name);
    void WriteStrLen(int16_t len);
    void WriteArrayLen(int32_t len);
};

} // namespace ImNBT

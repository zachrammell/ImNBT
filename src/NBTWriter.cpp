#include <ImNBT/NBTWriter.hpp>

#include "byteswapping.h"

#include "zlib.h"

#include <iomanip>
#include <sstream>
#include <cstring>

namespace ImNBT
{

template<typename T, std::enable_if_t<(sizeof(T) <= sizeof(T*)), bool> = true>
void Store(std::vector<uint8_t>& v, T data)
{
  auto const size = v.size();
  v.resize(size + sizeof(T));
  std::memcpy(v.data() + size, &data, sizeof(T));
}

template<typename T, std::enable_if_t<(sizeof(T) > sizeof(T*)), bool> = true>
void Store(std::vector<uint8_t>& v, T& data)
{
  auto const size = v.size();
  v.resize(size + sizeof(T));
  std::memcpy(v.data() + size, &data, sizeof(T));
}

template<typename T>
void StoreRange(std::vector<uint8_t>& v, T* data, size_t count)
{
  auto const size = v.size();
  v.resize(size + sizeof(T) * count);
  std::memcpy(v.data() + size, data, sizeof(T) * count);
}

std::string EscapeQuotes(std::string_view inStr)
{
  std::string result;
  size_t const length = inStr.length();
  result.reserve(length + 10); // assume up to 10 quotes
  for (size_t i = 0; i < length; ++i)
  {
    if (inStr[i] == '"')
    {
      result += R"(\")";
    }
    else
    {
      result += inStr[i];
    }
  }
  return result;
}

static std::ostream& NewlineFn(std::ostream& out);
static std::ostream& SpacingFn(std::ostream& out, int depth);

class NewlineOp
{
  Writer::PrettyPrint prettyPrint;
public:
  NewlineOp(Writer::PrettyPrint prettyPrint) : prettyPrint(prettyPrint) {}
  friend std::ostream& operator<<(std::ostream& out, NewlineOp const& op)
  {
    if (op.prettyPrint == Writer::PrettyPrint::Enabled)
      return NewlineFn(out);
    return out;
  }
};

class SpacingOp
{
  Writer::PrettyPrint prettyPrint;
  int depth;
public:
  SpacingOp(Writer::PrettyPrint prettyPrint, int depth) : prettyPrint(prettyPrint), depth(depth) {}
  friend std::ostream& operator<<(std::ostream& out, SpacingOp const& op)
  {
    if (op.prettyPrint == Writer::PrettyPrint::Enabled)
      return SpacingFn(out, op.depth);
    return out;
  }
};

#define Newline NewlineOp(textOutputState.prettyPrint)
#define Spacing SpacingOp(textOutputState.prettyPrint, textOutputState.depth)

// private implementations

Writer::Writer()
{
  Begin();
}

Writer::~Writer()
{
  Finalize();
}

bool Writer::ExportTextFile(StringView filepath, PrettyPrint prettyPrint)
{
  if (!Finalized())
    return false;

  FILE* file = fopen(filepath.data(), "wb");
  if (!file)
  {
    return false;
  }
  std::string text;
  if (!ExportString(text, prettyPrint))
  {
    return false;
  }
  auto const written = fwrite(text.data(), sizeof(uint8_t), text.size(), file);
  fclose(file);
  return written == text.size();
}

bool Writer::ExportBinaryFileUncompressed(StringView filepath)
{
  if (!Finalized())
    return false;

  FILE* file = fopen(filepath.data(), "wb");
  if (!file)
  {
    return false;
  }
  std::vector<uint8_t> data;
  if (!ExportBinary(data))
  {
    fclose(file);
    return false;
  }
  auto const written = fwrite(data.data(), sizeof(uint8_t), data.size(), file);
  fclose(file);
  return written == data.size();
}

bool Writer::ExportBinaryFile(StringView filepath)
{
  if (!Finalized())
    return false;

  FILE* file = fopen(filepath.data(), "wb");
  if (!file)
  {
    return false;
  }
  std::vector<uint8_t> data;
  if (!ExportBinary(data))
  {
    fclose(file);
    return false;
  }

  z_stream zs{};
  zs.avail_in = static_cast<uint32_t>(data.size());
  zs.next_in = data.data();

  // "Add 16 to windowBits to write a simple gzip header and trailer around the compressed data instead of a zlib wrapper"
  deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
  auto const deflatedDataSizeBound = deflateBound(&zs, static_cast<unsigned long>(data.size()));
  std::vector<uint8_t> deflatedData;
  deflatedData.resize(deflatedDataSizeBound);
  zs.avail_out = static_cast<uint32_t>(deflatedDataSizeBound);
  zs.next_out = deflatedData.data();

  deflate(&zs, Z_FINISH);
  deflateEnd(&zs);

  deflatedData.resize(zs.total_out);

  auto const written = fwrite(deflatedData.data(), sizeof(uint8_t), deflatedData.size(), file);
  fclose(file);
  return written == deflatedData.size();
}

bool Writer::ExportString(std::string& out, PrettyPrint prettyPrint)
{
  if (!Finalized())
    return false;
  textOutputState = {};
  textOutputState.prettyPrint = prettyPrint;
  auto const& root = dataStore.namedTags[0];
  std::stringstream outStream(out);
  OutputTextTag(outStream, root);
  out = outStream.str();
  return true;
}

bool Writer::ExportBinary(std::vector<uint8_t>& out)
{
  if (!Finalized())
    return false;
  auto const& root = dataStore.namedTags[0];
  OutputBinaryTag(out, root);
  return true;
}

void Writer::OutputBinaryTag(std::vector<uint8_t>& out, NamedDataTag const& tag)
{
  Store(out, tag.dataTag.type);
  OutputBinaryStr(out, tag.GetName());
  OutputBinaryPayload(out, tag.dataTag);
}

void Writer::OutputBinaryStr(std::vector<uint8_t>& out, StringView str)
{
  uint16_t const lenBigEndian = swap_u16(static_cast<int16_t>(str.length()));
  Store(out, lenBigEndian);
  StoreRange(out, str.data(), str.length());
}

void Writer::OutputBinaryPayload(std::vector<uint8_t>& out, DataTag const& tag)
{
  switch (tag.type)
  {
    case TAG::Byte: {
      Store(out, tag.payload.As<byte>());
    }
    break;
    case TAG::Short: {
      Store(out, swap_i16(tag.payload.As<int16_t>()));
    }
    break;
    case TAG::Int: {
      Store(out, swap_i32(tag.payload.As<int32_t>()));
    }
    break;
    case TAG::Long: {
      Store(out, swap_i64(tag.payload.As<int64_t>()));
    }
    break;
    case TAG::Float: {
      Store(out, swap_f32(tag.payload.As<float>()));
    }
    break;
    case TAG::Double: {
      Store(out, swap_f64(tag.payload.As<double>()));
    }
    break;
    case TAG::Byte_Array: {
      auto& byteArray = tag.payload.As<TagPayload::ByteArray>();
      Store(out, swap_i32(byteArray.count_));
      StoreRange(out, dataStore.Pool<byte>().data() + byteArray.poolIndex_, byteArray.count_);
    }
    break;
    case TAG::Int_Array: {
      auto& intArray = tag.payload.As<TagPayload::IntArray>();
      auto intPool = dataStore.Pool<int32_t>().data() + intArray.poolIndex_;
      Store(out, swap_i32(intArray.count_));
      for (int i = 0; i < intArray.count_; ++i)
      {
        Store(out, swap_i32(intPool[i]));
      }
    }
    break;
    case TAG::Long_Array: {
      auto& longArray = tag.payload.As<TagPayload::LongArray>();
      auto longPool = dataStore.Pool<int64_t>().data() + longArray.poolIndex_;
      Store(out, swap_i32(longArray.count_));
      for (int i = 0; i < longArray.count_; ++i)
      {
        Store(out, swap_i64(longPool[i]));
      }
    }
    break;
    case TAG::String: {
      auto& string = tag.payload.As<TagPayload::String>();
      Store(out, swap_u16(string.length_));
      StoreRange(out, dataStore.Pool<char>().data() + string.poolIndex_, string.length_);
    }
    break;
    case TAG::List: {
      auto& list = tag.payload.As<TagPayload::List>();
      if (list.count_ == 0)
      {
        Store(out, TAG::End);
        Store(out, list.count_);
        return;
      }
      Store(out, list.elementType_);
      Store(out, swap_u32(list.count_));
      switch (list.elementType_)
      {
        case TAG::Byte: {
          StoreRange(out, dataStore.Pool<byte>().data() + list.poolIndex_, list.count_);
        }
        break;
        case TAG::Short: {
          auto const* shortPool = dataStore.Pool<int16_t>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(int16_t) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_i16(shortPool[i]));
          }
        }
        break;
        case TAG::Int: {
          auto const* intPool = dataStore.Pool<int32_t>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(int32_t) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_i32(intPool[i]));
          }
        }
        break;
        case TAG::Long: {
          auto const* longPool = dataStore.Pool<int64_t>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(int64_t) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_i64(longPool[i]));
          }
        }
        break;
        case TAG::Float: {
          auto const* floatPool = dataStore.Pool<float>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(float) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_f32(floatPool[i]));
          }
        }
        break;
        case TAG::Double: {
          auto const* doublePool = dataStore.Pool<double>().data() + list.poolIndex_;
          out.reserve(out.size() + sizeof(double) * list.count_);
          for (int i = 0; i < list.count_; ++i)
          {
            Store(out, swap_f64(doublePool[i]));
          }
        }
        break;
        case TAG::Byte_Array: {
          auto const* byteArrayPool = dataStore.Pool<TagPayload::ByteArray>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            auto const& byteArray = byteArrayPool[i];
            Store(out, swap_i32(byteArray.count_));
            StoreRange(out, dataStore.Pool<byte>().data() + byteArray.poolIndex_, byteArray.count_);
          }
        }
        break;
        case TAG::Int_Array: {
          auto const* intArrayPool = dataStore.Pool<TagPayload::IntArray>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            auto const& intArray = intArrayPool[i];
            auto const* intPool = dataStore.Pool<int32_t>().data() + intArray.poolIndex_;
            Store(out, swap_i32(intArray.count_));
            for (int j = 0; j < intArray.count_; ++j)
            {
              Store(out, swap_i32(intPool[j]));
            }
          }
        }
        break;
        case TAG::Long_Array: {
          auto const* longArrayPool = dataStore.Pool<TagPayload::LongArray>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            auto const& longArray = longArrayPool[i];
            auto const* longPool = dataStore.Pool<int64_t>().data() + longArray.poolIndex_;
            Store(out, swap_i32(longArray.count_));
            for (int j = 0; j < longArray.count_; ++j)
            {
              Store(out, swap_i64(longPool[j]));
            }
          }
        }
        break;
        case TAG::String: {
          auto const* stringPool = dataStore.Pool<TagPayload::String>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            auto& string = stringPool[i];
            Store(out, swap_u16(string.length_));
            StoreRange(out, dataStore.Pool<char>().data() + string.poolIndex_, string.length_);
          }
        }
        break;
        case TAG::List: {
          auto const* listPool = dataStore.Pool<TagPayload::List>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            DataTag sublistTag;
            sublistTag.type = TAG::List;
            sublistTag.payload.Set(listPool[i]);
            OutputBinaryPayload(out, sublistTag);
          }
        }
        break;
        case TAG::Compound: {
          auto const* compoundPool = dataStore.Pool<TagPayload::Compound>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_; ++i)
          {
            DataTag subcompoundTag;
            subcompoundTag.type = TAG::Compound;
            subcompoundTag.payload.Set(compoundPool[i]);
            OutputBinaryPayload(out, subcompoundTag);
          }
        }
        break;
      }
    }
    break;
    case TAG::Compound: {
      auto& compound = tag.payload.As<TagPayload::Compound>();
      for (auto namedTagIndex : dataStore.compoundStorage[compound.storageIndex_])
      {
        OutputBinaryTag(out, dataStore.namedTags[namedTagIndex]);
      }
      Store(out, TAG::End);
    }
    break;
  }
}

void Writer::OutputTextTag(std::ostream& out, NamedDataTag const& tag)
{
  // root tag likely nameless
  if (!tag.GetName().empty())
  {
    OutputTextStr(out, tag.GetName());
    out << ':';
  }
  OutputTextPayload(out, tag.dataTag);
}

void Writer::OutputTextStr(std::ostream& out, StringView str)
{
  out << '"' << EscapeQuotes(str) << '"';
}

void Writer::OutputTextPayload(std::ostream& out, DataTag const& tag)
{
  switch (tag.type)
  {
    case TAG::Byte: {
      out << std::to_string(tag.payload.As<byte>()) << 'b';
    }
    break;
    case TAG::Short: {
      out << tag.payload.As<int16_t>() << 's';
    }
    break;
    case TAG::Int: {
      out << tag.payload.As<int32_t>();
    }
    break;
    case TAG::Long: {
      out << tag.payload.As<int64_t>() << 'l';
    }
    break;
    case TAG::Float: {
      out << std::setprecision(7);
      out << tag.payload.As<float>() << 'f';
    }
    break;
    case TAG::Double: {
      out << std::setprecision(15);
      out << tag.payload.As<double>();
    }
    break;
    case TAG::Byte_Array: {
      out << "[B;";
      auto& byteArray = tag.payload.As<TagPayload::ByteArray>();
      auto bytePool = dataStore.Pool<byte>().data() + byteArray.poolIndex_;
      for (int i = 0; i < byteArray.count_ - 1; ++i)
      {
        out << std::to_string(bytePool[i]) << "b,";
      }
      if (byteArray.count_)
        out << std::to_string(bytePool[byteArray.count_ - 1]) << "b]";
    }
    break;
    case TAG::Int_Array: {
      out << "[I;";
      auto& intArray = tag.payload.As<TagPayload::IntArray>();
      auto intPool = dataStore.Pool<int32_t>().data() + intArray.poolIndex_;
      for (int i = 0; i < intArray.count_ - 1; ++i)
      {
        out << intPool[i] << ',';
      }
      if (intArray.count_)
        out << intPool[intArray.count_ - 1] << ']';
    }
    break;
    case TAG::Long_Array: {
      out << "[L;";
      auto& longArray = tag.payload.As<TagPayload::LongArray>();
      auto longPool = dataStore.Pool<int64_t>().data() + longArray.poolIndex_;
      for (int i = 0; i < longArray.count_ - 1; ++i)
      {
        out << longPool[i] << "l,";
      }
      if (longArray.count_)
        out << longPool[longArray.count_ - 1] << "L]";
    }
    break;
    case TAG::String: {
      auto& string = tag.payload.As<TagPayload::String>();
      OutputTextStr(out, { dataStore.Pool<char>().data() + string.poolIndex_, string.length_ });
    }
    break;
    case TAG::List: {
      out << '[';
      auto& list = tag.payload.As<TagPayload::List>();
      if (list.count_ == 0)
      {
        out << ']';
        return;
      }
      switch (list.elementType_)
      {
        case TAG::Byte: {
          auto const* bytePool = dataStore.Pool<byte>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_ - 1; ++i)
          {
            out << std::to_string(bytePool[i]) << "b,";
          }
          out << std::to_string(bytePool[list.count_ - 1]) << 'b';
        }
        break;
        case TAG::Short: {
          auto const* shortPool = dataStore.Pool<int16_t>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_ - 1; ++i)
          {
            out << shortPool[i] << "s,";
          }
          out << shortPool[list.count_ - 1] << 's';
        }
        break;
        case TAG::Int: {
          auto const* intPool = dataStore.Pool<int32_t>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_ - 1; ++i)
          {
            out << intPool[i] << ',';
          }
          out << intPool[list.count_ - 1];
        }
        break;
        case TAG::Long: {
          auto const* longPool = dataStore.Pool<int64_t>().data() + list.poolIndex_;
          for (int i = 0; i < list.count_ - 1; ++i)
          {
            out << longPool[i] << "l,";
          }
          out << longPool[list.count_ - 1] << 'l';
        }
        break;
        case TAG::Float: {
          auto const* floatPool = dataStore.Pool<float>().data() + list.poolIndex_;
          out << std::setprecision(7);
          for (int i = 0; i < list.count_ - 1; ++i)
          {
            out << floatPool[i] << "f,";
          }
          out << floatPool[list.count_ - 1] << 'f';
        }
        break;
        case TAG::Double: {
          auto const* doublePool = dataStore.Pool<double>().data() + list.poolIndex_;
          out << std::setprecision(15);
          for (int i = 0; i < list.count_ - 1; ++i)
          {
            out << doublePool[i] << ',';
          }
          out << doublePool[list.count_ - 1];
        }
        break;
        case TAG::Byte_Array: {
          auto const* byteArrayPool = dataStore.Pool<TagPayload::ByteArray>().data() + list.poolIndex_;
          ++textOutputState.depth;
          for (int i = 0; i < list.count_; ++i)
          {
            out << Spacing;
            DataTag newTag;
            newTag.type = list.elementType_;
            newTag.payload.Set(byteArrayPool[i]);
            OutputTextPayload(out, newTag);
            if (i != list.count_ - 1)
              out << ',';
          }
          --textOutputState.depth;
        }
        break;
        case TAG::Int_Array: {
          auto const* intArrayPool = dataStore.Pool<TagPayload::IntArray>().data() + list.poolIndex_;
          ++textOutputState.depth;
          for (int i = 0; i < list.count_; ++i)
          {
            out << Spacing;
            DataTag newTag;
            newTag.type = list.elementType_;
            newTag.payload.Set(intArrayPool[i]);
            OutputTextPayload(out, newTag);
            if (i != list.count_ - 1)
              out << ',';
          }
          --textOutputState.depth;
        }
        break;
        case TAG::Long_Array: {
          auto const* longArrayPool = dataStore.Pool<TagPayload::LongArray>().data() + list.poolIndex_;
          ++textOutputState.depth;
          for (int i = 0; i < list.count_; ++i)
          {
            out << Spacing;
            DataTag newTag;
            newTag.type = list.elementType_;
            newTag.payload.Set(longArrayPool[i]);
            OutputTextPayload(out, newTag);
            if (i != list.count_ - 1)
              out << ',';
          }
          --textOutputState.depth;
        }
        break;
        case TAG::List: {
          auto const* listPool = dataStore.Pool<TagPayload::List>().data() + list.poolIndex_;
          ++textOutputState.depth;
          for (int i = 0; i < list.count_; ++i)
          {
            out << Spacing;
            DataTag newTag;
            newTag.type = list.elementType_;
            newTag.payload.Set(listPool[i]);
            OutputTextPayload(out, newTag);
            if (i != list.count_ - 1)
              out << ',';
          }
          --textOutputState.depth;
        }
        break;
        case TAG::Compound: {
          auto const* compoundPool = dataStore.Pool<TagPayload::Compound>().data() + list.poolIndex_;
          ++textOutputState.depth;
          for (int i = 0; i < list.count_; ++i)
          {
            out << Newline << Spacing;
            DataTag newTag;
            newTag.type = list.elementType_;
            newTag.payload.Set(compoundPool[i]);
            OutputTextPayload(out, newTag);
            if (i != list.count_ - 1)
              out << ',';
          }
          --textOutputState.depth;
        }
        break;
      }
      out << ']';
    }
    break;
    case TAG::Compound: {
      auto& compound = tag.payload.As<TagPayload::Compound>();
      out << '{' << Newline;
      ++textOutputState.depth;
      for (auto namedTagIndex : dataStore.compoundStorage[compound.storageIndex_])
      {
        out << Spacing;
        OutputTextTag(out, dataStore.namedTags[namedTagIndex]);
        if (namedTagIndex != dataStore.compoundStorage[compound.storageIndex_].back())
          out << ',' << Newline;
        else
          out << Newline;
      }
      --textOutputState.depth;
      out << Spacing << '}';
    }
    break;
  }
}

std::ostream& NewlineFn(std::ostream& out)
{
  out << '\n';
  return out;
}

std::ostream& SpacingFn(std::ostream& out, int depth)
{
  static char const* spacing[] = {
    "",
    "  ",
    "    ",
    "      ",
    "        ",
    "          ",
    "            ",
    "              ",
    "                ",
    "                  ",
    "                    ",
    "                      ",
    "                        ",
    "                          ",
    "                            ",
    "                              ",
    "                                ",
    "                                  ",
    "                                    ",
    "                                      ",
    "                                        ",
    "                                          ",
    "                                            ",
    "                                              ",
    "                                                ",
    "                                                  ",
    "                                                    ",
    "                                                      ",
    "                                                        ",
    "                                                          ",
    "                                                            ",
    "                                                              ",
    "                                                                ",
    "                                                                  ",
    "                                                                    ",
    "                                                                      ",
    "                                                                        ",
    "                                                                          ",
    "                                                                            ",
    "                                                                              ",
    "                                                                                ",
    "                                                                                  ",
    "                                                                                    ",
    "                                                                                      ",
    "                                                                                        ",
    "                                                                                          ",
    "                                                                                            ",
    "                                                                                              ",
    "                                                                                                ",
    "                                                                                                  ",
    "                                                                                                    ",
    "                                                                                                      ",
    "                                                                                                        ",
    "                                                                                                          ",
    "                                                                                                            ",
    "                                                                                                              ",
    "                                                                                                                ",
    "                                                                                                                  ",
    "                                                                                                                    ",
    "                                                                                                                      ",
    "                                                                                                                        ",
    "                                                                                                                          ",
    "                                                                                                                            ",
    "                                                                                                                              ",
    "                                                                                                                                ",
    "                                                                                                                                  ",
    "                                                                                                                                    ",
    "                                                                                                                                      ",
    "                                                                                                                                        ",
    "                                                                                                                                          ",
    "                                                                                                                                            ",
    "                                                                                                                                              ",
    "                                                                                                                                                ",
    "                                                                                                                                                  ",
    "                                                                                                                                                    ",
    "                                                                                                                                                      ",
  };
  out << spacing[depth];
  return out;
}

#undef Newline
#undef Spacing

} // namespace ImNBT

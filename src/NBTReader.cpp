#include <ImNBT/NBTReader.hpp>

#include "byteswapping.h"

#include "zlib.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>

namespace ImNBT
{

bool Reader::ImportFile(StringView filepath)
{
  if (ImportBinaryFileUncompressed(filepath))
    return true;
  if (ImportBinaryFile(filepath))
    return true;
  if (ImportTextFile(filepath))
    return true;
  return false;
}

bool Reader::ImportTextFile(StringView filepath)
{
  if (!ImportUncompressedFile(filepath))
    return false;

  Clear();

  return ParseTextStream();
}

bool Reader::ImportBinaryFile(StringView filepath)
{
  if (!ImportCompressedFile(filepath))
    return false;

  Clear();

  return ParseBinaryStream();
}

bool Reader::ImportBinaryFileUncompressed(StringView filepath)
{
  if (!ImportUncompressedFile(filepath))
    return false;

  Clear();

  return ParseBinaryStream();
}

bool Reader::ImportString(char const* data, uint32_t length)
{
  std::vector<uint8_t> mem(data, data + length);
  memoryStream.SetContents(std::move(mem));
  return ParseTextStream();
}

bool Reader::ImportBinary(uint8_t const* data, uint32_t length)
{
  std::vector mem(data, data + length);
  memoryStream.SetContents(std::move(mem));
  return ParseBinaryStream();
}

bool Reader::OpenCompound(StringView name)
{
  if (!HandleNesting(name, TAG::Compound))
  {
    return false;
  }
  return OpenContainer(TAG::Compound, name);
}

void Reader::CloseCompound()
{
  ContainerInfo& container = containers.top();
  if (container.type == TAG::Compound)
  {
    containers.pop();
    return;
  }
  assert(!"Reader : Compound Close Mismatch - Attempted to close a compound when a compound was not open.\n");
}

bool Reader::OpenList(StringView name)
{
  if (!HandleNesting(name, TAG::List))
  {
    return false;
  }
  return OpenContainer(TAG::List, name);
}

int32_t Reader::ListSize()
{
  ContainerInfo& container = containers.top();
  if (container.type == TAG::List)
  {
    return container.Count(dataStore);
  }
  assert(!"Reader : Invalid List Size Read - Attempted to read a list's size when a list was not open.\n");
  return -1;
}

void Reader::CloseList()
{
  ContainerInfo& container = containers.top();
  if (container.type == TAG::List)
  {
    containers.pop();
    return;
  }
  assert(!"Reader : List Close Mismatch - Attempted to close a list when a list was not open.\n");
}

int8_t Reader::ReadByte(StringView name)
{
  HandleNesting(name, TAG::Byte);
  return ReadValue<byte>(TAG::Byte, name);
}

int16_t Reader::ReadShort(StringView name)
{
  HandleNesting(name, TAG::Short);
  return ReadValue<int16_t>(TAG::Short, name);
}

int32_t Reader::ReadInt(StringView name)
{
  HandleNesting(name, TAG::Int);
  return ReadValue<int32_t>(TAG::Int, name);
}

int64_t Reader::ReadLong(StringView name)
{
  HandleNesting(name, TAG::Long);
  return ReadValue<int64_t>(TAG::Long, name);
}

float Reader::ReadFloat(StringView name)
{
  HandleNesting(name, TAG::Float);
  return ReadValue<float>(TAG::Float, name);
}

double Reader::ReadDouble(StringView name)
{
  HandleNesting(name, TAG::Double);
  return ReadValue<double>(TAG::Double, name);
}

std::vector<int8_t> Reader::ReadByteArray(StringView name)
{
  HandleNesting(name, TAG::Byte_Array);
  TagPayload::ByteArray byteArray = ReadValue<TagPayload::ByteArray>(TAG::Byte_Array, name);
  auto* bytePool = dataStore.Pool<byte>().data() + byteArray.poolIndex_;
  std::vector<int8_t> ret{
    bytePool,
    bytePool + byteArray.count_
  };
  return ret;
}

std::vector<int32_t> Reader::ReadIntArray(StringView name)
{
  HandleNesting(name, TAG::Int_Array);
  TagPayload::IntArray intArray = ReadValue<TagPayload::IntArray>(TAG::Int_Array, name);
  auto* intPool = dataStore.Pool<int32_t>().data() + intArray.poolIndex_;
  std::vector<int32_t> ret{
    intPool,
    intPool + intArray.count_
  };
  std::transform(ret.begin(), ret.end(), ret.begin(), swap_i32);
  return ret;
}

std::vector<int64_t> Reader::ReadLongArray(StringView name)
{
  HandleNesting(name, TAG::Long_Array);
  TagPayload::LongArray longArray = ReadValue<TagPayload::LongArray>(TAG::Long_Array, name);
  auto* longPool = dataStore.Pool<int64_t>().data() + longArray.poolIndex_;
  std::vector<int64_t> ret{
    longPool,
    longPool + longArray.count_
  };
  std::transform(ret.begin(), ret.end(), ret.begin(), swap_i64);
  return ret;
}

StringView Reader::ReadString(StringView name)
{
  HandleNesting(name, TAG::String);
  TagPayload::String string = ReadValue<TagPayload::String>(TAG::String, name);
  return StringView{
    dataStore.Pool<char>().data() + string.poolIndex_,
    static_cast<size_t>(string.length_)
  };
}

Optional<int8_t> Reader::MaybeReadByte(StringView name)
{
  if (!HandleNesting(name, TAG::Byte))
    return std::nullopt;
  if (auto b = MaybeReadValue<byte>(TAG::Byte, name))
    return *b;
  return {};
}

Optional<int16_t> Reader::MaybeReadShort(StringView name)
{
  if (!HandleNesting(name, TAG::Short))
    return std::nullopt;
  return MaybeReadValue<int16_t>(TAG::Short, name);
}

Optional<int32_t> Reader::MaybeReadInt(StringView name)
{
  if (!HandleNesting(name, TAG::Int))
    return std::nullopt;
  return MaybeReadValue<int32_t>(TAG::Int, name);
}

Optional<int64_t> Reader::MaybeReadLong(StringView name)
{
  if (!HandleNesting(name, TAG::Long))
    return std::nullopt;
  return MaybeReadValue<int64_t>(TAG::Long, name);
}

Optional<float> Reader::MaybeReadFloat(StringView name)
{
  if (!HandleNesting(name, TAG::Float))
    return std::nullopt;
  return MaybeReadValue<float>(TAG::Float, name);
}

Optional<double> Reader::MaybeReadDouble(StringView name)
{
  if (!HandleNesting(name, TAG::Double))
    return std::nullopt;
  return MaybeReadValue<double>(TAG::Double, name);
}

Optional<std::vector<int8_t>> Reader::MaybeReadByteArray(StringView name)
{
  if (!HandleNesting(name, TAG::Byte_Array))
    return std::nullopt;
  Optional<TagPayload::ByteArray> byteArray = MaybeReadValue<TagPayload::ByteArray>(TAG::Byte_Array, name);
  if (!byteArray)
    return std::nullopt;
  auto* bytePool = dataStore.Pool<byte>().data() + byteArray->poolIndex_;
  std::vector<int8_t> ret{
    bytePool,
    bytePool + byteArray->count_
  };
  return std::make_optional(ret);
}

Optional<std::vector<int32_t>> Reader::MaybeReadIntArray(StringView name)
{
  if (!HandleNesting(name, TAG::Int_Array))
    return std::nullopt;
  Optional<TagPayload::IntArray> intArray = MaybeReadValue<TagPayload::IntArray>(TAG::Int_Array, name);
  if (!intArray)
    return std::nullopt;
  auto* intPool = dataStore.Pool<int32_t>().data() + intArray->poolIndex_;
  std::vector<int32_t> ret{
    intPool,
    intPool + intArray->count_
  };
  std::transform(ret.begin(), ret.end(), ret.begin(), swap_i32);
  return std::make_optional(ret);
}

Optional<std::vector<int64_t>> Reader::MaybeReadLongArray(StringView name)
{
  if (!HandleNesting(name, TAG::Long_Array))
    return std::nullopt;
  Optional<TagPayload::LongArray> longArray = MaybeReadValue<TagPayload::LongArray>(TAG::Long_Array, name);
  if (!longArray)
    return std::nullopt;
  auto* longPool = dataStore.Pool<int64_t>().data() + longArray->poolIndex_;
  std::vector<int64_t> ret{
    longPool,
    longPool + longArray->count_
  };
  std::transform(ret.begin(), ret.end(), ret.begin(), swap_i64);
  return std::make_optional(ret);
}

Optional<StringView> Reader::MaybeReadString(StringView name)
{
  if (!HandleNesting(name, TAG::String))
    return std::nullopt;
  Optional<TagPayload::String> string = MaybeReadValue<TagPayload::String>(TAG::String, name);
  if (!string)
    return std::nullopt;
  return StringView{
    dataStore.Pool<char>().data() + string->poolIndex_,
    static_cast<size_t>(string->length_)
  };
}

void Reader::MemoryStream::SetContents(std::vector<uint8_t>&& inData)
{
  Clear();
  data = std::move(inData);
}

void Reader::MemoryStream::Clear()
{
  position = 0;
  data.clear();
}

bool Reader::MemoryStream::HasContents() const
{
  return position < data.size();
}

char Reader::MemoryStream::CurrentByte() const
{
  assert(HasContents());
  return data[position];
}

char Reader::MemoryStream::LookaheadByte(int bytes) const
{
  assert(position + bytes < data.size());
  return data[position + bytes];
}

Reader::TextTokenizer::TextTokenizer(MemoryStream& stream)
  : stream(stream)
{
}

void Reader::TextTokenizer::Tokenize()
{
  while (stream.HasContents())
  {
    if (auto token = ParseToken())
    {
      tokens.emplace_back(*token);
    }
  }
}

Reader::Token const& Reader::TextTokenizer::Current() const
{
  return tokens[current];
}

bool Reader::TextTokenizer::Match(Token::Type type)
{
  if (tokens[current].type == type)
  {
    ++current;
    return true;
  }
  return false;
}

Optional<Reader::Token> Reader::TextTokenizer::ParseToken()
{
  stream.SkipBytes<' ', '\r', '\n', '\t'>();
  char const c = stream.CurrentByte();
  switch (c)
  {
    case '{':
      stream.Retrieve<char>();
      return Token{ Token::Type::COMPOUND_BEGIN };
    case '}':
      stream.Retrieve<char>();
      return Token{ Token::Type::COMPOUND_END };
    case '[':
      return TryParseListOpen();
    case ']':
      stream.Retrieve<char>();
      return Token{ Token::Type::LIST_END };
    case ':':
      stream.Retrieve<char>();
      return Token{ Token::Type::NAME_DELIM };
    case ',':
      stream.Retrieve<char>();
      return Token{ Token::Type::CONTAINER_DELIM };
    default:
      break;
  }
  if (auto const number = TryParseNumber())
  {
    return number;
  }
  if (auto const string = TryParseString())
  {
    return string;
  }
  return {};
}

Optional<Reader::Token> Reader::TextTokenizer::TryParseListOpen()
{
  if (stream.MatchCurrentByte<'['>())
  {
    Token t{ Token::Type::LIST_BEGIN };
    if (stream.LookaheadByte(1) == ';')
    {
      if (char type = stream.CurrentByte(); stream.MatchCurrentByte<'B', 'I', 'L'>())
      {
        t.typeIndicator = type;
      }
      // eat the `;`
      (void) stream.Retrieve<char>();
    }
    return t;
  }
  return {};
}

Optional<Reader::Token> Reader::TextTokenizer::TryParseNumber()
{
  Token::Type type = Token::Type::INTEGER;
  int offset = 0;
  int digitCount = 0;

  if (stream.LookaheadByte(offset) == '-' || stream.LookaheadByte(offset) == '+')
  {
    ++offset;
  }
  while (isdigit(stream.LookaheadByte(offset)))
  {
    ++offset;
    ++digitCount;
  }
  if (stream.LookaheadByte(offset) == '.')
  {
    ++offset;
    type = Token::Type::REAL;
    while (isdigit(stream.LookaheadByte(offset)))
    {
      ++offset;
      ++digitCount;
    }
  }
  if (digitCount)
  {
    Token t;
    t.type = type;
    t.text = StringView(stream.RetrieveRangeView<char>(offset), offset);
    if (char const suffix = stream.CurrentByte(); stream.MatchCurrentByte<'l', 'L', 'b', 'B', 's', 'S', 'f', 'F', 'd', 'D'>())
    {
      t.typeIndicator = suffix;
    }
    return t;
  }
  return {};
}

Optional<Reader::Token> Reader::TextTokenizer::TryParseString()
{
  int offset = 0;
  char quote = '\0';
  if (stream.LookaheadByte(offset) == '\'')
  {
    quote = '\'';
    ++offset;
  }
  else if (stream.LookaheadByte(offset) == '\"')
  {
    quote = '\"';
    ++offset;
  }

  if (quote)
  {
    while (!(stream.LookaheadByte(offset) == quote && stream.LookaheadByte(offset - 1) != '\\'))
    {
      ++offset;
    }
    if (offset >= 2)
    {
      (void) stream.Retrieve<char>();
      Token t{ Token::Type::STRING };
      t.text = StringView(stream.RetrieveRangeView<char>(offset), offset - 1);
      return t;
    }
  }
  else
  {
    while (!CheckByte<',', '[', ']', '{', '}', ' '>(stream.LookaheadByte(offset)))
    {
      ++offset;
    }
    if (offset >= 1)
    {
      Token t{ Token::Type::STRING };
      t.text = StringView(stream.RetrieveRangeView<char>(offset), offset);
      return t;
    }
  }
  return {};
}

void Reader::Clear()
{
  dataStore.Clear();
  decltype(containers)().swap(containers);
}

bool Reader::ImportCompressedFile(StringView filepath)
{
  gzFile infile = gzopen(filepath.data(), "rb");
  if (!infile) return false;

  std::vector<uint8_t> fileData;
  std::array<uint8_t, 8192> inputBuffer{};
  bool reading = true, ret = true;
  while (reading)
  {
    size_t const bytesRead = gzfread(inputBuffer.data(), sizeof(uint8_t), inputBuffer.size(), infile);
    if (bytesRead < inputBuffer.size())
    {
      if (gzeof(infile))
      {
        reading = false;
      }
      int err;
      gzerror(infile, &err);
      if (err)
      {
        ret = false;
        goto cleanup;
      }
    }
    fileData.insert(fileData.end(), inputBuffer.begin(), inputBuffer.begin() + bytesRead);
  }
  memoryStream.SetContents(std::move(fileData));
  this->filepath = filepath;

cleanup:
  gzclose(infile);
  return ret;
}

bool Reader::ImportUncompressedFile(StringView filepath)
{
  FILE* infile = fopen(filepath.data(), "rb");
  if (!infile) return false;

  bool ret = true;

  // get file size
  if (fseek(infile, 0, SEEK_END))
  {
    ret = false;
    goto cleanup;
  }
  {
    size_t const fileSize = ftell(infile);
    rewind(infile);

    std::vector<uint8_t> fileData(fileSize, {});
    size_t const bytesRead = fread(fileData.data(), sizeof(uint8_t), fileSize, infile);
    if (bytesRead != fileSize)
    {
      ret = false;
      goto cleanup;
    }

    memoryStream.SetContents(std::move(fileData));
  }
  this->filepath = filepath;

cleanup:
  fclose(infile);
  return ret;
}

bool Reader::ParseTextStream()
{
  TextTokenizer tokenizer(memoryStream);
  tokenizer.Tokenize();

  textTokenizer = &tokenizer;

  if (!textTokenizer->Match(Token::Type::COMPOUND_BEGIN))
    return false;

  Begin();

  do
  {
    if (ParseTextNamedTag() == TAG::End)
      return false;
  } while (textTokenizer->Match(Token::Type::CONTAINER_DELIM));

  if (!textTokenizer->Match(Token::Type::COMPOUND_END))
    return false;

  textTokenizer = nullptr;

  return true;
}

bool Reader::ParseBinaryStream()
{
  // parse root tag
  TAG const type = RetrieveBinaryTag();
  if (type != TAG::Compound)
    return false;
  Begin(RetrieveBinaryStr());

  do {
  } while (ParseBinaryNamedTag() != TAG::End);

  return true;
}

TAG Reader::ParseBinaryNamedTag()
{
  TAG const type = RetrieveBinaryTag();
  if (type == TAG::End)
    return type;
  auto const name = RetrieveBinaryStr();
  ParseBinaryPayload(type, name);
  return type;
}

bool Reader::ParseBinaryPayload(TAG type, StringView name)
{
  switch (type)
  {
    case TAG::Byte:
      WriteByte(memoryStream.Retrieve<byte>(), name);
      break;
    case TAG::Short:
      WriteShort(swap_i16(memoryStream.Retrieve<int16_t>()), name);
      break;
    case TAG::Int:
      WriteInt(swap_i32(memoryStream.Retrieve<int32_t>()), name);
      break;
    case TAG::Long:
      WriteLong(swap_i64(memoryStream.Retrieve<int64_t>()), name);
      break;
    case TAG::Float:
      WriteFloat(swap_f32(memoryStream.Retrieve<float>()), name);
      break;
    case TAG::Double:
      WriteDouble(swap_f64(memoryStream.Retrieve<double>()), name);
      break;
    case TAG::String:
      WriteString(RetrieveBinaryStr(), name);
      break;
    case TAG::Byte_Array: {
      auto const count = RetrieveBinaryArrayLen();
      WriteByteArray(memoryStream.RetrieveRangeView<int8_t>(count), count, name);
    }
    break;
    case TAG::Int_Array: {
      auto const count = RetrieveBinaryArrayLen();
      WriteIntArray(memoryStream.RetrieveRangeView<int32_t>(count), count, name);
    }
    break;
    case TAG::Long_Array: {
      auto const count = RetrieveBinaryArrayLen();
      WriteLongArray(memoryStream.RetrieveRangeView<int64_t>(count), count, name);
    }
    break;
    case TAG::List: {
      if (BeginList(name))
      {
        auto const elementType = RetrieveBinaryTag();
        auto const count = RetrieveBinaryArrayLen();
        for (int i = 0; i < count; ++i)
        {
          ParseBinaryPayload(elementType);
        }
        EndList();
      }
      else
        return false;
    }
    break;
    case TAG::Compound: {
      if (BeginCompound(name))
      {
        do {
        } while (ParseBinaryNamedTag() != TAG::End);
        EndCompound();
      }
      else
        return false;
    }
  }
  return true;
}

TAG Reader::RetrieveBinaryTag()
{
  return memoryStream.Retrieve<TAG>();
}

int32_t Reader::RetrieveBinaryArrayLen()
{
  return swap_i32(memoryStream.Retrieve<int32_t>());
}

template<typename T>
static auto ByteSwap(T x) -> T
{
  if constexpr(sizeof(T) == 2)
  {
    uint16_t const tmp = swap_u16(reinterpret_cast<uint16_t&>(x));
    return reinterpret_cast<T const&>(tmp);
  }
  if constexpr(sizeof(T) == 4)
  {
    uint32_t const tmp = swap_u32(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<T const&>(tmp);
  }
  if constexpr(sizeof(T) == 8)
  {
    uint64_t const tmp = swap_u64(reinterpret_cast<uint64_t&>(x));
    return reinterpret_cast<T const&>(tmp);
  }
  return x;
}

template<typename T>
static auto ParseNumber(StringView str) -> T
{
  T number;
  std::from_chars(str.data(), str.data() + str.size(), number);
  return number;
}

template<typename T, void(Builder::*WriteArray)(T const*, int32_t, StringView), char... Suffix>
static auto PackedIntegerList(Reader* reader, TAG tag, StringView name) -> TAG
{
  using Token = Reader::Token;
  Reader::TextTokenizer* textTokenizer = reader->textTokenizer;
  std::vector<T> integers;
  do
  {
    Token const& token = textTokenizer->Current();
    if (!textTokenizer->Match(Token::Type::INTEGER))
      return TAG::End;
    if constexpr(sizeof...(Suffix) > 0)
      if (!Reader::CheckByte<Suffix...>(token.typeIndicator))
        return TAG::End;
    integers.push_back(ParseNumber<T>(token.text));
  } while (textTokenizer->Match(Token::Type::CONTAINER_DELIM));

  if (!textTokenizer->Match(Token::Type::LIST_END))
    return TAG::End;
  std::transform(integers.begin(), integers.end(), integers.begin(), ByteSwap<T>);
  (reader->*WriteArray)(integers.data(), integers.size(), name);
  return tag;
}

TAG Reader::ParseTextNamedTag()
{
  Token const& name = textTokenizer->Current();
  if (!textTokenizer->Match(Token::Type::STRING))
    return TAG::End;
  if (!textTokenizer->Match(Token::Type::NAME_DELIM))
    return TAG::End;
  return ParseTextPayload(name.text);
}

TAG Reader::ParseTextPayload(StringView name)
{
  if (textTokenizer->Match(Token::Type::COMPOUND_BEGIN) && BeginCompound(name))
  {
    while (ParseTextNamedTag() != TAG::End)
    {
      if (!textTokenizer->Match(Token::Type::CONTAINER_DELIM))
        break;
    }

    if (textTokenizer->Match(Token::Type::COMPOUND_END))
    {
      EndCompound();
      return TAG::Compound;
    }

    return TAG::End;
  }
  if (Token const& listOpen = textTokenizer->Current(); textTokenizer->Match(Token::Type::LIST_BEGIN))
  {
    switch (listOpen.typeIndicator)
    {
      case 'B': return PackedIntegerList<int8_t,  &Builder::WriteByteArray, 'b', 'B'>(this, TAG::Byte_Array, name);
      case 'I': return PackedIntegerList<int32_t, &Builder::WriteIntArray>(this, TAG::Int_Array, name);
      case 'L': return PackedIntegerList<int64_t, &Builder::WriteLongArray, 'l', 'L'>(this, TAG::Long_Array, name);
      case '\0':
        break;
      default:
        return TAG::End;
    }
    if (BeginList(name))
    {
      // empty list
      if (textTokenizer->Match(Token::Type::LIST_END))
      {
        EndList();
        return TAG::List;
      }
      do
      {
        if (ParseTextPayload() == TAG::End)
          return TAG::End;
      } while (textTokenizer->Match(Token::Type::CONTAINER_DELIM));

      if (textTokenizer->Match(Token::Type::LIST_END))
      {
        EndList();
        return TAG::List;
      }
    }
    return TAG::End;
  }
  if (Token const& string = textTokenizer->Current(); textTokenizer->Match(Token::Type::STRING))
  {
    if (string.text == "true")
    {
      WriteByte(1, name);
      return TAG::Byte;
    }
    if (string.text == "false")
    {
      WriteByte(0, name);
      return TAG::Byte;
    }
    WriteString(string.text, name);
    return TAG::String;
  }
  if (Token const& integer = textTokenizer->Current(); textTokenizer->Match(Token::Type::INTEGER))
  {
    switch (integer.typeIndicator)
    {
      case 'b':
      case 'B': {
        WriteByte(ParseNumber<int8_t>(integer.text), name);
        return TAG::Byte;
      }
      case 's':
      case 'S': {
        WriteShort(ParseNumber<int16_t>(integer.text), name);
        return TAG::Short;
      }
      case 'l':
      case 'L':{
        WriteLong(ParseNumber<int64_t>(integer.text), name);
        return TAG::Long;
      }
      // ints do not use a suffix
      case '\0': {
        WriteInt(ParseNumber<int32_t>(integer.text), name);
        return TAG::Int;
      }
      default:
        return TAG::End;
    }
  }
  if (Token const& real = textTokenizer->Current(); textTokenizer->Match(Token::Type::REAL))
  {
    switch (real.typeIndicator)
    {
      case 'f':
      case 'F': {
        WriteFloat(ParseNumber<float>(real.text), name);
        return TAG::Float;
      }
      case 'd':
      case 'D':
      case '\0': {
        WriteDouble(ParseNumber<double>(real.text), name);
        return TAG::Float;
      }
      default:
        return TAG::End;
    }
  }
  
  return TAG::End;
}

std::string Reader::RetrieveBinaryStr()
{
  auto const len = swap_i16(memoryStream.Retrieve<int16_t>());
  std::string str(memoryStream.RetrieveRangeView<char>(len), len);
  return str;
}

bool Reader::HandleNesting(StringView name, TAG t)
{
  auto& container = containers.top();
  // Lists have strict requirements
  if (container.Type() == TAG::List)
  {
    if (!name.empty())
    {
      // problems, lists cannot have named tags
      assert(!"Reader : List Named Read - Attempted to read named tag from a list.");
      return false;
    }
    if (container.ElementType(dataStore) != t)
    {
      if (container.Count(dataStore) != 0)
      {
        assert(!"Reader : List Type Mismatch - Attempted to read the wrong type from a list.");
        return false;
      }
    }
    else
    {
      if (container.currentIndex >= container.Count(dataStore))
      {
        assert(!"Reader : List Overread - Attempted to read too many items from a list.");
        return false;
      }
    }
    ++container.currentIndex;
  }
  else if (container.Type() == TAG::Compound)
  {
    if (name.empty())
    {
      // bad, compound tags must have names
      assert(!"Reader : Compound Unnamed Read - Attempted to read unnamed tag from a compound.");
      return false;
    }
  }
  return true;
}

bool Reader::OpenContainer(TAG t, StringView name)
{
  auto& container = containers.top();
  if (container.Type() == TAG::List)
  {
    ContainerInfo newContainer{};
    newContainer.named = false;
    newContainer.type = t;
    if (t == TAG::List)
    {
      newContainer.anonContainer.list.poolIndex_ = dataStore.Pool<TagPayload::List>()[(container.currentIndex - 1) + container.PoolIndex(dataStore)].poolIndex_;
    }
    if (t == TAG::Compound)
    {
      newContainer.anonContainer.compound.storageIndex_ = dataStore.Pool<TagPayload::Compound>()[(container.currentIndex - 1) + container.PoolIndex(dataStore)].storageIndex_;
    }
    containers.push(newContainer);

    return true;
  }
  if (container.Type() == TAG::Compound)
  {
    for (auto tagIndex : dataStore.compoundStorage[container.Storage(dataStore)])
    {
      // TODO: if this ever becomes a performance issue, look at changing the vector to a set
      if (dataStore.namedTags[tagIndex].GetName() == name)
      {
        ContainerInfo newContainer{};
        newContainer.named = true;
        newContainer.type = t;
        newContainer.namedContainer.tagIndex = tagIndex;
        containers.push(newContainer);

        return true;
      }
    }
  }
  return false;
}

template<typename T>
T& Reader::ReadValue(TAG t, StringView name)
{
  ContainerInfo& container = containers.top();
  if (container.type == TAG::List)
  {
    return (dataStore.Pool<T>().data() + container.PoolIndex(dataStore))[container.currentIndex - 1];
  }
  if (container.type == TAG::Compound)
  {
    // TODO: if this ever becomes a performance issue, look at changing the vector to a set
    for (auto tagIndex : dataStore.compoundStorage[container.Storage(dataStore)])
    {
      auto& tag = dataStore.namedTags[tagIndex];
      if (tag.GetName() == name)
      {
        assert(tag.dataTag.type == t);
        return tag.dataTag.payload.As<T>();
      }
    }
  }

  assert(!"Reader Error: Value with given name not present");
  // undefined real bad behavior on purpose
  return *static_cast<T*>(nullptr);
}

template<typename T>
Optional<T> Reader::MaybeReadValue(TAG t, StringView name)
{
  ContainerInfo& container = containers.top();
  if (container.type == TAG::List)
  {
    return (dataStore.Pool<T>().data() + container.PoolIndex(dataStore))[container.currentIndex - 1];
  }
  if (container.type == TAG::Compound)
  {
    // TODO: if this ever becomes a performance issue, look at changing the vector to a set
    for (auto tagIndex : dataStore.compoundStorage[container.Storage(dataStore)])
    {
      auto& tag = dataStore.namedTags[tagIndex];
      if (tag.GetName() == name)
      {
        assert(tag.dataTag.type == t);
        return tag.dataTag.payload.As<T>();
      }
    }
  }
  return std::nullopt;
}

template<>
int8_t Reader::Read(StringView name)
{
  return ReadByte(name);
}
template<>
int16_t Reader::Read(StringView name)
{
  return ReadShort(name);
}
template<>
int32_t Reader::Read(StringView name)
{
  return ReadInt(name);
}
template<>
int64_t Reader::Read(StringView name)
{
  return ReadLong(name);
}
template<>
float Reader::Read(StringView name)
{
  return ReadFloat(name);
}
template<>
double Reader::Read(StringView name)
{
  return ReadDouble(name);
}
template<>
std::vector<int8_t> Reader::Read(StringView name)
{
  return ReadByteArray(name);
}
template<>
std::vector<int32_t> Reader::Read(StringView name)
{
  return ReadIntArray(name);
}
template<>
std::vector<int64_t> Reader::Read(StringView name)
{
  return ReadLongArray(name);
}
template<>
StringView Reader::Read(StringView name)
{
  return ReadString(name);
}

template<>
Optional<int8_t> Reader::MaybeRead(StringView name)
{
  return MaybeReadByte(name);
}
template<>
Optional<int16_t> Reader::MaybeRead(StringView name)
{
  return MaybeReadShort(name);
}
template<>
Optional<int32_t> Reader::MaybeRead(StringView name)
{
  return MaybeReadInt(name);
}
template<>
Optional<int64_t> Reader::MaybeRead(StringView name)
{
  return MaybeReadLong(name);
}
template<>
Optional<float> Reader::MaybeRead(StringView name)
{
  return MaybeReadFloat(name);
}
template<>
Optional<double> Reader::MaybeRead(StringView name)
{
  return MaybeReadDouble(name);
}
template<>
Optional<std::vector<int8_t>> Reader::MaybeRead(StringView name)
{
  return MaybeReadByteArray(name);
}
template<>
Optional<std::vector<int32_t>> Reader::MaybeRead(StringView name)
{
  return MaybeReadIntArray(name);
}
template<>
Optional<std::vector<int64_t>> Reader::MaybeRead(StringView name)
{
  return MaybeReadLongArray(name);
}
template<>
Optional<StringView> Reader::MaybeRead(StringView name)
{
  return MaybeReadString(name);
}

} // namespace ImNBT

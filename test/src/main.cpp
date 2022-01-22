#include <ImNBT/NBTReader.hpp>
#include <ImNBT/NBTWriter.hpp>

#include <ImNBT/NBTRepresentation.hpp>

#include <array>

void WriterTest()
{
  ImNBT::Writer writer;

  writer.WriteLong(9223372036854775807ll, "longTest");
  writer.WriteShort(32767, "shortTest");
  writer.WriteString(u8"HELLO WORLD THIS IS A TEST STRING ÅÄÖ!", "stringTest");
  writer.WriteFloat(0.4982315f, "floatTest");
  writer.WriteInt(2147483647, "intTest");
  if (writer.BeginCompound("nested compound test"))
  {
    if (writer.BeginCompound("ham"))
    {
      writer.WriteString("Hampus", "name");
      writer.WriteFloat(0.75f, "value");
      writer.EndCompound();
    }
    if (writer.BeginCompound("egg"))
    {
      writer.WriteString("Eggbert", "name");
      writer.WriteFloat(0.5f, "value");
      writer.EndCompound();
    }
    writer.EndCompound();
  }
  if (writer.BeginList("listTest (long)"))
  {
    for (int i = 0; i < 5; ++i)
    {
      writer.WriteLong(11ll + i);
    }
    writer.EndList();
  }
  if (writer.BeginList("listTest (compound)"))
  {
    for (int i = 0; i < 2; ++i)
    {
      if (writer.BeginCompound())
      {
        std::string name("Compound tag #");
        name += std::to_string(i);
        writer.WriteString(name, "name");
        writer.WriteLong(1264099775885ll, "created-on");
        writer.EndCompound();
      }
    }
    writer.EndList();
  }
  if (writer.BeginList("listTest (end)"))
  {
    writer.EndList();
  }
  writer.WriteByte(127, "byteTest");

  {
    std::array<int8_t, 1000> values{};
    for (int n = 0; n < values.size(); ++n)
    {
      values[n] = (n * n * 255 + n * 7) % 100;
    }
    writer.WriteByteArray(values.data(), values.size(), "byteArrayTest (the first 1000 values of (n*n*255+n*7)%100, starting with n=0 (0, 62, 34, 16, 8, ...))");
  }

  writer.WriteDouble(0.493128713218231, "doubleTest");

  std::array<int, 4> intArrayTest{ 66051, 67438087, 134810123, 202182159 };
  writer.WriteIntArray(intArrayTest.data(), intArrayTest.size(), "intArrayTest");

  std::array<int64_t, 2> longArrayTest{ 1003370060459195070, -2401053089480183795 };
  writer.WriteLongArray(longArrayTest.data(), longArrayTest.size(), "longArrayTest");

  writer.Finalize();

  writer.ExportBinaryFileUncompressed("./test/output/bigtest_uncompr");
  writer.ExportBinaryFile("./test/output/bigtest.nbt");
  writer.ExportTextFile("./test/output/bigtest.snbt");
  writer.ExportTextFile("./test/output/bigtest_oneline.snbt", ImNBT::Writer::PrettyPrint::Disabled);
}

void ReaderTest()
{
  ImNBT::Reader reader;

  if (!reader.ImportTextFile("./test/output/bigtest.snbt"))
  {
    assert(!"Text Import Failed");
  }
  if (!reader.ImportBinaryFileUncompressed("./test/data/bigtest_uncompr"))
  {
    assert(!"Uncompressed Binary Import Failed");
  }
  if (!reader.ImportBinaryFile("./test/output/bigtest.nbt"))
  {
    assert(!"Binary Import Failed");
  }
  auto longTest = reader.ReadLong("longTest");
  assert(longTest == 9223372036854775807ll);
  auto shortTest = reader.ReadShort("shortTest");
  assert(shortTest == 32767);
  auto stringTest = reader.ReadString("stringTest");
  assert(stringTest == u8"HELLO WORLD THIS IS A TEST STRING ÅÄÖ!");
  auto floatTest= reader.ReadFloat("floatTest");
  assert(floatTest == 0.4982315f);
  auto intTest = reader.ReadInt("intTest");
  assert(intTest == 2147483647);
  if (reader.OpenCompound("nested compound test"))
  {
    if (reader.OpenCompound("ham"))
    {
      auto name = reader.ReadString("name");
      assert(name == "Hampus");
      auto value = reader.ReadFloat("value");
      assert(value == 0.75f);
      reader.CloseCompound();
    }
    if (reader.OpenCompound("egg"))
    {
      auto name = reader.ReadString("name");
      assert(name == "Eggbert");
      auto value = reader.ReadFloat("value");
      assert(value == 0.5f);
      reader.CloseCompound();
    }
    reader.CloseCompound();
  }
  if (reader.OpenList("listTest (long)"))
  {
    for (int i = 0; i < 5; ++i)
    {
      auto _ = reader.ReadLong();
      assert(_ == 11ll + i);
    }
    reader.CloseList();
  }
  if (reader.OpenList("listTest (compound)"))
  {
    for (int i = 0; i < 2; ++i)
    {
      if (reader.OpenCompound())
      {
        auto name = reader.ReadString("name");
        assert(name == std::string("Compound tag #") + std::to_string(i));
        auto created_on = reader.ReadLong("created-on");
        assert(created_on == 1264099775885ll);
        reader.CloseCompound();
      }
    }
    reader.CloseList();
  }
  if (reader.OpenList("listTest (end)"))
  {
    reader.CloseList();
  }
  auto byteTest = reader.ReadByte("byteTest");
  assert(byteTest == 127);

  std::vector<int8_t> byteArrayTest = reader.ReadByteArray("byteArrayTest (the first 1000 values of (n*n*255+n*7)%100, starting with n=0 (0, 62, 34, 16, 8, ...))");
  for (size_t n = 0; n < byteArrayTest.size(); ++n)
  {
    assert(byteArrayTest[n] == (n * n * 255 + n * 7) % 100);
  }

  auto doubleTest = reader.ReadDouble("doubleTest");
  assert(doubleTest == 0.493128713218231);

  auto intArrayTest = reader.ReadIntArray("intArrayTest");
  assert((intArrayTest == std::vector{ 66051, 67438087, 134810123, 202182159 }));

  auto longArrayTest = reader.ReadLongArray("longArrayTest");
  assert((longArrayTest == std::vector{ 1003370060459195070, -2401053089480183795 }));
}

void NestingDuplicationTest()
{
  ImNBT::Writer writer;

  if (writer.BeginCompound("base"))
  {
    writer.WriteString("L1", "depth");
    if (writer.BeginCompound("sub compound 1"))
    {
      writer.WriteInt(3, "x");
      writer.EndCompound();
    }
    if (writer.BeginList("list of compounds 1"))
    {
      if (writer.BeginCompound())
      {
        writer.WriteString("L2", "depth");
        if (writer.BeginList("list of compounds 2"))
        {
          if (writer.BeginCompound())
          {
            writer.WriteInt(15, "x");
            writer.EndCompound();
          }
          writer.EndList();
        }
        writer.EndCompound();
      }
      writer.EndList();
    }
    writer.EndCompound();
  }

  writer.Finalize();

  writer.ExportTextFile("./NestingDuplication.test");
}

void ListsOfListsOfListsTest()
{
  ImNBT::Writer writer;

  if (writer.BeginList("base"))
  {
    for (int i = 0; i < 5; ++i)
    {
      if (writer.BeginList())
      {
        for (int j = 0; j < 5; ++j)
        {
          if (writer.BeginList())
          {
            for (int k = 0; k < 5; ++k)
            {
              writer.WriteInt(i * 100 + j * 10 + k);
            }
            writer.EndList();
          }
        }
        writer.EndList();
      }
    }
    writer.EndList();
  }

  writer.Finalize();

  writer.ExportTextFile("./ListsOfListsOfLists.test");
}

int main()
{
  //WriterTest();

  //ReaderTest();

  NestingDuplicationTest();

  ListsOfListsOfListsTest();

  return 0;
}

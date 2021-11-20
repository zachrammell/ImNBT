#include <ImNBT/NBTReader.hpp>
#include <ImNBT/NBTWriter.hpp>

#include <ImNBT/NBTRepresentation.hpp>

#include <array>

void BuilderTest()
{
    ImNBT::Writer builder;

    builder.WriteLong(9223372036854775807ll, "longTest");
    builder.WriteShort(32767, "shortTest");
    builder.WriteString(u8"HELLO WORLD THIS IS A TEST STRING ÅÄÖ!", "stringTest");
    builder.WriteFloat(0.4982315f, "floatTest");
    builder.WriteInt(2147483647, "intTest");
    if (builder.BeginCompound("nested compound test"))
    {
        if (builder.BeginCompound("ham"))
        {
            builder.WriteString("Hampus", "name");
            builder.WriteFloat(0.75f, "value");
            builder.EndCompound();
        }
        if (builder.BeginCompound("egg"))
        {
            builder.WriteString("Eggbert", "name");
            builder.WriteFloat(0.5f, "value");
            builder.EndCompound();
        }
        builder.EndCompound();
    }
    if (builder.BeginList("listTest (long)"))
    {
        for (int i = 0; i < 5; ++i)
        {
            builder.WriteLong(11ll + i);
        }
        builder.EndList();
    }
    if (builder.BeginList("listTest (compound)"))
    {
        for (int i = 0; i < 2; ++i)
        {
            if (builder.BeginCompound())
            {
                std::string name("Compound tag #");
                name += std::to_string(i);
                builder.WriteString(name, "name");
                builder.WriteLong(1264099775885ll, "created-on");
                builder.EndCompound();
            }
        }
        builder.EndList();
    }
    if (builder.BeginList("listTest (end)"))
    {
        builder.EndList();
    }
    builder.WriteByte(127, "byteTest");

    {
        std::array<int8_t, 1000> values {};
        for (int n = 0; n < values.size(); ++n)
        {
            values[n] = (n * n * 255 + n * 7) % 100;
        }
        builder.WriteByteArray(values.data(), values.size(), "byteArrayTest (the first 1000 values of (n*n*255+n*7)%100, starting with n=0 (0, 62, 34, 16, 8, ...))");
    }

    builder.WriteDouble(0.493128713218231, "doubleTest");

    std::array<int, 4> intArrayTest{ 66051, 67438087, 134810123, 202182159 };
    builder.WriteIntArray(intArrayTest.data(), intArrayTest.size(), "intArrayTest");

    std::array<int64_t, 2> longArrayTest{ 1003370060459195070, -2401053089480183795 };
    builder.WriteLongArray(longArrayTest.data(), longArrayTest.size(), "longArrayTest");

    builder.Finalize();

    builder.OutputBinaryFileUncompressed("./test/output/bigtest_uncompr");
    builder.OutputBinaryFile("./test/output/bigtest.nbt");
}

int main()
{
    ImNBT::Reader reader("./test/data/bigtest_uncompr");

    BuilderTest();

    return 0;
}

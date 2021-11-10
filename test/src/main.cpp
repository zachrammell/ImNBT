#include <ImNBT/NBTReader.hpp>
#include <ImNBT/NBTWriter.hpp>

#include <ImNBT/NBTRepresentation.hpp>

class StringReader
{
public:
    explicit StringReader(std::string_view inStr)
        : str(inStr)
    {}

private:
    std::string_view str;
};

void BuilderTest()
{
    ImNBT::Builder builder;

    builder.WriteFloat(3.0f, "Lives");
    if (builder.BeginList("Prices"))
    {
        for (int i = 0; i < 20; ++i)
        {
            builder.WriteFloat((float)i);
        }
        builder.EndList();
    }
    if (builder.BeginList("Lists"))
    {
        for (int i = 0; i < 5; ++i)
        {
            if (builder.BeginList())
            {
                builder.EndList();
            }
        }
        builder.EndList();
    }
    if (builder.BeginCompound("Things"))
    {
        if (builder.BeginList("Vec3"))
        {
            for (int i = 0; i < 3; ++i)
            {
                builder.WriteFloat((float)i * 3.14159);
            }
            builder.EndList();
        }
        builder.WriteFloat(1.0f, "Alpha");
        builder.EndCompound();
    }
    if (builder.BeginList("Empty"))
    {
        builder.EndList();
    }
}

int main()
{
    ImNBT::Reader reader("test/data/bigtest_uncompr");

    BuilderTest();

    return 0;
}

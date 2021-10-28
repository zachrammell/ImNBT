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

int main()
{
    ImNBT::Reader reader("test/data/bigtest_uncompr");
    
    return 0;
}

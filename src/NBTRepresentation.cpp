#include <ImNBT/NBTRepresentation.hpp>

namespace ImNBT
{

bool IsContainer(TAG t)
{
    return t == TAG::List || t == TAG::Compound;
}

}

#include <ImNBT/NBTRepresentation.hpp>

namespace ImNBT
{

namespace Internal
{

bool IsContainer(TAG t) { return t == TAG::List || t == TAG::Compound; }

} // namespace Internal

} // namespace ImNBT

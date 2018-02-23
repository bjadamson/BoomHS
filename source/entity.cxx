#include <boomhs/entity.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// EntityRegistry
EntityID
EntityRegistry::create()
{
  return registry_.create();
}

void
EntityRegistry::destroy(EntityID const eid)
{
  registry_.destroy(eid);
}

} // ns boomhs

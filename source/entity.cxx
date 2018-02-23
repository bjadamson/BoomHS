#include <boomhs/entity.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// EntityRegistry
uint32_t
EntityRegistry::create()
{
  return registry_.create();
}

void
EntityRegistry::destroy(uint32_t const eid)
{
  registry_.destroy(eid);
}

} // ns boomhs

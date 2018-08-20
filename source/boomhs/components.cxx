#include <boomhs/components.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// OrbitalBody
OrbitalBody::OrbitalBody(glm::vec3 const& r, float const o)
    : radius(r)
    , offset(o)
{
}

} // namespace boomhs

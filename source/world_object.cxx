#include <boomhs/world_object.hpp>
#include <stlw/format.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/ext.hpp>

namespace boomhs
{

std::string
WorldObject::display() const
{
  return fmt::sprintf(
      "world_pos: '%s'\nforward_: '%s'\nforward_vector: '%s'\nright_vector: '%s'\nup_vector: '%s'\nquat: '%s'\n",
      glm::to_string(world_position()),
      glm::to_string(forward_),
      glm::to_string(forward_vector()),
      glm::to_string(right_vector()),
      glm::to_string(up_vector()),
      glm::to_string(orientation())
      );
}

void
WorldObject::rotate(float const angle, glm::vec3 const& axis)
{
  glm::quat const new_rotation{axis * glm::radians(angle)};
  transform_->rotation = new_rotation * transform_->rotation;
}

} // ns boomhs

#include <boomhs/player.hpp>
#include <stlw/format.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/ext.hpp>
#include <iostream>

namespace boomhs
{

std::string
Player::display() const
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
Player::rotate(float const angle, glm::vec3 const& axis)
{
  glm::quat const new_rotation{axis * glm::radians(angle)};
  transform_.rotation = new_rotation * transform_.rotation;
  arrow_.rotation = new_rotation * arrow_.rotation;
}

} // ns boomhs

#include <boomhs/raycast.hpp>
#include <boomhs/math.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/screen_info.hpp>

#include <boomhs/frame.hpp>

using namespace boomhs;
namespace sconv = boomhs::math::space_conversions;

namespace boomhs
{

glm::vec3
Raycast::calculate_ray_into_screen(glm::vec2 const& point, glm::mat4 const& proj,
                                   glm::mat4 const& view, FloatRect const& view_rect)
{
  // When doing mouse picking, we want our ray to be pointed "into" the screen
  float constexpr Z            = -1.0f;
  return sconv::screen_to_world(point, view_rect, proj, view, Z);
}

} // namespace boomhs

#include <boomhs/engine.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/math.hpp>
#include <boomhs/raycast.hpp>
#include <boomhs/viewport.hpp>

using namespace boomhs;
namespace sconv = boomhs::math::space_conversions;

namespace boomhs
{

glm::vec3
Raycast::calculate_ray_into_screen(glm::vec2 const& point, glm::mat4 const& proj,
                                   glm::mat4 const& view, Viewport const& vp)
{
  // When doing mouse picking, we want our ray to be pointed "into" the screen
  float constexpr Z    = -1.0f;
  auto const view_rect = vp.size_rect_float();
  return sconv::screen_to_world(point, Z, proj, view, view_rect);
}

} // namespace boomhs

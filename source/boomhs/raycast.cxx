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
  //
  // TODO: This value is different for OpenGL than other GPU libraries, and shouldn't be defined
  // here, but somewhere in the OpenGL code, and somehow passed into this function.
  float constexpr Z    = -1.0f;
  auto const view_rect = vp.size_rect_float();
  auto const wpos = sconv::screen_to_world(point, Z, proj, view, view_rect);

  // The normalized world position IS by definition a Ray (from the origin)
  return glm::normalize(wpos);
}

} // namespace boomhs

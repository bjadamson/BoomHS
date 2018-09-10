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
Raycast::calculate_ray(FrameState& fstate)
{
  // When doing mouse picking, we want our ray to be pointed "into" the screen
  float constexpr Z            = -1.0f;
  auto&           es           = fstate.es;
  auto const&     coords       = es.device_states.mouse.current.coords();
  auto const      mouse_coords = glm::vec2{coords.x, coords.y};

  auto const proj = fstate.projection_matrix();
  auto const view = fstate.view_matrix();

  auto const view_rect = es.dimensions.rect();
  return sconv::screen_to_world(mouse_coords, view_rect, proj, view, Z);
}

} // namespace boomhs

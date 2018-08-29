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

  glm::vec2 const ndc   = sconv::screen_to_ndc(mouse_coords, es.dimensions.as_rectangle());
  glm::vec4 const clip  = sconv::ndc_to_clip(ndc, Z);
  glm::vec4 const eye   = sconv::clip_to_eye(clip, fstate.projection_matrix(), Z);
  glm::vec3 const world = sconv::eye_to_world(eye, fstate.view_matrix());
  return world;
}

} // namespace boomhs

#include <boomhs/dimensions.hpp>
#include <boomhs/mouse_picker.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>

namespace
{
} // namespace

namespace boomhs
{

MousePicker::MousePicker() {}

glm::vec3
MousePicker::calculate_ray(RenderState& rstate) const
{
  auto const calc_ndc = [](glm::vec2 const& scoords, Dimensions const& dim) {
    float const x = ((2.0f * scoords.x) / dim.w) - 1.0f;
    float const y = ((2.0f * scoords.y) / dim.h) - 1.0f;

    auto const assert_fn = [](float const v) {
      assert(v <= 1.0f);
      assert(v >= -1.0f);
    };
    assert_fn(x);
    assert_fn(y);
    return glm::vec2{x, -y};
  };

  // When doing mouse picking, we want our ray to be pointed "into" the screen
  auto constexpr Z         = -1.0f;
  auto const calc_eyespace = [&rstate](glm::vec4 const& clip) {
    auto const      proj_matrix = rstate.projection_matrix();
    auto const      inv_proj    = glm::inverse(proj_matrix);
    glm::vec4 const eye_coords  = inv_proj * clip;
    return glm::vec4{eye_coords.x, eye_coords.y, Z, 0.0f};
  };
  auto const calc_worldspace = [&rstate](auto const& eye) {
    glm::mat4 const inv_view  = glm::inverse(rstate.view_matrix());
    glm::vec4 const ray       = inv_view * eye;
    glm::vec3 const ray_world = glm::vec3{ray.x, ray.y, ray.z};
    return glm::normalize(ray_world);
  };

  auto&           es           = rstate.es;
  auto const&     coords       = es.mouse_state.coords;
  auto const      mouse_coords = glm::vec2{coords.x, coords.y};
  glm::vec2 const ndc          = calc_ndc(mouse_coords, es.dimensions);

  auto& logger = es.logger;

  glm::vec4 const clip       = glm::vec4{ndc.x, ndc.y, Z, 1.0f};
  glm::vec4 const eyespace   = calc_eyespace(clip);
  glm::vec3 const worldspace = calc_worldspace(eyespace);
  return worldspace;
}

} // namespace boomhs

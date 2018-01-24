#pragma once
#include <boomhs/components.hpp>
#include <boomhs/types.hpp>
#include <boomhs/spherical.hpp>
#include <window/mouse.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <glm/glm.hpp>

namespace boomhs
{
class WorldObject;
struct UiState;

struct Projection
{
  float const field_of_view;
  float const viewport_aspect_ratio;
  float const near_plane;
  float const far_plane;
};

class Camera
{
  SphericalCoordinates coordinates_{0.0f, 0.0f, 0.0f};
  float extra_theta_ = 0.0f;
  Projection const projection_;
  EnttLookup player_lookup_;
  glm::vec3 forward_, up_;

  Transform&
  get_target()
  {
    return player_lookup_.lookup<Transform>();
  }

  Transform const&
  get_target() const
  {
    return player_lookup_.lookup<Transform>();
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);
  Camera(Projection const&, EnttLookup const&, glm::vec3 const& f, glm::vec3 const& u);

  glm::mat4
  projection_matrix() const;

  glm::mat4
  view_matrix() const;

  glm::mat4
  camera_matrix() const;

  auto const&
  projection() const
  {
    return projection_;
  }

  glm::vec3
  forward_vector() const;

  void
  set_coordinates(SphericalCoordinates const& sc)
  {
    coordinates_ = sc;
  }

  SphericalCoordinates
  spherical_coordinates() const
  {
    return coordinates_;
  }

  glm::vec3
  local_position() const
  {
    return to_cartesian(coordinates_);
  }

  glm::vec3
  world_position() const
  {
    return target_position() + local_position();
  }

  glm::vec3
  target_position() const
  {
    auto &target = get_target();
    return target.translation;
  }

  void
  rotate_behind_player(stlw::Logger &, WorldObject const&);

  Camera&
  rotate(stlw::Logger &, UiState &, window::mouse_data const&);

  Camera&
  zoom(float const);

  void
  set_target(std::uint32_t const eid)
  {
    player_lookup_.set_eid(eid);
  }
};

} // ns boomhs

#pragma once
#include <boomhs/components.hpp>
#include <boomhs/types.hpp>
#include <boomhs/spherical.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <glm/glm.hpp>

namespace boomhs
{
class WorldObject;
struct MouseState;

struct PerspectiveViewport
{
  float field_of_view;
  float const viewport_aspect_ratio;
  float near_plane;
  float far_plane;
};

struct OrthoProjection
{
  float left, right, bottom, top, far, near;
};

enum CameraMode
{
  Perspective = 0,
  Ortho,
  MAX
};

using ModeNamePair = std::pair<CameraMode, char const*>;
std::array<ModeNamePair, 2> constexpr CAMERA_MODES = {{
  {Perspective, "Perspective"},
  {Ortho, "Ortho"}
}};

class Camera
{
  EnttLookup player_lookup_;
  glm::vec3 forward_, up_;

  SphericalCoordinates coordinates_;
  CameraMode mode_ = Perspective;

  PerspectiveViewport perspective_;
  OrthoProjection ortho_;

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
  Camera(EnttLookup const&, glm::vec3 const& f, glm::vec3 const& u);

  // public fields
  bool flip_y = false;
  bool rotate_lock = false;
  float rotation_speed = 35.0f;

  glm::mat4
  projection_matrix() const;

  glm::mat4
  view_matrix() const;

  glm::mat4
  camera_matrix() const;

  auto const&
  perspective() const
  {
    return perspective_;
  }

  auto
  mode() const
  {
    return mode_;
  }

  void
  set_mode(CameraMode const m)
  {
    mode_ = m;
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

  Camera&
  rotate(float const d_angle, float const d_phi);

  Camera&
  zoom(float const);

  void
  set_target(std::uint32_t const eid)
  {
    player_lookup_.set_eid(eid);
  }

  auto const&
  perspective_ref() const
  {
    return perspective_;
  }

  auto&
  perspective_ref()
  {
    return perspective_;
  }

  auto&
  ortho_ref()
  {
    return ortho_;
  }
};

} // ns boomhs

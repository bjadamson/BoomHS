#pragma once
#include <boomhs/components.hpp>
#include <boomhs/spherical.hpp>
#include <boomhs/types.hpp>

#include <stlw/log.hpp>
#include <stlw/math.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{
class WorldObject;
struct MouseState;

struct PerspectiveViewport
{
  float       field_of_view;
  float const viewport_aspect_ratio;
  float       near_plane;
  float       far_plane;
};

struct OrthoProjection
{
  float left, right, bottom, top, far, near;
};

enum CameraMode
{
  Perspective = 0,
  Ortho,
  FPS,
  MAX
};

using ModeNamePair                                 = std::pair<CameraMode, char const*>;
std::array<ModeNamePair, 3> constexpr CAMERA_MODES = {
    {{Ortho, "Ortho"}, {Perspective, "Perspective"}, {FPS, "FPS"}}};

class Camera
{
  EnttLookup player_lookup_;
  glm::vec3  forward_, up_;

  SphericalCoordinates coordinates_;
  CameraMode           mode_ = Perspective;

  PerspectiveViewport perspective_;
  OrthoProjection     ortho_;

  void zoom(float);

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);
  Camera(EnttLookup const&, glm::vec3 const& f, glm::vec3 const& u);

  // public fields
  bool  flip_y      = false;
  bool  rotate_lock = true;
  float rotation_speed;

  Transform&       get_target() { return player_lookup_.lookup<Transform>(); }
  Transform const& get_target() const { return player_lookup_.lookup<Transform>(); }

  auto mode() const { return mode_; }
  void set_mode(CameraMode const m) { mode_ = m; }
  void next_mode();

  glm::vec3 eye_forward() const { return forward_; }
  glm::vec3 eye_backward() const { return -eye_forward(); }

  glm::vec3 eye_up() const { return up_; }
  glm::vec3 eye_down() const { return -eye_up(); }

  glm::vec3 eye_left() const { return -eye_right(); }
  glm::vec3 eye_right() const { return glm::normalize(glm::cross(eye_forward(), eye_up())); }

  auto const& ortho() const { return ortho_; }
  auto const& perspective() const { return perspective_; }

  glm::vec3 world_forward() const { return glm::normalize(world_position() - target_position()); }

  SphericalCoordinates spherical_coordinates() const { return coordinates_; }
  void                 set_coordinates(SphericalCoordinates const& sc) { coordinates_ = sc; }

  glm::vec3 local_position() const { return to_cartesian(coordinates_); }
  glm::vec3 world_position() const { return target_position() + local_position(); }

  glm::vec3 target_position() const;

  void decrease_zoom(float);
  void increase_zoom(float);

  auto const& perspective_ref() const { return perspective_; }
  auto&       perspective_ref() { return perspective_; }
  auto&       ortho_ref() { return ortho_; }

  Camera& rotate(float, float);
  void    set_target(EntityID const eid) { player_lookup_.set_eid(eid); }
};

glm::mat4
compute_cameramatrix(Camera const&);

glm::mat4
compute_projectionmatrix(CameraMode, PerspectiveViewport const&, OrthoProjection const&);

glm::mat4
compute_projectionmatrix(Camera const&);

glm::mat4
compute_viewmatrix(Camera const&);

glm::mat4
compute_viewmatrix(CameraMode, glm::vec3 const&, glm::vec3 const&, glm::vec3 const&,
                   glm::vec3 const&);

} // namespace boomhs

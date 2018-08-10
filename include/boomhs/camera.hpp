#pragma once
#include <boomhs/components.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/spherical.hpp>

#include <boomhs/math.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{
class ScreenDimensions;
struct MouseState;
struct WorldObject;

struct PerspectiveViewport
{
  float field_of_view;
  float viewport_aspect_ratio;
  float near_plane;
  float far_plane;
};

struct OrthoProjection
{
  float left, right, bottom, top, far, near;

  static OrthoProjection from_ints(int, int, int, int, int, int);
};

enum class CameraMode
{
  ThirdPerson = 0,
  Ortho,
  FPS,
  MAX
};

// clang-format off
using ModeNamePair                                 = std::pair<CameraMode, char const*>;
std::array<ModeNamePair, 3> constexpr CAMERA_MODES = {
    {
      {CameraMode::Ortho,       "Ortho"},
      {CameraMode::ThirdPerson, "ThirdPerson"},
      {CameraMode::FPS,         "FPS"}
    }
};
// clang-format on

class Camera
{
  WorldObject* target_ = nullptr;
  glm::vec3    forward_, up_;

  SphericalCoordinates coordinates_;
  CameraMode           mode_ = CameraMode::ThirdPerson;

  PerspectiveViewport perspective_;
  OrthoProjection     ortho_;

  void check_pointers() const;
  void zoom(float);

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);
  Camera(ScreenDimensions const&, glm::vec3 const&, glm::vec3 const&);

  // public fields
  bool  flip_y;
  bool  rotate_lock;
  float rotation_speed;

  WorldObject&       get_target();
  WorldObject const& get_target() const;

  auto mode() const { return mode_; }
  void set_mode(CameraMode);
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

  glm::vec3 local_position() const;
  glm::vec3 world_position() const;

  glm::vec3 target_position() const;

  void decrease_zoom(float);
  void increase_zoom(float);

  auto const& perspective_ref() const { return perspective_; }
  auto&       perspective_ref() { return perspective_; }
  auto&       ortho_ref() { return ortho_; }

  Camera& rotate(float, float);
  void    set_target(WorldObject&);

  // static fns
  static Camera make_defaultcamera(ScreenDimensions const&);

  static glm::mat4
  compute_projectionmatrix(CameraMode, PerspectiveViewport const&, OrthoProjection const&);

  static glm::mat4 compute_viewmatrix(CameraMode, glm::vec3 const&, glm::vec3 const&,
                                      glm::vec3 const&, glm::vec3 const&);
};

} // namespace boomhs

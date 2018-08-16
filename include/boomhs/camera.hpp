#pragma once
#include <boomhs/components.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/spherical.hpp>

#include <boomhs/math.hpp>
#include <common/log.hpp>
#include <common/type_macros.hpp>

namespace boomhs
{
class ScreenDimensions;
struct WorldObject;

class AspectRatio
{
  std::array<float, 2> nd_;

public:
  explicit constexpr AspectRatio(float const np, float const dp)
      : nd_({np, dp})
  {
  }

  float constexpr compute() const { return nd_[0] / nd_[1]; }

  float*       data() { return nd_.data(); }
  float const* data() const { return nd_.data(); }
};

struct Viewport
{
  AspectRatio aspect_ratio;
  float       field_of_view;
  Frustum     frustum;
};

enum class CameraMode
{
  ThirdPerson = 0,
  Ortho,
  FPS,
  MAX
};

// clang-format off
struct CameraModes
{
  using ModeNamePair = std::pair<CameraMode, char const*>;
  CameraModes() = delete;

  static std::array<ModeNamePair, 3> constexpr CAMERA_MODES = {
    {
      {CameraMode::Ortho,       "Ortho"},
      {CameraMode::ThirdPerson, "ThirdPerson"},
      {CameraMode::FPS,         "FPS"}
    }
  };

  static std::vector<std::string> string_list();
};

// clang-format on
class CameraFPS
{
public:
  CameraFPS() = default;
  MOVE_CONSTRUCTIBLE_ONLY(CameraFPS);

  glm::vec3 world_position() const;
};

class CameraArcball
{
  WorldObject* target_ = nullptr;
  glm::vec3    forward_, up_;

  SphericalCoordinates coordinates_;

  void zoom(float);
  void check_pointers() const;

  friend class Camera;
public:
  CameraArcball(glm::vec3 const&, glm::vec3 const&);
  MOVE_CONSTRUCTIBLE_ONLY(CameraArcball);

  // fields
  bool  flip_y = false;
  bool  rotate_lock = false;
  float rotation_speed;

  // methods
  SphericalCoordinates spherical_coordinates() const { return coordinates_; }
  void                 set_coordinates(SphericalCoordinates const& sc) { coordinates_ = sc; }

  WorldObject&       get_target();
  WorldObject const& get_target() const;

  void decrease_zoom(float);
  void increase_zoom(float);

  CameraArcball& rotate(float, float);

  glm::vec3 local_position() const;
  glm::vec3 world_position() const;

  glm::vec3 target_position() const;
};

class Camera
{
  WorldObject* target_ = nullptr;
  glm::vec3    forward_, up_;

  CameraMode mode_ = CameraMode::ThirdPerson;
  Viewport viewport_;

  void check_pointers() const;
public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);
  Camera(ScreenDimensions const&, Viewport&&, glm::vec3 const&, glm::vec3 const&);

  // public fields
  CameraArcball arcball;
  CameraFPS     fps;

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

  glm::vec3 world_forward() const { return glm::normalize(arcball.world_position() - arcball.target_position()); }
  glm::vec3 world_position() const { return arcball.world_position(); }

  auto const& viewport_ref() const { return viewport_; }
  auto&       viewport_ref() { return viewport_; }

  auto const& frustum_ref() const { return viewport_.frustum; }
  auto&       frustum_ref() { return viewport_.frustum; }

  Camera& rotate(float, float);
  void    set_target(WorldObject&);

  glm::mat4 compute_projectionmatrix() const;
  glm::mat4 compute_viewmatrix(glm::vec3 const&) const;

  // static fns
  static Camera make_default(ScreenDimensions const&);
};

} // namespace boomhs

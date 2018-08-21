#pragma once
#include <boomhs/frame.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/spherical.hpp>
#include <boomhs/world_object.hpp>

#include <boomhs/math.hpp>
#include <common/log.hpp>
#include <common/type_macros.hpp>

namespace window
{
class SDLWindow;
} // namespace window

namespace boomhs
{
class FrameTime;
class ScreenDimensions;

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

class CameraTarget
{
  WorldObject* wo_;

  // methods
  void validate() const;

  friend class Camera;

public:
  CameraTarget() = default;

  WorldObject& get()
  {
    validate();
    return *wo_;
  }

  WorldObject const& get() const
  {
    validate();
    return *wo_;
  }

  void set(WorldObject& wo) { wo_ = &wo; }
};

struct CameraState
{
  DeviceSensitivity sensitivity;

  bool rotation_lock;

  bool flip_y = false;
  bool flip_x = false;
};

class CameraFPS
{
  glm::vec3 forward_, up_;
  float xrot_, yrot_;

  CameraTarget& target_;
  Viewport&     viewport_;

  friend class Camera;

  auto&       transform() { return target_.get().transform(); }
  auto const& transform() const { return target_.get().transform(); }

public:
  CameraFPS(glm::vec3 const&, glm::vec3 const&, CameraTarget&, Viewport&);
  MOVE_CONSTRUCTIBLE_ONLY(CameraFPS);

  // fields
  CameraState cs;

  // methods
  CameraFPS& rotate_radians(float, float, FrameTime const&);

  void update(int, int, ScreenDimensions const&, window::SDLWindow&);
  glm::vec3 world_position() const;

  glm::mat4 compute_projectionmatrix() const;
  glm::mat4 compute_viewmatrix(glm::vec3 const&) const;
};

class CameraORTHO
{
  glm::vec3 forward_, up_;

  CameraTarget& target_;
  Viewport&     viewport_;
  friend class Camera;

public:
  CameraORTHO(glm::vec3 const&, glm::vec3 const&, CameraTarget&, Viewport&);
  MOVE_CONSTRUCTIBLE_ONLY(CameraORTHO);

  glm::mat4 compute_projectionmatrix() const;
  glm::mat4 compute_viewmatrix(glm::vec3 const&) const;
};

class CameraArcball
{
  glm::vec3 forward_, up_;

  CameraTarget& target_;
  Viewport&     viewport_;

  SphericalCoordinates coordinates_;

  void zoom(float, FrameTime const&);

  friend class Camera;

public:
  CameraArcball(glm::vec3 const&, glm::vec3 const&, CameraTarget&, Viewport&);
  MOVE_CONSTRUCTIBLE_ONLY(CameraArcball);

  // fields
  CameraState   cs;

  // methods
  SphericalCoordinates spherical_coordinates() const { return coordinates_; }
  void                 set_coordinates(SphericalCoordinates const& sc) { coordinates_ = sc; }

  void decrease_zoom(float, FrameTime const&);
  void increase_zoom(float, FrameTime const&);

  CameraArcball& rotate_radians(float, float, FrameTime const&);

  glm::vec3 local_position() const;
  glm::vec3 world_position() const;

  glm::vec3 target_position() const;

  glm::mat4 compute_projectionmatrix() const;
  glm::mat4 compute_viewmatrix(glm::vec3 const&) const;
};

class Camera
{
  CameraTarget target_;
  Viewport     viewport_;
  CameraMode   mode_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);
  Camera(Viewport&&, glm::vec3 const&, glm::vec3 const&);

  // public fields
  CameraArcball arcball;
  CameraFPS     fps;
  CameraORTHO   ortho;

  WorldObject&       get_target();
  WorldObject const& get_target() const;

  auto mode() const { return mode_; }
  void set_mode(CameraMode);
  void next_mode();
  bool is_firstperson() const { return CameraMode::FPS == mode(); }
  bool is_thirdperson() const { return CameraMode::ThirdPerson == mode(); }

  void toggle_rotation_lock();

  glm::vec3 eye_forward() const;
  glm::vec3 eye_backward() const { return -eye_forward(); }
  glm::vec3 eye_up() const { return world_up() * get_target().orientation(); }

  glm::vec3 eye_left() const { return -eye_right(); }
  glm::vec3 eye_right() const { return glm::normalize(glm::cross(eye_forward(), eye_up())); }

  glm::vec3 world_forward() const;
  glm::vec3 world_up() const;
  glm::vec3 world_position() const;

  auto const& viewport_ref() const { return viewport_; }
  auto&       viewport_ref() { return viewport_; }

  auto const& frustum_ref() const { return viewport_.frustum; }
  auto&       frustum_ref() { return viewport_.frustum; }

  Camera& rotate_radians(float, float, FrameTime const&);
  void    set_target(WorldObject&);

  glm::mat4 compute_projectionmatrix() const;
  glm::mat4 compute_viewmatrix(glm::vec3 const&) const;

  // static fns
  static Camera make_default(ScreenDimensions const&);
};

} // namespace boomhs

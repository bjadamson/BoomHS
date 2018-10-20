#pragma once
#include <boomhs/frame.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/spherical.hpp>
#include <boomhs/world_object.hpp>

#include <boomhs/math.hpp>
#include <common/log.hpp>
#include <common/type_macros.hpp>

namespace gl_sdl
{
class SDLWindow;
} // namespace gl_sdl

namespace boomhs
{
class  FrameTime;
struct ScreenSize;
class  Viewport;

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

struct ViewSettings
{
  AspectRatio aspect_ratio;
  float       field_of_view;
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
  float     xrot_, yrot_;
  WorldOrientation const orientation_;

  CameraTarget& target_;

  friend class Camera;

  auto&       transform() { return target_.get().transform(); }
  auto const& transform() const { return target_.get().transform(); }

  auto const& forward() const { return orientation_.forward; }
  auto const& up() const { return orientation_.up; }

public:
  CameraFPS(CameraTarget&, WorldOrientation const&);
  MOVE_CONSTRUCTIBLE_ONLY(CameraFPS);

  // fields
  CameraState cs;

  // methods
  CameraFPS& rotate_radians(float, float, FrameTime const&);

  glm::vec3 world_position() const;
  glm::mat4 calc_pm(ViewSettings const&, Frustum const&) const;
  glm::mat4 calc_vm(glm::vec3 const&) const;
};

class CameraORTHO
{
  WorldOrientation const orientation_;

  glm::vec2 zoom_;
  friend class Camera;

  auto const& forward() const { return orientation_.forward; }
  auto const& up() const { return orientation_.up; }

public:
  CameraORTHO(WorldOrientation const&);
  MOVE_CONSTRUCTIBLE_ONLY(CameraORTHO);

  // fields
  static glm::vec3 constexpr EYE_FORWARD = math::constants::Y_UNIT_VECTOR;
  glm::vec3 position;

  // methods
  glm::mat4 calc_pm(AspectRatio const&, Frustum const&, ScreenSize const&, glm::vec2 const&) const;
  glm::mat4 calc_vm() const;

  void grow_view(glm::vec2 const&);
  void shink_view(glm::vec2 const&);

  void scroll(glm::vec2 const&);
  auto zoom() const { return zoom_; }
};

class CameraArcball
{
  CameraTarget&        target_;
  SphericalCoordinates coordinates_;

  WorldOrientation     orientation_;
  glm::vec3 const      world_up_;

  // methods
  void zoom(float, FrameTime const&);

  auto forward() const { return orientation_.forward; }
  auto up() const { return orientation_.up; }
  auto orientation() const { return WorldOrientation{forward(), up()}; }

  friend class Camera;

public:
  CameraArcball(CameraTarget&, WorldOrientation const&);
  MOVE_CONSTRUCTIBLE_ONLY(CameraArcball);

  // fields
  CameraState cs;

  // methods
  SphericalCoordinates spherical_coordinates() const { return coordinates_; }
  void                 set_coordinates(SphericalCoordinates const& sc) { coordinates_ = sc; }

  void decrease_zoom(float, FrameTime const&);
  void increase_zoom(float, FrameTime const&);

  CameraArcball& rotate_radians(float, float, FrameTime const&);

  glm::vec3 local_position() const;
  glm::vec3 world_position() const;

  glm::vec3 target_position() const;

  glm::mat4 calc_pm(ViewSettings const&, Frustum const&) const;
  glm::mat4 calc_vm(glm::vec3 const&) const;
};

class Camera
{
  CameraTarget            target_;
  ViewSettings            view_settings_;
  CameraMode              mode_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);
  Camera(ViewSettings&&, WorldOrientation const&, WorldOrientation const&);

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
  glm::vec3 eye_up() const;

  glm::vec3 eye_left() const { return -eye_right(); }
  glm::vec3 eye_right() const { return glm::normalize(glm::cross(eye_forward(), eye_up())); }

  glm::vec3 world_forward() const;
  glm::vec3 world_up() const;
  glm::vec3 world_position() const;
  WorldOrientation const& orientation_ref() const;

  auto const& view_settings_ref() const { return view_settings_; }
  auto&       view_settings_ref() { return view_settings_; }

  Camera& rotate_radians(float, float, FrameTime const&);
  void    set_target(WorldObject&);

  // static fns
  static Camera make_default(WorldOrientation const&, WorldOrientation const&);
};

} // namespace boomhs

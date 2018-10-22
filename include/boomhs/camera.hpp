#pragma once
#include <boomhs/frame.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/spherical.hpp>
#include <boomhs/world_object.hpp>

#include <boomhs/math.hpp>
#include <common/log.hpp>
#include <common/type_macros.hpp>

// TODO
#include <iostream>

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

#define CAMERA_TARGET_BODY_IMPL assert(target_); return *target_;
#define CAMERA_CLASS_TARGET_IMPL                                                                   \
  auto& target() { CAMERA_TARGET_BODY_IMPL }                                                       \
  auto const& target() const { CAMERA_TARGET_BODY_IMPL }

using CameraPosition = glm::vec3;
using CameraCenter   = glm::vec3;
using CameraForward  = glm::vec3;
using CameraUp       = glm::vec3;

/*
 * Explicit copy method.
 *
 * Rationale: It is easy to accidentally take a reference to a copy of an instance of this class.
 * To make it more unlikely, force the user to call clone().
*/
#define CLONE_CAMERA_IMPL                                                                          \
auto clone() const { return *this; }

class CameraFPS
{
  float            xrot_, yrot_;
  WorldOrientation orientation_;

  CameraTarget* target_;

  CAMERA_CLASS_TARGET_IMPL

  auto&       transform() { return target().get().transform(); }
  auto const& transform() const { return target().get().transform(); }

  auto eye_forward() const { return orientation_.forward * transform().rotation; }
  auto const& eye_up() const { return orientation_.up; }

  friend class Camera;
  COPY_DEFAULT(CameraFPS);
public:
  CameraFPS(WorldOrientation const&);
  MOVE_DEFAULT(CameraFPS);

  // fields
  CameraState cs;

  // methods
  CameraFPS& rotate_radians(float, float, FrameTime const&);

  glm::vec3 position() const;
  glm::mat4 calc_pm(ViewSettings const&, Frustum const&) const;
  glm::mat4 calc_vm(glm::vec3 const&) const;

  CLONE_CAMERA_IMPL
};

class CameraORTHO
{
  WorldOrientation orientation_;

  glm::vec2 zoom_;
  void zoom(glm::vec2 const&, FrameTime const&);

  COPY_DEFAULT(CameraORTHO);
  friend class Camera;

public:
  CameraORTHO(WorldOrientation const&, glm::vec3 const&);
  MOVE_DEFAULT(CameraORTHO);

  // fields
  glm::vec3 position;
  bool flip_rightv = false;

  // methods
  // Compute the projection-matrix.
  glm::mat4 calc_pm(AspectRatio const&, Frustum const&, ScreenSize const&) const;

  // Compute the view-matrix.
  glm::mat4 calc_vm() const;

  auto const& eye_forward() const { return orientation_.forward; }
  auto const& eye_up() const { return orientation_.up; }

  void zoom_in(glm::vec2 const&, FrameTime const&);
  void zoom_out(glm::vec2 const&, FrameTime const&);

  void scroll(glm::vec2 const&);
  auto const& zoom() const { return zoom_; }

  // Compute the projection-matrix.
  static glm::mat4 compute_pm(AspectRatio const&, Frustum const&, ScreenSize const&,
                              CameraPosition const&, glm::ivec2 const&);

  // Compute the view-matrix.
  static glm::mat4 compute_vm(CameraPosition const&, CameraCenter const&, CameraUp const&, bool);
};

class CameraArcball
{
  CameraTarget*        target_;
  SphericalCoordinates coordinates_;

  bool up_inverted_;
  glm::vec3            world_up_;

  // methods
  void zoom(float, FrameTime const&);
  glm::vec3 local_position() const;

  auto eye_forward() const {
    return glm::normalize(position() - target_position());
  }
  auto eye_up() const { return up_inverted_ ? -world_up_ : world_up_; }

  CAMERA_CLASS_TARGET_IMPL
#undef CAMERA_CLASS_TARGET_IMPL
#undef CAMERA_TARGET_BODY_IMPL

  friend class Camera;
  COPY_DEFAULT(CameraArcball);

public:
  CameraArcball(glm::vec3 const&);
  MOVE_DEFAULT(CameraArcball);

  // fields
  CameraState cs;

  // methods
  SphericalCoordinates spherical_coordinates() const { return coordinates_; }
  void                 set_coordinates(SphericalCoordinates const& sc) { coordinates_ = sc; }

  void zoom_out(float, FrameTime const&);
  void zoom_in(float, FrameTime const&);

  CameraArcball& rotate_radians(float, float, FrameTime const&);

  glm::vec3 position() const;

  glm::vec3 target_position() const;

  glm::mat4 calc_pm(ViewSettings const&, Frustum const&) const;
  glm::mat4 calc_vm(glm::vec3 const&) const;

  CLONE_CAMERA_IMPL
};

class Camera
{
  CameraTarget            target_;
  ViewSettings            view_settings_;
  CameraMode              mode_;

  COPY_DEFAULT(Camera);
public:
  MOVE_DEFAULT(Camera);
  Camera(CameraMode, ViewSettings&&, CameraArcball&&, CameraFPS&&, CameraORTHO&&);

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

  glm::vec3 position() const;

  auto const& view_settings_ref() const { return view_settings_; }
  auto&       view_settings_ref() { return view_settings_; }

  Camera& rotate_radians(float, float, FrameTime const&);
  void    set_target(WorldObject&);

  glm::mat4 calc_pm(ViewSettings const&, Frustum const&, ScreenSize const&) const;
  glm::mat4 calc_vm(glm::vec3 const&) const;

  void zoom_out(float, FrameTime const&);
  void zoom_in(float, FrameTime const&);

  CLONE_CAMERA_IMPL

  // static fns
  static Camera make_default(CameraMode, WorldOrientation const&, WorldOrientation const&);
};

#undef CLONE_CAMERA_IMPL

} // namespace boomhs

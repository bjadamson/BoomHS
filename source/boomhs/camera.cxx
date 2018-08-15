#include <boomhs/camera.hpp>
#include <boomhs/screen_size.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

#include <boomhs/math.hpp>
#include <extlibs/imgui.hpp>

using namespace boomhs::math::constants;
using namespace opengl;

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//// Camera
Camera::Camera(ScreenDimensions const& dimensions, glm::vec3 const& forward, glm::vec3 const& up)
    : forward_(forward)
    , up_(up)
    , coordinates_(0.0f, 0.0f, 0.0f)
    , perspective_({110.0f, 4.0f / 3.0f, 0.1f, 2000.0f})
    , frustum_(
        Frustum::from_ints(
          dimensions.left(),
          dimensions.right(),
          dimensions.top(),
          dimensions.bottom(),
          -1,
          1))
    , flip_y(false)
    , rotate_lock(false)
    , rotation_speed(600.0)
{
}

void
Camera::check_pointers() const
{
  assert(nullptr != target_);
}

#define GET_TARGET_IMPL()                                                                          \
  check_pointers();                                                                                \
  return *target_

WorldObject&
Camera::get_target()
{
  GET_TARGET_IMPL();
}

WorldObject const&
Camera::get_target() const
{
  GET_TARGET_IMPL();
}
#undef GET_TARGET_IMPL

void
Camera::set_mode(CameraMode const m)
{
  mode_ = m;
}

void
Camera::next_mode()
{
  check_pointers();

  auto const cast = [](auto const v) { return static_cast<int>(v); };
  CameraMode const m = static_cast<CameraMode>(cast(mode()) + cast(1));
  if (CameraMode::MAX == m) {
    set_mode(static_cast<CameraMode>(0));
  }
  else {
    set_mode(m);
  }
}

Camera&
Camera::rotate(float const d_theta, float const d_phi)
{
  check_pointers();

  float constexpr PI     = glm::pi<float>();
  float constexpr TWO_PI = PI * 2.0f;

  auto& theta = coordinates_.theta;
  theta       = (up_.y > 0.0f) ? (theta - d_theta) : (theta + d_theta);
  if (theta > TWO_PI) {
    theta -= TWO_PI;
  }
  else if (theta < -TWO_PI) {
    theta += TWO_PI;
  }

  auto&       phi     = coordinates_.phi;
  float const new_phi = flip_y ? (phi + d_phi) : (phi - d_phi);
  bool const  top_hemisphere =
      (new_phi > 0 && new_phi < (PI / 2.0f)) || (new_phi < -(PI / 2.0f) && new_phi > -TWO_PI);
  if (!rotate_lock || top_hemisphere) {
    phi = new_phi;
  }

  // Keep phi within -2PI to +2PI for easy 'up' comparison
  if (phi > TWO_PI) {
    phi -= TWO_PI;
  }
  else if (phi < -TWO_PI) {
    phi += TWO_PI;
  }

  // If phi is between 0 to PI or -PI to -2PI, make 'up' be positive Y, other wise make it negative
  // Y
  auto& up = up_;
  if ((phi > 0 && phi < PI) || (phi < -PI && phi > -TWO_PI)) {
    up = Y_UNIT_VECTOR;
  }
  else {
    up = -Y_UNIT_VECTOR;
  }
  return *this;
}

void
Camera::set_target(WorldObject& target)
{
  target_ = &target;
}

void
Camera::zoom(float const amount)
{
  check_pointers();

  float constexpr MIN_RADIUS = 0.01f;
  float const new_radius     = coordinates_.radius + amount;
  if (new_radius >= MIN_RADIUS) {
    coordinates_.radius = new_radius;
  }
  else {
    coordinates_.radius = MIN_RADIUS;
  }
}

void
Camera::decrease_zoom(float const amount)
{
  check_pointers();
  zoom(-amount);
}

void
Camera::increase_zoom(float const amount)
{
  check_pointers();
  zoom(amount);
}

glm::vec3
Camera::local_position() const
{
  check_pointers();
  return to_cartesian(coordinates_);
}

glm::vec3
Camera::world_position() const
{
  return target_position() + local_position();
}

glm::vec3
Camera::target_position() const
{
  check_pointers();

  auto& target = get_target().transform();
  return target.translation;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// free functions
Camera
Camera::make_defaultcamera(ScreenDimensions const& dimensions)
{
  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const FORWARD = -Z_UNIT_VECTOR;
  auto constexpr UP  = Y_UNIT_VECTOR;

  Camera camera(dimensions, FORWARD, UP);

  SphericalCoordinates sc;
  sc.radius = 3.8f;
  sc.theta  = glm::radians(-0.229f);
  sc.phi    = glm::radians(38.2735f);
  camera.set_coordinates(sc);

  return camera;
}

glm::mat4
Camera::compute_projectionmatrix() const
{
  auto const& p = perspective_;
  auto const& f = frustum_;
  auto const fov = glm::radians(p.field_of_view);

  switch (mode()) {
  case CameraMode::ThirdPerson:
  case CameraMode::FPS:
    return glm::perspective(fov, p.viewport_aspect_ratio, p.near_plane, p.far_plane);
  case CameraMode::Ortho: {
    return glm::ortho(f.left, f.right, f.bottom, f.top, f.far, f.near);
  }
  case CameraMode::MAX:
    break;
  }

  std::abort();
  return glm::mat4{}; // appease compiler
}

glm::mat4
Camera::compute_viewmatrix(glm::vec3 const& pos) const
{
  auto const& target  = get_target().transform().translation;
  auto const& up      = eye_up();
  auto const& forward = world_forward();

  switch (mode()) {
    case CameraMode::Ortho: {
    return glm::lookAt(ZERO, target, Y_UNIT_VECTOR);
  }
  case CameraMode::FPS: {
    return glm::lookAt(target, target + forward, Y_UNIT_VECTOR);
  }
  case CameraMode::ThirdPerson: {
    return glm::lookAt(pos, target, up);
  }
  case CameraMode::MAX: {
    break;
  }
  }
  std::abort();
  return glm::mat4{}; // appease compiler
}

} // namespace boomhs

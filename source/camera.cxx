#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/constants.hpp>
#include <stlw/math.hpp>
#include <window/mouse.hpp>

using namespace opengl;

namespace boomhs
{

Camera::Camera(Transform* target, glm::vec3 const& forward, glm::vec3 const& up)
    : target_(target)
    , forward_(forward)
    , up_(up)
    , coordinates_(0.0f, 0.0f, 0.0f)
    , perspective_({70.0f, 4.0f / 3.0f, 0.1f, 2000.0f})
    , ortho_({-10, 10, -10, 10, -200, 200})
    , flip_y(false)
    , rotate_lock(false)
    , rotation_speed(600.0)
{
}

void
Camera::next_mode()
{
  assert(nullptr != target_);

  CameraMode const m = static_cast<CameraMode>(mode() + 1);
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
  assert(nullptr != target_);

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
Camera::set_target(Transform& target)
{
  target_ = &target;
}

void
Camera::zoom(float const amount)
{
  assert(nullptr != target_);

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
  assert(nullptr != target_);
  zoom(-amount);
}

void
Camera::increase_zoom(float const amount)
{
  assert(nullptr != target_);
  zoom(amount);
}

glm::vec3
Camera::local_position() const
{
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
  assert(nullptr != target_);

  auto& target = get_target();
  return target.translation;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// free functions
Camera
Camera::make_defaultcamera()
{
  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const FORWARD = -Z_UNIT_VECTOR;
  auto constexpr UP  = Y_UNIT_VECTOR;

  Camera camera(nullptr, FORWARD, UP);

  SphericalCoordinates sc;
  sc.radius = 3.8f;
  sc.theta  = glm::radians(-0.229f);
  sc.phi    = glm::radians(38.2735f);
  camera.set_coordinates(sc);

  return camera;
}

glm::mat4
Camera::compute_projectionmatrix(CameraMode const mode, PerspectiveViewport const& p,
                                 OrthoProjection const& o)
{
  auto const fov = glm::radians(p.field_of_view);
  switch (mode) {
  case Perspective:
  case FPS:
    return glm::perspective(fov, p.viewport_aspect_ratio, p.near_plane, p.far_plane);
  case Ortho: {
    return glm::ortho(o.left, o.right, o.bottom, o.top, o.far, o.near);
  }
  case MAX:
    break;
  }

  std::abort();
  return glm::mat4{}; // appease compiler
}

glm::mat4
Camera::compute_viewmatrix(CameraMode const mode, glm::vec3 const& eye, glm::vec3 const& center,
                           glm::vec3 const& up, glm::vec3 const& fps_center)
{
  auto constexpr ZERO = glm::vec3{0};

  switch (mode) {
  case Ortho: {
    return glm::lookAt(ZERO, center, Y_UNIT_VECTOR);
  }
  case FPS: {
    return glm::lookAt(center, fps_center, Y_UNIT_VECTOR);
  }
  case Perspective: {
    return glm::lookAt(eye, center, up);
  }
  case MAX: {
    break;
  }
  }
  std::abort();
  return glm::mat4{}; // appease compiler
}

} // namespace boomhs

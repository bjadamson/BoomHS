#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/constants.hpp>
#include <stlw/math.hpp>
#include <window/mouse.hpp>

using namespace opengl;

/*
namespace
{

glm::vec3
calculate_mouse_worldpos(Camera const& camera, WorldObject const& player, int const mouse_x,
    int const mouse_y, Dimensions const& dimensions)
{
  auto const& a = camera.perspective_ref();
  glm::vec4 const viewport = glm::vec4(0, 0, 1024, 768);
  glm::mat4 const view = camera.view_matrix();
  glm::mat4 const projection = camera.projection_matrix();
  float const height = dimensions.h;

  // Calculate the view-projection matrix.
  glm::mat4 const transform = projection * view;

  // Calculate the intersection of the mouse ray with the near (z=0) and far (z=1) planes.
  glm::vec3 const near = glm::unProject(glm::vec3{mouse_x, dimensions.h - mouse_y, 0}, glm::mat4(),
transform, viewport); glm::vec3 const far = glm::unProject(glm::vec3{mouse_x, dimensions.h -
mouse_y, 1}, glm::mat4(), transform, viewport);

  auto const z = 0.0f;
  glm::vec3 const world_pos = glm::mix(near, far, ((z - near.z) / (far.z - near.z)));

  //float const Z_PLANE = 0.0;
  //assert(768 == dimensions.h);
  //glm::vec3 screen_pos = glm::vec3(mouse_x, (dimensions.h - mouse_y), Z_PLANE);

  //glm::vec3 const world_pos = glm::unProject(screen_pos, view, projection, viewport);
  return world_pos;
}

} // ns anon
*/

namespace boomhs
{

Camera::Camera(Transform* target, glm::vec3 const& forward, glm::vec3 const& up)
    : target_(target)
    , forward_(forward)
    , up_(up)
    , coordinates_(0.0f, 0.0f, 0.0f)
    , perspective_({90.0f, 4.0f / 3.0f, 0.1f, 2000.0f})
    , ortho_({-10, 10, -10, 10, -200, 200})
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
make_defaultcamera()
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
compute_projectionmatrix(CameraMode const mode, PerspectiveViewport const& p,
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
compute_projectionmatrix(Camera const& camera)
{
  auto const  mode        = camera.mode();
  auto const& perspective = camera.perspective();
  auto const& ortho       = camera.ortho();
  return compute_projectionmatrix(mode, perspective, ortho);
}

glm::mat4
compute_viewmatrix(CameraMode const mode, glm::vec3 const& eye, glm::vec3 const& center,
                   glm::vec3 const& up, glm::vec3 const& fps_center)
{
  auto constexpr ZERO = glm::vec3{0, 0, 0};

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

glm::mat4
compute_viewmatrix(Camera const& camera)
{
  auto const  mode         = camera.mode();
  auto const& target       = camera.get_target().translation;
  auto const  position_xyz = camera.world_position();
  auto const& up           = camera.eye_up();
  auto const& fps_center   = camera.world_forward() + target;

  return compute_viewmatrix(mode, position_xyz, target, up, fps_center);
}

glm::mat4
compute_cameramatrix(Camera const& camera)
{
  return compute_projectionmatrix(camera) * compute_viewmatrix(camera);
}

} // namespace boomhs

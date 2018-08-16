#include <boomhs/camera.hpp>
#include <boomhs/screen_size.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

#include <boomhs/math.hpp>
#include <extlibs/imgui.hpp>

using namespace boomhs;
using namespace boomhs::math::constants;
using namespace opengl;

auto
make_viewport(float const fov, ScreenDimensions const& dimensions, AspectRatio const ar,
              float const near, float const far)
{
  Frustum frustum{
      static_cast<float>(dimensions.left()),
      static_cast<float>(dimensions.right()),
      static_cast<float>(dimensions.bottom()),
      static_cast<float>(dimensions.top()),
      near,
      far};
  return Viewport{ar, fov, MOVE(frustum)};
}

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// CameraModes
std::vector<std::string>
CameraModes::string_list()
{
  std::vector<std::string> result;
  for (auto const& it : CAMERA_MODES) {
    result.emplace_back(it.second);
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CameraArcball
CameraArcball::CameraArcball(glm::vec3 const& forward, glm::vec3 const& up, CameraTarget& t,
                             Viewport& vp)
    : forward_(forward)
    , up_(up)
    , target_(t)
    , viewport_(vp)
    , coordinates_(0.0f, 0.0f, 0.0f)
    , rotation_speed(600.0)
{
}

void
CameraArcball::zoom(float const amount)
{
  float constexpr MIN_RADIUS = 0.01f;
  float const new_radius     = coordinates_.radius + amount;

  coordinates_.radius = (new_radius >= MIN_RADIUS)
    ? new_radius
    : MIN_RADIUS;
}

void
CameraArcball::decrease_zoom(float const amount)
{
  zoom(-amount);
}

void
CameraArcball::increase_zoom(float const amount)
{
  zoom(amount);
}

CameraArcball&
CameraArcball::rotate(float const d_theta, float const d_phi)
{
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

glm::vec3
CameraArcball::local_position() const
{
  return to_cartesian(coordinates_);
}

glm::vec3
CameraArcball::world_position() const
{
  return target_position() + local_position();
}

glm::vec3
CameraArcball::target_position() const
{
  auto& target = target_.get().transform();
  return target.translation;
}

glm::mat4
CameraArcball::compute_projectionmatrix() const
{
  auto const ar  = viewport_.aspect_ratio.compute();
  auto const fov = glm::radians(viewport_.field_of_view);
  auto const& f  = viewport_.frustum;

  return glm::perspective(fov, ar, f.near, f.far);
}

glm::mat4
CameraArcball::compute_viewmatrix(glm::vec3 const& target_pos) const
{
  auto const& target  = target_.get().transform().translation;
  return glm::lookAt(target_pos, target, up_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CameraFPS
CameraFPS::CameraFPS(glm::vec3 const& forward, glm::vec3 const& up, CameraTarget& t, Viewport& vp)
    : forward_(forward)
    , up_(up)
    , target_(t)
    , viewport_(vp)
{
}

glm::mat4
CameraFPS::compute_projectionmatrix() const
{
  auto const ar  = viewport_.aspect_ratio.compute();
  auto const fov = glm::radians(viewport_.field_of_view);
  auto const& f  = viewport_.frustum;
  return glm::perspective(fov, ar, f.near, f.far);
}

glm::mat4
CameraFPS::compute_viewmatrix(glm::vec3 const& world_forward) const
{
  auto const pos = target_.get().transform().translation;

  // NOTE: this might need to be changed back to (pos +  world_forward)
  //
  // I changed it temporarily to make FPS camera work, but I think it will be wrong
  // again in the future when the FPS camera doesn't use the ARCBALL camera for anything.
  return glm::lookAt(pos, pos - world_forward, up_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CameraORTHO
CameraORTHO::CameraORTHO(glm::vec3 const& forward, glm::vec3 const& up, CameraTarget& t,
                         Viewport& vp)
    : forward_(forward)
    , up_(up)
    , target_(t)
    , viewport_(vp)
{
}

glm::mat4
CameraORTHO::compute_projectionmatrix() const
{
  auto const& f = viewport_.frustum;
  return glm::ortho(f.left, f.right, f.bottom, f.top, f.near, f.far);
}

glm::mat4
CameraORTHO::compute_viewmatrix(glm::vec3 const& target) const
{
  return glm::lookAt(ZERO, target, Y_UNIT_VECTOR);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Camera
Camera::Camera(ScreenDimensions const& dimensions, Viewport&& vp, glm::vec3 const& forward,
               glm::vec3 const& up)
    : viewport_(MOVE(vp))
    , mode_(CameraMode::ThirdPerson)
    , arcball(forward, up, target_, viewport_)
    , fps(forward, up, target_, viewport_)
    , ortho(forward, up, target_, viewport_)
{
}

WorldObject&
Camera::get_target()
{
  return target_.get();
}

WorldObject const&
Camera::get_target() const
{
  return target_.get();
}

void
Camera::set_mode(CameraMode const m)
{
  mode_ = m;
}

void
Camera::next_mode()
{
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
  switch (mode()) {
    case CameraMode::FPS:
      arcball.rotate(d_theta, d_phi);
      break;
    case CameraMode::Ortho:
      //break;
    case CameraMode::ThirdPerson:
      arcball.rotate(d_theta, d_phi);
      break;
    case CameraMode::MAX:
      std::abort();
      break;
  }
  return *this;
}

void
Camera::set_target(WorldObject& target)
{
  target_.set(target);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Camera
glm::vec3
Camera::eye_forward() const
{
  switch (mode()) {
    case CameraMode::FPS:
      return fps.forward_;
    case CameraMode::Ortho:
      return ortho.forward_;
    case CameraMode::ThirdPerson:
      return arcball.forward_;
    case CameraMode::MAX:
      break;
  }
  std::abort();
  return ZERO;
}

glm::vec3
Camera::eye_up() const
{
  switch (mode()) {
    case CameraMode::FPS:
      return fps.up_;
    case CameraMode::Ortho:
      return ortho.up_;
    case CameraMode::ThirdPerson:
      return arcball.up_;
    case CameraMode::MAX:
      break;
  }
  std::abort();
  return ZERO;
}

glm::vec3
Camera::world_forward() const
{
  switch (mode()) {
    case CameraMode::FPS:
    case CameraMode::Ortho:
    case CameraMode::ThirdPerson:
      return glm::normalize(arcball.world_position() - arcball.target_position());
    case CameraMode::MAX:
      break;
  }
  std::abort();
  return ZERO;
}

glm::vec3
Camera::world_position() const
{
  switch (mode()) {
    case CameraMode::FPS:
    case CameraMode::Ortho:
    case CameraMode::ThirdPerson:
      return arcball.world_position();
    case CameraMode::MAX:
      break;
  }
  std::abort();
  return ZERO;
}

glm::mat4
Camera::compute_projectionmatrix() const
{
  switch (mode()) {
  case CameraMode::ThirdPerson:
    return arcball.compute_projectionmatrix();
  case CameraMode::FPS:
    return fps.compute_projectionmatrix();
  case CameraMode::Ortho:
    return ortho.compute_projectionmatrix();
  case CameraMode::MAX:
    break;
  }

  std::abort();
  return glm::mat4{}; // appease compiler
}

glm::mat4
Camera::compute_viewmatrix(glm::vec3 const& target_pos) const
{
  switch (mode()) {
    case CameraMode::Ortho:
      return ortho.compute_viewmatrix(target_pos);
    case CameraMode::FPS:
      return fps.compute_viewmatrix(world_forward());
    case CameraMode::ThirdPerson:
      return arcball.compute_viewmatrix(target_pos);
    case CameraMode::MAX:
      break;
  }
  std::abort();
  return glm::mat4{}; // appease compiler
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// free functions
Camera
Camera::make_default(ScreenDimensions const& dimensions)
{
  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const FORWARD = -Z_UNIT_VECTOR;
  auto constexpr UP  = Y_UNIT_VECTOR;

  auto constexpr FOV  = 110.0f;
  auto constexpr AR   = AspectRatio{4.0f, 3.0f};
  auto constexpr NEAR = 0.1f;
  auto constexpr FAR  = 10000.0f;
  auto vp = make_viewport(FOV, dimensions, AR, NEAR, FAR);

  Camera camera(dimensions, MOVE(vp), FORWARD, UP);

  SphericalCoordinates sc;
  sc.radius = 3.8f;
  sc.theta  = glm::radians(-0.229f);
  sc.phi    = glm::radians(38.2735f);
  camera.arcball.set_coordinates(sc);

  return camera;
}

} // namespace boomhs

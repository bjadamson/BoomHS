#include <boomhs/camera.hpp>
#include <boomhs/screen_info.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/world_object.hpp>

#include <window/sdl_window.hpp>
#include <boomhs/clock.hpp>

#include <boomhs/math.hpp>
#include <extlibs/imgui.hpp>

using namespace boomhs;
using namespace boomhs::math;
using namespace boomhs::math::constants;
using namespace window;


// Without this multiplier, the camera would take years to rotate. This is because we normalize the
// rotation and scaling with the delta time each frame.
//
// This massive speed multiplier is needed to restore numbers fast enough for humans to enjoy.
double constexpr ZOOM_SPEED_MULTIPLIER = 1000.0;

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
// CameraTarget
void
CameraTarget::validate() const
{
  assert(nullptr != wo_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CameraArcball
CameraArcball::CameraArcball(glm::vec3 const& forward, glm::vec3 const& up, CameraTarget& t,
                             Viewport& vp, bool& flip_y)
    : forward_(forward)
    , up_(up)
    , target_(t)
    , viewport_(vp)
    , coordinates_(0.0f, 0.0f, 0.0f)
    , flip_y_(flip_y)
    , rotation_lock(false)
{
}

void
CameraArcball::zoom(float const amount, FrameTime const& ft)
{
  float constexpr MIN_RADIUS = 0.01f;
  float const delta          = (amount * ft.delta_millis() * ZOOM_SPEED_MULTIPLIER);
  float const new_radius     = coordinates_.radius + delta;

  coordinates_.radius = (new_radius >= MIN_RADIUS)
    ? new_radius
    : MIN_RADIUS;
}

void
CameraArcball::decrease_zoom(float const amount, FrameTime const& ft)
{
  zoom(-amount, ft);
}

void
CameraArcball::increase_zoom(float const amount, FrameTime const& ft)
{
  zoom(amount, ft);
}

CameraArcball&
CameraArcball::rotate(float const dx, float const dy, DeviceSensitivity const& sens,
    FrameTime const& ft)
{
  auto& theta = coordinates_.theta;
  theta       = (up_.y > 0.0f) ? (theta - dx) : (theta + dx);
  if (theta > TWO_PI) {
    theta -= TWO_PI;
  }
  else if (theta < -TWO_PI) {
    theta += TWO_PI;
  }

  auto&       phi     = coordinates_.phi;
  float const new_phi = flip_y_ ? (phi + dy) : (phi - dy);

  bool const in_region0     = new_phi > 0 && new_phi < (PI / 2.0f);
  bool const in_region1     = new_phi < -(PI / 2.0f);
  bool const top_hemisphere = in_region0 || in_region1;
  if (!rotation_lock || top_hemisphere) {
    phi = new_phi;
  }

  // Keep phi within -2PI to +2PI for easy 'up' comparison
  if (phi > TWO_PI) {
    phi -= TWO_PI;
  }
  else if (phi < -TWO_PI) {
    phi += TWO_PI;
  }

  // If phi in range (0, PI) or (-PI to -2PI), make 'up' be positive Y, otherwise make it negative Y
  if ((phi > 0 && phi < PI) || (phi < -PI && phi > -TWO_PI)) {
    up_ = Y_UNIT_VECTOR;
  }
  else {
    up_ = -Y_UNIT_VECTOR;
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
  return target_.get().transform().translation;
}

glm::mat4
CameraArcball::compute_projectionmatrix() const
{
  auto const ar  = viewport_.aspect_ratio.compute();
  auto const fov = viewport_.field_of_view;
  auto const& f  = viewport_.frustum;

  return glm::perspective(fov, ar, f.near, f.far);
}

glm::mat4
CameraArcball::compute_viewmatrix(glm::vec3 const& target_pos) const
{
  auto const& target =  target_.get().transform().translation;
  return glm::lookAt(target_pos, target, up_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CameraFPS
CameraFPS::CameraFPS(glm::vec3 const& forward, glm::vec3 const& up, CameraTarget& t, Viewport& vp)
    : forward_(forward)
    , up_(up)
    , target_(t)
    , viewport_(vp)
    , rotation_lock(true)
{
}

void
CameraFPS::update(int const xpos, int const ypos, ScreenDimensions const& dim,
                  SDLWindow& window)
{
}

CameraFPS&
CameraFPS::rotate_degrees(float const dx, float const dy, DeviceSensitivity const& sens, FrameTime const& ft)
{
  transform().rotate_degrees(dy, EulerAxis::X);
  transform().rotate_degrees(dx, EulerAxis::Y);
  return *this;
}

glm::vec3
CameraFPS::world_position() const
{
  return transform().translation;
}

glm::mat4
CameraFPS::compute_projectionmatrix() const
{
  auto const ar  = viewport_.aspect_ratio.compute();
  auto const fov = viewport_.field_of_view;
  auto const& f  = viewport_.frustum;
  return glm::perspective(fov, ar, f.near, f.far);
}

glm::mat4
CameraFPS::compute_viewmatrix(glm::vec3 const& eye_fwd) const
{
  auto const pos = transform().translation;

  return glm::lookAt(pos, pos + eye_fwd, up_);
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
Camera::Camera(Viewport&& vp, glm::vec3 const& forward,
               glm::vec3 const& up)
    : viewport_(MOVE(vp))
    , mode_(CameraMode::ThirdPerson)
    , arcball(forward, up, target_, viewport_, flip_y)
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
  if (CameraMode::MAX == m || CameraMode::FREE_FLOATING == m) {
    set_mode(static_cast<CameraMode>(0));
  }
  else {
    assert(m < CameraMode::FREE_FLOATING);
    set_mode(m);
  }
}

void
Camera::toggle_rotation_lock()
{
  switch (mode()) {
    case CameraMode::FPS:
      fps.rotation_lock ^= true;
      break;
    case CameraMode::Ortho:
      break;
    case CameraMode::ThirdPerson:
      arcball.rotation_lock ^= true;
      break;
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      std::abort();
      break;
  }
}

Camera&
Camera::rotate(float dx, float dy, DeviceSensitivity const& sens, FrameTime const& ft)
{
  dx *= ft.delta_millis() * sens.x;
  dy *= ft.delta_millis() * sens.y;

  switch (mode()) {
    case CameraMode::FPS:
      fps.rotate_degrees(glm::degrees(dx), glm::degrees(dy), sens, ft);
      break;
    case CameraMode::Ortho:
      //break;
    case CameraMode::ThirdPerson:
      arcball.rotate(dx, dy, sens, ft);
      break;
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      std::abort();
      break;
  }
  return *this;
}

void
Camera::set_target(WorldObject& wo)
{
  target_.set(wo);
}


glm::vec3
Camera::eye_forward() const
{
  switch (mode()) {
    case CameraMode::FPS:
      return glm::normalize(-Z_UNIT_VECTOR * fps.transform().rotation);
    case CameraMode::Ortho:
    case CameraMode::ThirdPerson:
      return glm::normalize(arcball.world_position() - arcball.target_position());
    case CameraMode::FREE_FLOATING:
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
      return fps.forward_;
    case CameraMode::Ortho:
      return ortho.forward_;
    case CameraMode::ThirdPerson:
      return arcball.forward_;
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      break;
  }
  std::abort();
  return ZERO;
}

glm::vec3
Camera::world_up() const
{
  switch (mode()) {
    case CameraMode::FPS:
      return fps.up_;
    case CameraMode::Ortho:
      return ortho.up_;
    case CameraMode::ThirdPerson:
      return arcball.up_;
    case CameraMode::FREE_FLOATING:
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
      return fps.world_position();
    case CameraMode::Ortho:
      return ZERO;
    case CameraMode::ThirdPerson:
      return arcball.world_position();
    case CameraMode::FREE_FLOATING:
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
  case CameraMode::FREE_FLOATING:
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
      return fps.compute_viewmatrix(eye_forward());
    case CameraMode::ThirdPerson:
      return arcball.compute_viewmatrix(target_pos);
    case CameraMode::FREE_FLOATING:
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

  auto constexpr FOV  = glm::radians(110.0f);
  auto constexpr AR   = AspectRatio{4.0f, 3.0f};
  auto constexpr NEAR = 0.1f;
  auto constexpr FAR  = 10000.0f;
  auto vp = make_viewport(FOV, dimensions, AR, NEAR, FAR);

  Camera camera(MOVE(vp), FORWARD, UP);

  SphericalCoordinates sc;
  sc.radius = 3.8f;
  sc.theta  = glm::radians(-0.229f);
  sc.phi    = glm::radians(38.2735f);
  camera.arcball.set_coordinates(sc);

  return camera;
}

} // namespace boomhs

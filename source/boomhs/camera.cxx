#include <boomhs/camera.hpp>
#include <boomhs/screen_info.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/world_object.hpp>

#include <window/sdl_window.hpp>
#include <boomhs/clock.hpp>

#include <boomhs/math.hpp>

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
                             Viewport& vp)
    : forward_(forward)
    , up_(up)
    , target_(t)
    , viewport_(vp)
    , coordinates_(0.0f, 0.0f, 0.0f)
{
  cs.rotation_lock = false;
  cs.sensitivity.x = 10.0f;
  cs.sensitivity.y = 10.0f;
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
CameraArcball::rotate_radians(float dx, float dy, FrameTime const& ft)
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
  float const new_phi = cs.flip_y ? (phi + dy) : (phi - dy);

  bool const in_region0     = new_phi > 0 && new_phi < (PI / 2.0f);
  bool const in_region1     = new_phi < -(PI / 2.0f);
  bool const top_hemisphere = in_region0 || in_region1;
  if (!cs.rotation_lock || top_hemisphere) {
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
    , xrot_(0)
    , yrot_(0)
    , target_(t)
    , viewport_(vp)
{
  cs.rotation_lock = true;

  cs.flip_x = true;
  cs.flip_y = true;

  cs.sensitivity.x = 10.0f;
  cs.sensitivity.y = 10.0f;
}

CameraFPS&
CameraFPS::rotate_radians(float dx, float dy, FrameTime const& ft)
{
  xrot_ += dx;
  {
    float const newy_rot = yrot_ + dy;
    if (std::abs(newy_rot) < glm::radians(65.0f)) {
      yrot_ = newy_rot;
    }
  }

  auto const xaxis = cs.flip_x
    ?  Y_UNIT_VECTOR
    : -Y_UNIT_VECTOR;

  auto const yaxis = cs.flip_y
    ?  X_UNIT_VECTOR
    : -X_UNIT_VECTOR;

  // combine existing rotation with new rotation.
  // Y-axis first
  glm::quat a   = glm::angleAxis(yrot_, yaxis) * glm::angleAxis(dy, yaxis);
  glm::quat b   = glm::angleAxis(xrot_, xaxis) * glm::angleAxis(dx, xaxis);
  transform().rotation = a * b;

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
    , zoom_(glm::vec2{0, 0})
    , position(-3, 50, 108)
    , lookat_position(-3, -180.0f, 48)
{
}

glm::mat4
CameraORTHO::compute_projectionmatrix() const
{
  auto const& f  = viewport_.frustum;
  auto const& ar = viewport_.aspect_ratio.compute();

  float const left  = 0.0f + zoom_.x;
  float const right = 128.0f - zoom_.x;

  float const top    = 0.0f + zoom_.y;
  float const bottom = 96.0f - zoom_.y;

  return glm::ortho(left, right, top, bottom, f.near, f.far);
}

glm::mat4
CameraORTHO::compute_viewmatrix(glm::vec3 const& target) const
{
  return glm::lookAt(position, lookat_position, up_);
}

void
CameraORTHO::grow_view(glm::vec2 const& amount)
{
  zoom_ += amount;
}

void
CameraORTHO::shink_view(glm::vec2 const& amount)
{
  zoom_ -= amount;
}

void
CameraORTHO::scroll(glm::vec2 const& sv)
{
  position        += glm::vec3{sv.x, 0, sv.y};
  lookat_position += glm::vec3{sv.x, 0, sv.y};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Camera
Camera::Camera(Viewport&& vp, glm::vec3 const& forward, glm::vec3 const& up)
    : viewport_(MOVE(vp))
    , arcball(forward, up, target_, viewport_)
    , fps(forward, up, target_, viewport_)
    , ortho(forward, up, target_, viewport_)
{
  set_mode(CameraMode::ThirdPerson);
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

  bool const fps_mode   = mode_ == CameraMode::FPS;
  auto const mouse_mode = fps_mode
      ? SDL_TRUE
      : SDL_FALSE;
  assert(0 == SDL_SetRelativeMouseMode(mouse_mode));
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
      fps.cs.rotation_lock ^= true;
      break;
    case CameraMode::Ortho:
      break;
    case CameraMode::ThirdPerson:
      arcball.cs.rotation_lock ^= true;
      break;
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      std::abort();
      break;
  }
}

Camera&
Camera::rotate_radians(float dx, float dy, FrameTime const& ft)
{
  dx *= ft.delta_millis();
  dy *= ft.delta_millis();

  switch (mode()) {
    case CameraMode::FPS:
      fps.rotate_radians(dx, dy, ft);
      break;
    case CameraMode::Ortho:
      break;
    case CameraMode::ThirdPerson:
      arcball.rotate_radians(dx, dy, ft);
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
      return ortho.forward_;
      break;
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
      return ortho.position;
    case CameraMode::ThirdPerson:
      return arcball.world_position();
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      break;
  }
  std::abort();
  return ZERO;
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
  auto constexpr NEAR = 0.001f;
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

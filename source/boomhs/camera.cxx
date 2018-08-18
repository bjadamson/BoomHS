#include <boomhs/camera.hpp>
#include <boomhs/screen_size.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <boomhs/math.hpp>
#include <extlibs/imgui.hpp>

using namespace boomhs;
using namespace boomhs::math;
using namespace boomhs::math::constants;
using namespace opengl;
using namespace window;


// Without this multiplier, the camera would take years to rotate. This is because we normalize the
// rotation and scaling with the delta time each frame.
//
// This massive speed multiplier is needed to restore numbers fast enough for humans to enjoy.
double constexpr ROTATION_SPEED_MULTIPLIER = 10000.0;

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
    , sensitivity(0.002, 0.002)
    , rotation_lock(false)
{
}

void
CameraArcball::zoom(float const amount, FrameTime const& ft)
{
  float constexpr MIN_RADIUS = 0.01f;
  float const delta          = (amount * ft.delta_millis() * ROTATION_SPEED_MULTIPLIER);
  float const new_radius     = coordinates_.radius + delta;

  coordinates_.radius = (new_radius >= MIN_RADIUS)
    ? new_radius
    : MIN_RADIUS;
}

void
CameraArcball::decrease_zoom(float const amount, window::FrameTime const& ft)
{
  zoom(-amount, ft);
}

void
CameraArcball::increase_zoom(float const amount, window::FrameTime const& ft)
{
  zoom(amount, ft);
}

CameraArcball&
CameraArcball::rotate(float const d_theta, float const d_phi)
{
  float const dx = d_theta * sensitivity.x;
  float const dy = d_phi   * sensitivity.y;

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
    , sensitivity(0.02, 0.02)
    , rotation_lock(true)
{
}

// theta: the angle (in radians) that v rotates around k
// v: a vector in 3D space
// k: a unit vector describing the axis of rotation
glm::vec3 ROTATE(float const theta, const glm::vec3& v, const glm::vec3& k)
{
  std::cout << "Rotating " << glm::to_string(v) << " "
            << theta << " radians around "
            << glm::to_string(k) << "..." << std::endl;

  float cos_theta = cos(theta);
  float sin_theta = sin(theta);

  glm::vec3 rotated = (v * cos_theta) + (glm::cross(k, v) * sin_theta) + (k * glm::dot(k, v)) * (1 - cos_theta);

  std::cout << "Rotated: " << glm::to_string(rotated) << std::endl;
  return rotated;
}

void
CameraFPS::update(int const xpos, int const ypos, ScreenDimensions const& dim,
                  SDLWindow& window)
{
#define CAMERA_ANGULAR_SPEED_DEG 1.0f
  auto const mouseAxisX = 1, mouseAxisY = -1;

  int const width  = dim.width() / 2;
  int const height = dim.height() / 2;
  std::cerr << "W: '" << width << "' h: '" << height << "'\n";
  std::cerr << "xpos: '" << xpos << "' ypos: '" << ypos << "'\n";
  int const mouseDeltaX = mouseAxisX * (xpos - width);
  int const mouseDeltaY = -mouseAxisY * (ypos- height);  // mouse y-offsets are upside-down!

  // HACK:  reset the cursor pos.:
  //app.SetCursorPosition(app.GetWidth()/2, app.GetHeight()/2);
  SDL_WarpMouseInWindow(window.raw(), width, height);

  float lookRightRads = glm::radians(mouseDeltaX * CAMERA_ANGULAR_SPEED_DEG);
  float lookUpRads    = glm::radians(mouseDeltaY * CAMERA_ANGULAR_SPEED_DEG);

  // Limit the aim vector in such a way that the 'up' vector never drops below the horizon:
#define MIN_UPWARDS_TILT_DEG 1.0f
  static const float zenithMinDeclination = glm::radians(MIN_UPWARDS_TILT_DEG);
  static const float zenithMaxDeclination = glm::radians(180.0f - MIN_UPWARDS_TILT_DEG);

  const float currentDeclination = std::acosf(forward_.y);  ///< declination from vertical y-axis
  const float requestedDeclination = currentDeclination - lookUpRads;

  // Clamp the up/down rotation to at most the min/max zenith:
  if(requestedDeclination < zenithMinDeclination) {
    lookUpRads = currentDeclination - zenithMinDeclination;
  }
  else if(requestedDeclination > zenithMaxDeclination) {
    lookUpRads = currentDeclination - zenithMaxDeclination;
  }

  // Rotate both the "aim" vector and the "up" vector ccw by 
  // lookUpRads radians within their common plane -- which should 
  // also contain the y-axis:  (i.e. no diagonal tilt allowed!)
  auto const right = glm::normalize(glm::cross(forward_, up_));
  forward_ = ROTATE(lookUpRads, forward_, right);
  up_      = ROTATE(lookUpRads, up_, right);

#define ASSERT_ORTHONORMAL(A, B, C) \
  assert(float_compare(glm::dot(A, B), 0)); \
  assert(float_compare(glm::dot(A, C), 0)); \
  assert(float_compare(glm::dot(B, C), 0));

  ASSERT_ORTHONORMAL(forward_, up_, right);

  // Rotate both the "aim" and the "up" vector ccw about the vertical y-axis:
  // (again, this keeps the y-axis in their common plane, and disallows diagonal tilt)
  forward_ = ROTATE(-lookRightRads, forward_, constants::Y_UNIT_VECTOR);
  up_      = ROTATE(-lookRightRads, up_, constants::Y_UNIT_VECTOR);
}

CameraFPS&
CameraFPS::rotate_degrees(float dx, float dy)
{
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
CameraFPS::compute_viewmatrix(glm::vec3 const& world_forward) const
{
  auto const pos = transform().translation;

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
Camera::rotate(float dx, float dy, FrameTime const& ft)
{
  dx *= ft.delta_millis() * ROTATION_SPEED_MULTIPLIER;
  dy *= ft.delta_millis() * ROTATION_SPEED_MULTIPLIER;

  switch (mode()) {
    case CameraMode::FPS:
      fps.rotate_degrees(glm::degrees(dx), glm::degrees(dy));
      break;
    case CameraMode::Ortho:
      //break;
    case CameraMode::ThirdPerson:
      arcball.rotate(dx, dy);
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
Camera::eye_up() const
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
Camera::world_forward() const
{
  switch (mode()) {
    case CameraMode::FPS:
      return glm::normalize(Z_UNIT_VECTOR * fps.transform().rotation_quat());
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
      return fps.compute_viewmatrix(world_forward());
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

#include <boomhs/camera.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/viewport.hpp>
#include <boomhs/world_object.hpp>

#include <gl_sdl/sdl_window.hpp>

#include <boomhs/math.hpp>

using namespace boomhs;
using namespace boomhs::math;
using namespace boomhs::math::constants;
using namespace gl_sdl;

// Without this multiplier, the camera would take years to rotate. This is because we normalize the
// rotation and scaling with the delta time each frame.
//
// This massive speed multiplier is needed to restore numbers fast enough for humans to enjoy.
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
CameraArcball::CameraArcball(glm::vec3 const& up)
    : coordinates_(0.0f, 0.0f, 0.0f)
    , up_inverted_(false)
    , world_up_(up)
{
  cs.rotation_lock = false;
  cs.sensitivity.x = 10.0f;
  cs.sensitivity.y = 10.0f;
}

void
CameraArcball::zoom(float const amount, FrameTime const& ft)
{
  float constexpr MIN_RADIUS            = 0.01f;
  float constexpr ZOOM_SPEED_MULTIPLIER = 1000.0;

  float const delta      = (amount * ft.delta_millis() * ZOOM_SPEED_MULTIPLIER);
  float const new_radius = coordinates_.radius + delta;

  coordinates_.radius = (new_radius >= MIN_RADIUS) ? new_radius : MIN_RADIUS;
}

void
CameraArcball::zoom_out(float const amount, FrameTime const& ft)
{
  zoom(-amount, ft);
}

void
CameraArcball::zoom_in(float const amount, FrameTime const& ft)
{
  zoom(amount, ft);
}

CameraArcball&
CameraArcball::rotate_radians(float dx, float dy, FrameTime const& ft)
{
  auto& theta = coordinates_.theta;
  theta       = (eye_up().y > 0.0f) ? (theta - dx) : (theta + dx);
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
    up_inverted_ = false;
  }
  else {
    up_inverted_ = true;
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
  return target().get().transform().translation;
}

glm::mat4
CameraArcball::calc_pm(ViewSettings const& vp, Frustum const& f) const
{
  auto const ar  = vp.aspect_ratio.compute();
  auto const fov = vp.field_of_view;

  return glm::perspective(fov, ar, f.near, f.far);
}

glm::mat4
CameraArcball::calc_vm(glm::vec3 const& pos) const
{
  auto const& target_pos = target().get().transform().translation;
  return glm::lookAt(pos, target_pos, eye_up());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CameraFPS
CameraFPS::CameraFPS(WorldOrientation const& wo)
    : xrot_(0)
    , yrot_(0)
    , orientation_(wo)
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

  auto const xaxis = cs.flip_x ? orientation_.up : -orientation_.up;

  auto const yaxis = cs.flip_y ? orientation_.right() : -orientation_.right();

  // combine existing rotation with new rotation.
  // Y-axis first
  glm::quat a          = glm::angleAxis(yrot_, yaxis) * glm::angleAxis(dy, yaxis);
  glm::quat b          = glm::angleAxis(xrot_, xaxis) * glm::angleAxis(dx, xaxis);
  transform().rotation = a * b;

  return *this;
}

glm::vec3
CameraFPS::world_position() const
{
  return transform().translation;
}

glm::mat4
CameraFPS::calc_pm(ViewSettings const& vp, Frustum const& f) const
{
  auto const ar  = vp.aspect_ratio.compute();
  auto const fov = vp.field_of_view;
  return glm::perspective(fov, ar, f.near, f.far);
}

glm::mat4
CameraFPS::calc_vm(glm::vec3 const& eye_fwd) const
{
  auto const pos = transform().translation;

  return glm::lookAt(pos, pos + eye_fwd, eye_up());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CameraORTHO
CameraORTHO::CameraORTHO(WorldOrientation const& wo, glm::vec3 const& pos)
    : orientation_(wo)
    , zoom_(VEC2{0})
    , position(pos)
{
}

glm::mat4
CameraORTHO::calc_pm(AspectRatio const& ar, Frustum const& f, ScreenSize const& ss) const
{
  auto const& zoom_amount = zoom();
  return compute_pm(ar, f, ss, position, zoom_amount);
}

glm::mat4
CameraORTHO::compute_pm(AspectRatio const& AR, Frustum const& f, ScreenSize const& ss,
    CameraPosition const& position, glm::ivec2 const& zoom)
{
  auto const& view_width  = ss.width;
  auto const& view_height = ss.height;

  // clang-format off
  float const left  = 0.0f + f.left      + zoom.x - position.x;
  float const right = left + view_width  - zoom.x - position.x;

  float const top    = 0.0f + f.top      + zoom.y + position.z;
  float const bottom = top + view_height - zoom.y + position.z;
  // clang-format on

  return glm::ortho(left, right, bottom, top, f.near, f.far);
}

glm::mat4
CameraORTHO::calc_vm() const
{
  auto const pos     = position;
  auto const  target = pos + eye_forward();
  auto const& up     = eye_up();

  return compute_vm(pos, target, up);
}

glm::mat4
CameraORTHO::compute_vm(CameraPosition const& pos, CameraCenter const& center, CameraUp const& up)
{
  // Calculate the view model matrix.
  auto vm = glm::lookAtRH(pos, center, up);

  // Flip the "right" vector computed by lookAt() so the X-Axis points "right" onto the screen.
  //
  // See implementation of glm::lookAtRH() for more details.
  auto& sx = vm[0][0];
  auto& sy = vm[1][0];
  auto& sz = vm[2][0];
  math::negate(sx, sy, sz);

  return vm;
}

void
CameraORTHO::zoom(glm::vec2 const& amount, FrameTime const& ft)
{
  auto const delta = amount * ft.delta_millis();
  zoom_ += delta;
}

void
CameraORTHO::zoom_out(glm::vec2 const& amount, FrameTime const& ft)
{
  zoom(amount, ft);
}

void
CameraORTHO::zoom_in(glm::vec2 const& amount, FrameTime const& ft)
{
  zoom(-amount, ft);
}

void
CameraORTHO::scroll(glm::vec2 const& sv)
{
  position += glm::vec3{sv.x, 0, sv.y};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Camera
Camera::Camera(CameraMode const cmode, ViewSettings&& vp, CameraArcball&& acam, CameraFPS&& fcam,
               CameraORTHO&& ocam)
    : view_settings_(MOVE(vp))
    , mode_(cmode)
    , arcball(MOVE(acam))
    , fps(MOVE(fcam))
    , ortho(MOVE(ocam))
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
  auto const       cast = [](auto const v) { return static_cast<int>(v); };
  CameraMode const m    = static_cast<CameraMode>(cast(mode()) + cast(1));
  if (CameraMode::MAX == m || CameraMode::Fullscreen_2DUI == m) {
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
  case CameraMode::Fullscreen_2DUI:
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
  case CameraMode::Fullscreen_2DUI:
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

  fps.target_ = &target_;
  arcball.target_ = &target_;
}

glm::vec3
Camera::eye_forward() const
{
  switch (mode()) {
  case CameraMode::FPS:
    return fps.eye_forward();
  case CameraMode::Ortho:
  case CameraMode::Fullscreen_2DUI:
    return ortho.eye_forward();
  case CameraMode::ThirdPerson:
    return arcball.eye_forward();
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
  return world_up() * get_target().orientation();
}

glm::vec3
Camera::world_forward() const
{
  switch (mode()) {
  case CameraMode::FPS:
    return fps.eye_forward();

  case CameraMode::Ortho:
  case CameraMode::Fullscreen_2DUI:
    return ortho.eye_forward();

  case CameraMode::ThirdPerson:
    return arcball.eye_forward();

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
    return fps.eye_up();

  case CameraMode::Ortho:
  case CameraMode::Fullscreen_2DUI:
    return ortho.eye_up();

  case CameraMode::ThirdPerson:
    return arcball.eye_up();

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
  case CameraMode::Fullscreen_2DUI:
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

glm::mat4
Camera::calc_pm(ViewSettings const& vs, Frustum const& frustum, ScreenSize const& ss) const
{
  switch (mode()) {
    case CameraMode::FPS:
      return fps.calc_pm(vs, frustum);
    case CameraMode::ThirdPerson:
      return arcball.calc_pm(vs, frustum);
    case CameraMode::Ortho:
    case CameraMode::Fullscreen_2DUI:
      return ortho.calc_pm(vs.aspect_ratio, frustum, ss);
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      break;
  }
  std::abort();
  return glm::mat4{};
}

glm::mat4
Camera::calc_vm(glm::vec3 const& pos) const
{
  switch (mode()) {
    case CameraMode::FPS:
      return fps.calc_vm(pos);
    case CameraMode::ThirdPerson:
      return arcball.calc_vm(pos);
    case CameraMode::Ortho:
    case CameraMode::Fullscreen_2DUI:
      return ortho.calc_vm();
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      break;
  }
  std::abort();
  return glm::mat4{};
}

#define ZOOM_INOUT_IMPL(METHOD)                                                                    \
  switch (mode()) {                                                                                \
    case CameraMode::FPS:                                                                          \
    case CameraMode::Fullscreen_2DUI:                                                              \
      break;                                                                                       \
                                                                                                   \
    case CameraMode::Ortho:                                                                        \
      ortho.METHOD(glm::vec2{v}, ft);                                                              \
      break;                                                                                       \
                                                                                                   \
    case CameraMode::ThirdPerson:                                                                  \
      arcball.METHOD(v, ft);                                                                       \
      break;                                                                                       \
    case CameraMode::FREE_FLOATING:                                                                \
    case CameraMode::MAX:                                                                          \
      std::abort();                                                                                \
  }

void
Camera::zoom_out(float const v, FrameTime const& ft)
{
  ZOOM_INOUT_IMPL(zoom_out);
}

void
Camera::zoom_in(float const v, FrameTime const& ft)
{
  ZOOM_INOUT_IMPL(zoom_in);
}

#undef ZOOM_INOUT_IMPL

///////////////////////////////////////////////////////////////////////////////////////////////////
// free functions
Camera
Camera::make_default(CameraMode const mode, WorldOrientation const& pers_wo, WorldOrientation const& ortho_wo)
{
  auto constexpr FOV = glm::radians(110.0f);
  auto constexpr AR  = AspectRatio{4.0f, 3.0f};
  ViewSettings vp{AR, FOV};


  CameraArcball arcball(pers_wo.up);
  CameraFPS fps(pers_wo);

  // It's weird using this direction unit vector as a position, but this is intentionaly.
  auto constexpr ORTHO_CAMERA_INIT_POSITION = constants::Y_UNIT_VECTOR;
  CameraORTHO ortho(ortho_wo, ORTHO_CAMERA_INIT_POSITION);

  Camera camera{mode, MOVE(vp), MOVE(arcball), MOVE(fps), MOVE(ortho)};

  SphericalCoordinates sc;
  sc.radius = 3.8f;
  sc.theta  = glm::radians(-0.229f);
  sc.phi    = glm::radians(38.2735f);
  camera.arcball.set_coordinates(sc);

  return camera;
}

} // namespace boomhs

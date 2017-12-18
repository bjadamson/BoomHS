#include <opengl/camera.hpp>
#include <boomhs/state.hpp>

namespace opengl
{

glm::vec3
to_cartesian(float const radius, float const theta, float const phi)
{
  float const x = radius * sinf(phi) * sinf(theta);
  float const y = radius * cosf(phi);
  float const z = radius * sinf(phi) * cosf(theta);

  return glm::vec3{x, y, z};
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FpsCamera
glm::vec3
FpsCamera::direction_facing_degrees() const
{
  return glm::degrees(glm::eulerAngles(this->orientation()));
}

glm::mat4
FpsCamera::view(Model const& target_model) const
{
  glm::vec3 const pos = -target_model.translation;
  auto const translation = glm::translate(glm::mat4(), pos);

  glm::mat4 const orientation = glm::mat4_cast(this->orientation_);
  return orientation * translation;
}

std::string
FpsCamera::display() const
{
  auto const f = direction_facing_degrees();
  auto const x = std::to_string(f.x);
  auto const y = std::to_string(f.y);
  auto const z = std::to_string(f.z);
  return fmt::sprintf("x: '%s', y: '%s', z: '%s'", x.c_str(), y.c_str(), z.c_str());
}

FpsCamera&
FpsCamera::rotate(stlw::Logger &logger, boomhs::UiState &uistate, window::mouse_data const& mdata)
{
  auto const& current = mdata.current;
  auto const& mouse_sens = mdata.sensitivity;

  glm::vec2 const delta = glm::vec2{current.xrel, current.yrel};

  auto const yaw = mouse_sens.x * delta.x;
  auto const pitch = mouse_sens.y * delta.y;
  auto const roll = this->roll_;

  bool const moving_down = current.yrel >= 0;
  bool const moving_up = current.yrel <= 0;

  auto const new_pitch = glm::degrees(this->pitch_ + pitch);
  if (mdata.pitch_lock) {
    if(new_pitch > 0.0f && moving_down) {
      LOG_ERROR("DOWN LOCK");
      return *this;
    }
    if(new_pitch < -45.0f && moving_up) {
      LOG_ERROR("UP LOCK");
      return *this;
    }
  }

  this->yaw_ += yaw;
  this->pitch_ += pitch;

  auto const quat = glm::quat{glm::vec3{pitch, yaw, roll}};
  this->orientation_ = glm::normalize(quat * this->orientation_);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// OrbitCamera
glm::mat4
OrbitCamera::view(Model const& target_model) const
{
  auto const& target = target_model.translation;
  auto const position_xyz = target + to_cartesian(radius_, theta_, phi_);

  return glm::lookAt(position_xyz, target, this->up());
}

std::string
OrbitCamera::display() const
{
  auto const r = std::to_string(radius_);
  auto const t = std::to_string(theta_);
  auto const p = std::to_string(phi_);
  return fmt::sprintf("r: '%s', t: '%s', p: '%s'", r.c_str(), t.c_str(), p.c_str());
}

OrbitCamera&
OrbitCamera::rotate(stlw::Logger &logger, boomhs::UiState &uistate, window::mouse_data const& mdata)
{
  auto const& current = mdata.current;
  glm::vec2 const delta = glm::vec2{current.xrel, current.yrel};

  auto const& mouse_sens = mdata.sensitivity;
  float const d_theta = mouse_sens.x * delta.x;
  float const d_phi = mouse_sens.y * delta.y;

  if (up_.y > 0.0f) {
    theta_ -= d_theta;
  } else {
    theta_ += d_theta;
  }

  if (uistate.flip_y) {
    phi_ += d_phi;
  } else {
    phi_ -= d_phi;
  }

  // Keep phi within -2PI to +2PI for easy 'up' comparison
  float constexpr PI = glm::pi<float>();
  float constexpr TWO_PI = PI * 2.0f;
  if (phi_ > TWO_PI) {
    phi_ -= TWO_PI;
  } else if (phi_ < -TWO_PI) {
    phi_ += TWO_PI;
  }

  // If phi is between 0 to PI or -PI to -2PI, make 'up' be positive Y, other wise make it negative Y
  if ((phi_ > 0 && phi_ < PI) || (phi_ < -PI && phi_ > -TWO_PI)) {
    up_ = Y_UNIT_VECTOR;
  } else {
    up_ = -Y_UNIT_VECTOR;
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Camera
Camera::Camera(Projection const& proj, skybox &&sb, glm::vec3 const& front, glm::vec3 const& up,
    CameraMode const cmode, Model &target)
  : projection_(proj)
  , skybox_(MOVE(sb))
  , fps_(front, up)
  , orbit_(front, up)
  , active_mode_(cmode)
  , target_(target)
{
  this->skybox_.model.translation = front;
}

std::string
Camera::display() const
{
  return active_mode_ == opengl::FPS ? fps_.display() : orbit_.display();
}

std::string
Camera::follow_target_display() const
{
  return fmt::sprintf("follow target: '%s'", glm::to_string(target_.translation));
}

Camera&
Camera::rotate(stlw::Logger &logger, boomhs::UiState &uistate, window::mouse_data const& mdata)
{
  if (active_mode_ == FPS) {
    fps_.rotate(logger, uistate, mdata);
  } else {
    orbit_.rotate(logger, uistate, mdata);
  }
  return *this;
}


} // ns opengl

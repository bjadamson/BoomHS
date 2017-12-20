#include <opengl/camera.hpp>
#include <boomhs/state.hpp>
#include <limits>

namespace {

//glm::vec3
//direction_facing_degrees(glm::quat const& orientation)
//{
  //return glm::degrees(glm::eulerAngles(orientation));
//}

} // ns anonymous

namespace opengl
{

SphericalCoordinates
to_spherical(glm::vec3 cartesian)
{
  static constexpr float EPSILONF = std::numeric_limits<float>::epsilon();

  if (cartesian.x == 0) {
    cartesian.x = EPSILONF;
  }
  float const radius = sqrt((cartesian.x * cartesian.x)
                  + (cartesian.y * cartesian.y)
                  + (cartesian.z * cartesian.z));
  float theta = acos(cartesian.z / radius);
  //float theta = atan(cartesian.z / cartesian.x);
  if (cartesian.x < 0) {
    float constexpr PI = glm::pi<float>();
    theta += PI;
  }
  float const phi = atan(cartesian.y / cartesian.x);

  return SphericalCoordinates{radius, theta, phi};
}

glm::vec3
to_cartesian(SphericalCoordinates const& coords)
{
  float const radius = coords.radius;
  float const theta = coords.theta;
  float const phi = coords.phi;

  float const sin_phi = sinf(phi);

  float const x = radius * sin_phi * sinf(theta);
  float const y = radius * cosf(phi);
  float const z = radius * sin_phi * cosf(theta);

  return glm::vec3{x, y, z};
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// OrbitCamera
glm::mat4
OrbitCamera::view(Model const& target_model) const
{
  auto const& target = target_model.translation;
  auto const position_xyz = target + to_cartesian(coordinates_);

  return glm::lookAt(position_xyz, target, this->up());
}

std::string
OrbitCamera::display() const
{
  auto const r = std::to_string(coordinates_.radius);
  auto const t = std::to_string(coordinates_.theta);
  auto const p = std::to_string(coordinates_.phi);

  auto const cart = to_cartesian(coordinates_);
  auto const x = std::to_string(cart.x);
  auto const y = std::to_string(cart.y);
  auto const z = std::to_string(cart.z);
  return fmt::sprintf("camera info:\nx: '%s', y: '%s', z: '%s'\n", x.c_str(), y.c_str(), z.c_str())
    + " " + fmt::sprintf("r: '%s', t: '%s', p: '%s'", r.c_str(), t.c_str(), p.c_str());
}

OrbitCamera&
OrbitCamera::zoom(float const distance)
{
  float const new_radius = coordinates_.radius - distance;
  if (new_radius > 0.0f) {
    coordinates_.radius -= distance;
  } else {
    coordinates_.radius = 0.0001f;
  }
  return *this;

  // Don't let the radius go negative
  // If it does, re-project our target down the look vector
  //if (m_radius <= 0.0f) {
    //coordinates_.radius = 30.0f;
    //auto const look = glm::normalize(m_target - to_cartesian());
    //target_ = DirectX::XMVectorAdd(m_target, DirectX::XMVectorScale(look, 30.0f));
  //}
}

OrbitCamera&
OrbitCamera::rotate(stlw::Logger &logger, boomhs::UiState &uistate, window::mouse_data const& mdata)
{
  auto const& current = mdata.current;
  glm::vec2 const delta = glm::vec2{current.xrel, current.yrel};

  auto const& mouse_sens = mdata.sensitivity;
  float const d_theta = mouse_sens.x * delta.x;
  float const d_phi = mouse_sens.y * delta.y;

  {
    auto const& theta = coordinates_.theta;
    coordinates_.theta = (up_.y > 0.0f) ? (theta - d_theta) : (theta + d_theta);
  }

  float constexpr PI = glm::pi<float>();
  float constexpr TWO_PI = PI * 2.0f;

  auto &phi = coordinates_.phi;
  float const new_phi = uistate.flip_y ? (phi + d_phi) : (phi - d_phi);
  bool const top_hemisphere = (new_phi > 0 && new_phi < (PI/2.0f)) || (new_phi < -(PI/2.0f) && new_phi > -TWO_PI);
  if (top_hemisphere) {
    phi = new_phi;
  }

  // Keep phi within -2PI to +2PI for easy 'up' comparison
  if (phi > TWO_PI) {
    phi -= TWO_PI;
  } else if (phi < -TWO_PI) {
    phi += TWO_PI;
  }

  // If phi is between 0 to PI or -PI to -2PI, make 'up' be positive Y, other wise make it negative Y
  if ((phi > 0 && phi < PI) || (phi < -PI && phi > -TWO_PI)) {
    up_ = Y_UNIT_VECTOR;
  } else {
    up_ = -Y_UNIT_VECTOR;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Camera
Camera::Camera(Projection const& proj, skybox &&sb, glm::vec3 const& forward, glm::vec3 const& up,
    Model &target)
  : projection_(proj)
  , skybox_(MOVE(sb))
  , orbit_(up)
  , target_(target)
{
  this->skybox_.model.translation = forward;
}

std::string
Camera::display() const
{
  return orbit_.display();
}

std::string
Camera::follow_target_display() const
{
  return fmt::sprintf("follow target\nxyz: '%s'\nrot rtp: '%s'",
      glm::to_string(target_.translation),
      glm::to_string(target_.rotation));
}

Camera&
Camera::rotate(stlw::Logger &logger, boomhs::UiState &uistate, window::mouse_data const& mdata)
{
  orbit_.rotate(logger, uistate, mdata);
  return *this;
}

} // ns opengl

/*
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
*/

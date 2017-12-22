#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/ext.hpp>
#include <boomhs/types.hpp>
#include <stlw/format.hpp>
#include <iostream>

namespace boomhs
{

inline glm::quat
rotation_between_vectors(glm::vec3 start, glm::vec3 dest)
{
  start = glm::normalize(start);
  dest = glm::normalize(dest);

  float const cos_theta = glm::dot(start, dest);
  glm::vec3 rotation_axis;

  if (cos_theta < -1 + 0.001f){
    // special case when vectors in opposite directions:
    // there is no "ideal" rotation axis
    // So guess one; any will do as long as it's perpendicular to start
    rotation_axis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
    if (glm::length2(rotation_axis) < 0.01 ) // bad luck, they were parallel, try again!
      rotation_axis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);

    rotation_axis = glm::normalize(rotation_axis);
    return glm::angleAxis(glm::radians(180.0f), rotation_axis);
  }

  rotation_axis = glm::cross(start, dest);

  float const s = sqrt((1 + cos_theta) * 2);
  float const invs = 1 / s;

  return glm::quat{
    s * 0.5f,
    rotation_axis.x * invs,
    rotation_axis.y * invs,
    rotation_axis.z * invs
  };
}

inline glm::quat
rotate_towards(glm::quat q1, glm::quat q2, float const max_angle)
{
  if( max_angle < 0.001f ){
    // No rotation allowed. Prevent dividing by 0 later.
    return q1;
  }

  float cosTheta = glm::dot(q1, q2);

  // q1 and q2 are already equal.
  // Force q2 just to be sure
  if(cosTheta > 0.9999f){
    return q2;
  }

  // Avoid taking the long path around the sphere
  if (cosTheta < 0){
      q1 = q1*-1.0f;
      cosTheta *= -1.0f;
  }

  float angle = acos(cosTheta);

  // If there is only a 2&deg; difference, and we are allowed 5&deg;,
  // then we arrived.
  if (angle < max_angle){
    return q2;
  }

  float const fT = max_angle / angle;
  angle = max_angle;

  glm::quat const res = (sin((1.0f - fT) * angle) * q1 + sin(fT * angle) * q2) / sin(angle);
  return glm::normalize(res);
}

class Player
{
  boomhs::Transform &transform_;
  boomhs::Transform &arrow_;
  glm::vec3 forward_, up_;

  auto&
  move(float const s, glm::vec3 const& dir)
  {
    transform_.translation += (s * dir);
    arrow_.translation += (s * dir);
    return *this;
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(Player);
  explicit Player(boomhs::Transform &m, boomhs::Transform &a, glm::vec3 const& forward, glm::vec3 const& up)
    : transform_(m)
    , arrow_(a)
    , forward_(forward)
    , up_(up)
  {
  }

  glm::vec3
  right_vector() const
  {
    auto const cross = glm::cross(forward_vector(), up_vector());
    return glm::normalize(cross);
  }

  auto back_vector() const { return -forward_vector(); }
  auto left_vector() const { return -right_vector(); }
  auto down_vector() const { return -up_vector(); }

  std::string
  display() const
  {
    auto const& pos = position();
    return fmt::sprintf(
        "pos: '%s'\nforward_: '%s'\nforward_vector: '%s'\nright_vector: '%s'\nup_vector: '%s'\nquat: '%s'\n",
        glm::to_string(pos),
        glm::to_string(forward_),
        glm::to_string(forward_vector()),
        glm::to_string(right_vector()),
        glm::to_string(up_vector()),
        glm::to_string(orientation())
        );
  }

  glm::vec3
  forward_vector() const
  {
    return forward_ * transform_.rotation;
  }

  glm::vec3
  up_vector() const
  {
    return up_ * transform_.rotation;
  }

  glm::quat const&
  orientation() const
  {
    return transform_.rotation;
  }

  glm::vec3 const&
  position() const
  {
    return transform_.translation;
  }

  auto& move_forward(float const s)
  {
    return move(s, forward_vector());
  }

  auto& move_backward(float const s)
  {
    return move(s, back_vector());
  }

  auto& move_left(float const s)
  {
    return move(s, left_vector());
  }

  auto& move_right(float const s)
  {
    return move(s, right_vector());
  }

  auto& move_up(float const s)
  {
    return move(s, up_vector());
  }

  auto& move_down(float const s)
  {
    return move(s, down_vector());
  }

  void
  rotate(float const angle, window::mouse_data const& mdata)
  {
    auto const& current = mdata.current;
    glm::vec2 const delta = glm::vec2{current.xrel, current.yrel};

    auto const& mouse_sens = mdata.sensitivity;
    auto constexpr yaw = 0.0f;
    auto const pitch = angle * mouse_sens.x * delta.x * 0.1f;
    auto constexpr roll = 0.0f;
    auto const new_rotation = glm::quat{glm::vec3{yaw, pitch, roll}};

    //bool const left = mdata.current.xrel < 0;
    //float const a = left ? 5.0f : -5.0f;
    //glm::quat const new_rotation = glm::angleAxis(glm::radians(a), glm::vec3{0.0, 1.0f, 0.0f});
    transform_.rotation = new_rotation * transform_.rotation;
    arrow_.rotation = new_rotation * arrow_.rotation;
  }

  void
  match_camera_rotation(Camera const& camera)
  {
    //glm::vec3 cam_euler = glm::eulerAngles(camera.orientation());
    //glm::vec3 player_euler = glm::eulerAngles(this->orientation());

    //auto const player_initial = this->orientation();
    //glm::quat const player_initial_inverse = glm::inverse(player_initial);
    //auto const camera_end = camera.orientation();

    //QTransition = QFinal * QInitial^{-1}
    //auto const transition = camera_end * player_initial_inverse;

    //transform_.rotation = camera.orientation();//transform_.rotation * transition;
    //arrow_.rotation = camera.orientation();//arrow_.rotation * transition;

    float constexpr PI = glm::pi<float>();
    auto const player_or = glm::eulerAngles(this->orientation());
    auto const camera_or = glm::eulerAngles(camera.orientation());
    auto const delta_rot = rotate_towards(player_or, camera_or, PI/8);

    std::cerr << "matching camera's rotation\n";
    //transform_.rotation = camera.orientation();
    //arrow_.rotation = camera.orientation();
  }
};

} // ns boomhs

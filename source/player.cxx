#include <boomhs/player.hpp>
#include <stlw/format.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/ext.hpp>
#include <iostream>

namespace {

glm::quat
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

glm::quat
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

} // ns anonymous

namespace boomhs
{

std::string
Player::display() const
{
  return fmt::sprintf(
      "world_pos: '%s'\nforward_: '%s'\nforward_vector: '%s'\nright_vector: '%s'\nup_vector: '%s'\nquat: '%s'\n",
      glm::to_string(world_position()),
      glm::to_string(forward_),
      glm::to_string(forward_vector()),
      glm::to_string(right_vector()),
      glm::to_string(up_vector()),
      glm::to_string(orientation())
      );
}

void
Player::rotate(float const angle, window::mouse_data const& mdata)
{
  //auto const& current = mdata.current;
  //glm::vec2 const delta = glm::vec2{current.xrel, current.yrel};

  //auto const& mouse_sens = mdata.sensitivity;
  //auto constexpr yaw = 0.0f;
  //auto const pitch = angle * mouse_sens.x * delta.x * 0.1f;
  //auto constexpr roll = 0.0f;
  //auto const new_rotation = glm::quat{glm::vec3{yaw, pitch, roll}};

  float constexpr ROT = 3.0f;
  bool const left = mdata.current.xrel < 0;
  float const a = left ? ROT : -ROT;
  glm::quat const new_rotation = glm::angleAxis(glm::radians(a), glm::vec3{0.0, 1.0f, 0.0f});
  transform_.rotation = new_rotation * transform_.rotation;
  arrow_.rotation = new_rotation * arrow_.rotation;
}

void
Player::multiply_quat(glm::quat const& quat)
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

  //float constexpr PI = glm::pi<float>();
  //auto const player_or = glm::eulerAngles(this->orientation());
  //auto const camera_or = glm::eulerAngles(camera.orientation());
  //auto const delta_rot = rotate_towards(player_or, camera_or, PI/8);

  //std::cerr << "matching camera's rotation\n";

  //glm::quat const quat = rotation_between_vectors(forward_vector(), fwd_v);
  ////transform_.rotation = quat * transform_.rotation;
  //arrow_.rotation = quat * arrow_.rotation;
  //transform_.rotation = camera.orientation();
  //arrow_.rotation = camera.orientation();
}

} // ns boomhs

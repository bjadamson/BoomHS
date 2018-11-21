#pragma once
#include <extlibs/glm.hpp>

namespace boomhs::math
{

enum class EulerAxis
{
  X = 0,
  Y,
  Z,
  INVALID
};

///////////////////////////////
// Quaternion to Euler
//
// algorithm modified from source:
// http://bediyap.com/programming/convert-quaternion-to-euler-rotations/
///////////////////////////////
enum class RotSeq
{
  zyx,
  zyz,
  zxy,
  zxz,
  yxz,
  yxy,
  yzx,
  yzy,
  xyz,
  xyx,
  xzy,
  xzx
};
glm::vec3
quat_to_euler(glm::quat const&, RotSeq);

} // namespace boomhs::math

#include <boomhs/euler.hpp>
#include <boomhs/math.hpp>

namespace
{

glm::vec3
three_axis_rotation(float const r11, float const r12, float const r21, float const r31,
                    float const r32)
{
  glm::vec3 res;
  res[0] = std::atan2(r11, r12);
  res[1] = std::acos(r21);
  res[2] = std::atan2(r31, r32);
  return res;
}

glm::vec3
threeaxisrot(float const r11, float const r12, float const r21, float const r31, float const r32)
{
  glm::vec3 res;
  res[0] = std::atan2(r31, r32);
  res[1] = std::asin(r21);
  res[2] = std::atan2(r11, r12);
  return res;
}

} // namespace


namespace boomhs::math
{

glm::vec3
quat_to_euler(glm::quat const& q, RotSeq const rot_seq)
{
  switch (rot_seq) {
  case RotSeq::zyx:
    return threeaxisrot(2 * (q.x * q.y + q.w * q.z), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z,
                        -2 * (q.x * q.z - q.w * q.y), 2 * (q.y * q.z + q.w * q.x),
                        q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);
    break;

  case RotSeq::zyz:
    return three_axis_rotation(2 * (q.y * q.z - q.w * q.x), 2 * (q.x * q.z + q.w * q.y),
                               q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z,
                               2 * (q.y * q.z + q.w * q.x), -2 * (q.x * q.z - q.w * q.y));
    break;

  case RotSeq::zxy:
    return threeaxisrot(-2 * (q.x * q.y - q.w * q.z), q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z,
                        2 * (q.y * q.z + q.w * q.x), -2 * (q.x * q.z - q.w * q.y),
                        q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);
    break;

  case RotSeq::zxz:
    return three_axis_rotation(2 * (q.x * q.z + q.w * q.y), -2 * (q.y * q.z - q.w * q.x),
                               q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z,
                               2 * (q.x * q.z - q.w * q.y), 2 * (q.y * q.z + q.w * q.x));
    break;

  case RotSeq::yxz:
    return threeaxisrot(2 * (q.x * q.z + q.w * q.y), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z,
                        -2 * (q.y * q.z - q.w * q.x), 2 * (q.x * q.y + q.w * q.z),
                        q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z);
    break;

  case RotSeq::yxy:
    return three_axis_rotation(2 * (q.x * q.y - q.w * q.z), 2 * (q.y * q.z + q.w * q.x),
                               q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z,
                               2 * (q.x * q.y + q.w * q.z), -2 * (q.y * q.z - q.w * q.x));
    break;

  case RotSeq::yzx:
    return threeaxisrot(-2 * (q.x * q.z - q.w * q.y), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z,
                        2 * (q.x * q.y + q.w * q.z), -2 * (q.y * q.z - q.w * q.x),
                        q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z);
    break;

  case RotSeq::yzy:
    return three_axis_rotation(2 * (q.y * q.z + q.w * q.x), -2 * (q.x * q.y - q.w * q.z),
                               q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z,
                               2 * (q.y * q.z - q.w * q.x), 2 * (q.x * q.y + q.w * q.z));
    break;

  case RotSeq::xyz:
    return threeaxisrot(-2 * (q.y * q.z - q.w * q.x), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z,
                        2 * (q.x * q.z + q.w * q.y), -2 * (q.x * q.y - q.w * q.z),
                        q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
    break;

  case RotSeq::xyx:
    return three_axis_rotation(2 * (q.x * q.y + q.w * q.z), -2 * (q.x * q.z - q.w * q.y),
                               q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z,
                               2 * (q.x * q.y - q.w * q.z), 2 * (q.x * q.z + q.w * q.y));
    break;

  case RotSeq::xzy:
    return threeaxisrot(2 * (q.y * q.z + q.w * q.x), q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z,
                        -2 * (q.x * q.y - q.w * q.z), 2 * (q.x * q.z + q.w * q.y),
                        q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
    break;

  case RotSeq::xzx:
    return three_axis_rotation(2 * (q.x * q.z - q.w * q.y), 2 * (q.x * q.y + q.w * q.z),
                               q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z,
                               2 * (q.x * q.z + q.w * q.y), -2 * (q.x * q.y - q.w * q.z));
    break;
  default:
    break;
  }
  std::abort();
  return constants::ZERO;
}

} // namespace boomhs::math

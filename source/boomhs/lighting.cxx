#include <boomhs/lighting.hpp>
#include <common/algorithm.hpp>

using namespace opengl;

namespace boomhs
{

Attenuation
operator*(Attenuation const& att, float const v)
{
  Attenuation result = att;
  result *= v;
  return result;
}

Attenuation&
operator*=(Attenuation &att, float const v)
{
  att.constant *= v;
  att.linear *= v;
  att.quadratic *= v;
  return att;
}

Attenuation
operator/(Attenuation const& att, float const v)
{
  Attenuation result = att;
  result /= v;
  return result;
}

Attenuation&
operator/=(Attenuation &att, float const v)
{
  att.constant /= v;
  att.linear /= v;
  att.quadratic /= v;
  return att;
}

std::ostream&
operator<<(std::ostream& stream, Attenuation const& att)
{
  stream << "{";
  stream << att.constant << ", " << att.linear << ", " << att.quadratic;
  stream << "}";
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GlobalLight
GlobalLight::GlobalLight(Color const& amb, DirectionalLight &&dl)
  : ambient(amb)
  , directional(MOVE(dl))
{
}

} // ns boomhs

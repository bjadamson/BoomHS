#include <opengl/lighting.hpp>

namespace opengl
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Material
Material::Material(glm::vec3 const& amb, glm::vec3 const& diff, glm::vec3 const& spec, float const shiny)
  : ambient(amb)
  , diffuse(diff)
  , specular(spec)
  , shininess(shiny)
{
}

Material::Material(opengl::Color const& amb, opengl::Color const& diff, opengl::Color const& spec,
    float const shiny)
  : Material(amb.rgb(), diff.rgb(), spec.rgb(), shiny)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GlobalLight
GlobalLight::GlobalLight(opengl::Color const& amb, DirectionalLight &&dl)
  : ambient(amb)
  , directional(MOVE(dl))
{
}

} // ns opengl

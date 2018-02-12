#pragma once
#include <array>
#include <opengl/glew.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

struct Attenuation
{
  float constant;
  float linear;
  float quadratic;
};

// https://learnopengl.com/#!Lighting/Light-casters
static auto constexpr ATTENUATION_VALUE_TABLE = stlw::make_array<Attenuation>(
    Attenuation{1.0, 0.7, 1.8},
    Attenuation{1.0, 0.35, 0.44},
    Attenuation{1.0, 0.22, 0.20},
    Attenuation{1.0, 0.14, 0.07},
    Attenuation{1.0, 0.09, 0.032},
    Attenuation{1.0, 0.07, 0.017},
    Attenuation{1.0, 0.045, 0.0075},
    Attenuation{1.0, 0.027, 0.0028},
    Attenuation{1.0, 0.022, 0.0019},
    Attenuation{1.0, 0.014, 0.0007},
    Attenuation{1.0, 0.007, 0.0002},
    Attenuation{1.0, 0.0014, 0.000007}
    );

// https://learnopengl.com/#!Lighting/Light-casters
static auto constexpr ATTENUATION_DISTANCE_STRINGS =
    "7\0"
    "13\0"
    "20\0"
    "32\0"
    "50\0"
    "65\0"
    "100\0"
    "160\0"
    "200\0"
    "325\0"
    "600\0"
    "3250\0"
    "\0";

struct Material
{
  glm::vec3 ambient = LOC::WHITE.rgb();
  glm::vec3 diffuse = LOC::WHITE.rgb();
  glm::vec3 specular = LOC::WHITE.rgb();
  float shininess = 1.0;

  Material() = default;

  Material(glm::vec3 const& amb, glm::vec3 const& diff, glm::vec3 const& spec, float const shiny)
    : ambient(amb)
    , diffuse(diff)
    , specular(spec)
    , shininess(shiny)
  {
  }

  //
  // The alpha value for the colors is truncated.
  Material(opengl::Color const& amb, opengl::Color const& diff, opengl::Color const& spec,
      float const shiny)
    : Material(amb.rgb(), diff.rgb(), spec.rgb(), shiny)
  {
  }
};

struct Light
{
  opengl::Color diffuse = LOC::WHITE;
  opengl::Color specular = LOC::BLACK;

  static constexpr auto INIT_ATTENUATION_INDEX = ATTENUATION_VALUE_TABLE.size() - 1;
  Attenuation attenuation = opengl::ATTENUATION_VALUE_TABLE[INIT_ATTENUATION_INDEX];
};

struct PointLight
{
  Light light;
};

struct PointLights
{
  std::vector<Light> pointlights;
};

struct DirectionalLight
{
  Light light;
  glm::vec3 direction{0.0f, 0.0f, 0.0f};
};

struct GlobalLight
{
  opengl::Color ambient = LOC::BLACK;

  // TODO: could there be more than one instance of "directional light"?
  DirectionalLight directional;

  explicit GlobalLight(DirectionalLight &&dl)
    : directional(MOVE(dl))
  {
  }
};

} // ns opengl

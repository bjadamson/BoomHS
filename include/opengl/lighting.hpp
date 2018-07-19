#pragma once
#include <opengl/colors.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/type_macros.hpp>

#include <array>
#include <extlibs/glew.hpp>
#include <ostream>

namespace opengl
{

struct Attenuation
{
  float constant;
  float linear;
  float quadratic;
};

Attenuation operator*(Attenuation const&, float);

Attenuation&
operator*=(Attenuation&, float);

Attenuation
operator/(Attenuation const&, float);

Attenuation&
operator/=(Attenuation&, float const);

std::ostream&
operator<<(std::ostream&, Attenuation const&);

// https://learnopengl.com/#!Lighting/Light-casters
static constexpr auto ATTENUATION_VALUE_TABLE = stlw::make_array<Attenuation>(
    Attenuation{1.0, 0.7, 1.8}, Attenuation{1.0, 0.35, 0.44}, Attenuation{1.0, 0.22, 0.20},
    Attenuation{1.0, 0.14, 0.07}, Attenuation{1.0, 0.09, 0.032}, Attenuation{1.0, 0.07, 0.017},
    Attenuation{1.0, 0.045, 0.0075}, Attenuation{1.0, 0.027, 0.0028},
    Attenuation{1.0, 0.022, 0.0019}, Attenuation{1.0, 0.014, 0.0007},
    Attenuation{1.0, 0.007, 0.0002}, Attenuation{1.0, 0.0014, 0.000007});
// https://learnopengl.com/#!Lighting/Light-casters
static constexpr auto ATTENUATION_DISTANCE_STRINGS = "7\0"
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
  glm::vec3 ambient   = glm::vec3{1.0f};
  glm::vec3 diffuse   = glm::vec3{1.0f};
  glm::vec3 specular  = glm::vec3{1.0f};
  float     shininess = 1.0f;

  Material() = default;
  MOVE_DEFAULT(Material);
  COPY_DEFAULT(Material);

  Material(glm::vec3 const&, glm::vec3 const&, glm::vec3 const&, float);

  //
  // The alpha value for the colors is truncated.
  Material(opengl::Color const&, opengl::Color const&, opengl::Color const&, float const);
};

struct Light
{
  opengl::Color diffuse  = LOC::WHITE;
  opengl::Color specular = LOC::BLACK;
};

struct DirectionalLight
{
  Light light;

  glm::vec3 direction       = {};
  glm::vec3 screenspace_pos = {};
};

struct GlobalLight
{
  opengl::Color ambient;

  // TODO: could there be more than one instance of "directional light"?
  DirectionalLight directional;

  explicit GlobalLight(opengl::Color const&, DirectionalLight&&);
};

struct PointLight
{
  Light light;

  static constexpr auto INIT_ATTENUATION_INDEX = ATTENUATION_VALUE_TABLE.size() - 1;
  Attenuation           attenuation = opengl::ATTENUATION_VALUE_TABLE[INIT_ATTENUATION_INDEX];
};

struct PointLights
{
  std::vector<Light> pointlights;
};

} // namespace opengl

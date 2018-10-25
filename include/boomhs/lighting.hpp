#pragma once
#include <boomhs/color.hpp>
#include <boomhs/entity.hpp>

#include <common/type_macros.hpp>

#include <array>

namespace boomhs
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
static constexpr auto ATTENUATION_VALUE_TABLE = common::make_array<Attenuation>(
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

struct Light
{
  ColorRGB diffuse  = LOC3::WHITE;
  ColorRGB specular = LOC3::BLACK;
};

struct DirectionalLight
{
  Light light;

  glm::vec3 direction       = {};
  glm::vec2 screenspace_pos = {};
};

struct GlobalLight
{
  ColorRGB ambient;

  // TODO: could there be more than one instance of "directional light"?
  DirectionalLight directional;

  explicit GlobalLight(ColorRGB const&, DirectionalLight&&);
};

struct PointLight
{
  Light light;

  static constexpr auto INIT_ATTENUATION_INDEX = ATTENUATION_VALUE_TABLE.size() - 1;
  Attenuation           attenuation            = ATTENUATION_VALUE_TABLE[INIT_ATTENUATION_INDEX];
};

struct PointLights
{
  std::vector<Light> pointlights;
};

inline auto
find_pointlights(EntityRegistry& registry)
{
  return find_all_entities_with_component<PointLight>(registry);
}

} // namespace boomhs

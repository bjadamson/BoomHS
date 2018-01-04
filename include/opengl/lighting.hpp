#pragma once
#include <array>
#include <opengl/glew.hpp>
#include <stlw/type_ctors.hpp>

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

} // ns opengl

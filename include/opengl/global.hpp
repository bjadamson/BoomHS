#pragma once
#include <opengl/types.hpp>
#include <extlibs/static_string.hpp>

#include <boomhs/world_object.hpp>

// Functions within this namespace have global side effects within OpenGL's internal state.
//
// Here be dragons.
namespace opengl::global
{
static auto const vao_bind   = [](auto& vao) { glBindVertexArray(vao.gl_raw_value()); };
static auto const vao_unbind = []() { glBindVertexArray(0); };

static auto const texture_bind = [](auto const& texture) {
  glBindTexture(texture.target, texture.id);
};
static auto const texture_unbind = [](auto const& texture) { glBindTexture(texture.target, 0); };

// Define a name for the default "View" matrix OpenGL assumes. (if you pass in a Identity matrix to
// all view computations, OpenGL transforms the data the same as if you used this matrix as the
// view matrix)
//
// TODO: Update GLM library and use constexpr
auto inline/* constexpr*/
default_viewmatrix()
{
  using namespace ::boomhs::math;

  auto const/*expr*/ EYE    = +boomhs::math::constants::ZERO;
  auto const/*expr*/ CENTER = -boomhs::math::constants::Z_UNIT_VECTOR;
  auto const/*expr*/ UP     = +boomhs::math::constants::Y_UNIT_VECTOR;
  return glm::lookAt(EYE, CENTER, UP);
}

} // namespace opengl::global

namespace opengl::glsl
{

auto constexpr
prefix_string()
{
  return static_str::literal(R"GLSL(#version 300 es
precision mediump float;

)GLSL");

}

// Helper macro to make it easier to embed glsl code into the source code.
// 1. Keep some glsl shader code as a static string within the final binary.
// 2. Maintain a single glsl version throughout the project.
//
// Call this macro when defining GLSL shader source code, and it will insert the correct prefix
// into the beginning of the shader code.
#define GLSL_SOURCE(SOURCE) \
  ::opengl::glsl::prefix_string() + SOURCE

} // namespace opengl::glsl

namespace opengl::world_orientation
{

// -Z is pointing FORWARD into the scene, +Y is pointing UP on the screen.
auto constexpr FORWARDZ_FORWARD = -boomhs::math::constants::Z_UNIT_VECTOR;
auto constexpr FORWARDZ_UP      = +boomhs::math::constants::Y_UNIT_VECTOR;
auto constexpr FORWARDZ         = boomhs::WorldOrientation{FORWARDZ_FORWARD, FORWARDZ_UP};

// +Z is pointing BACKWARD into the scene, +Y is pointing UP on the screen.
auto constexpr REVERSEZ_FORWARD = +boomhs::math::constants::Z_UNIT_VECTOR;
auto constexpr REVERSEZ_UP      = +boomhs::math::constants::Y_UNIT_VECTOR;
auto constexpr REVERSEZ         = boomhs::WorldOrientation{REVERSEZ_FORWARD, REVERSEZ_UP};

// -Y is pointing FORWARD into the scene, +Z is pointing UP on the screen.
auto constexpr TOPDOWN_FORWARD  = -boomhs::math::constants::Y_UNIT_VECTOR;
auto constexpr TOPDOWN_UP       = +boomhs::math::constants::Z_UNIT_VECTOR;
auto constexpr TOPDOWN          = boomhs::WorldOrientation{TOPDOWN_FORWARD, TOPDOWN_UP};

// +Y is pointing FORWARD into the scene, +Z is pointing UP on the screen.
auto constexpr BOTTOMUP_FORWARD = +boomhs::math::constants::Y_UNIT_VECTOR;
auto constexpr BOTTOMUP_UP      = +boomhs::math::constants::Z_UNIT_VECTOR;
auto constexpr BOTTOMUP         = boomhs::WorldOrientation{BOTTOMUP_FORWARD, BOTTOMUP_UP};

// -X is pointing FORWARD into the scene, +Y is pointing UP on the screen.
auto constexpr FORWARDX_FORWARD = -boomhs::math::constants::X_UNIT_VECTOR;
auto constexpr FORWARDX_UP      = +boomhs::math::constants::Y_UNIT_VECTOR;
auto constexpr FORWARDX         = boomhs::WorldOrientation{FORWARDX_FORWARD, FORWARDX_UP};

// +X is pointing FORWARD into the scene, +Y is pointing UP on the screen.
// lol
auto constexpr REVERSEX_FORWARD = +boomhs::math::constants::X_UNIT_VECTOR;
auto constexpr REVERSEX_UP      = +boomhs::math::constants::Y_UNIT_VECTOR;
auto constexpr REVERSEX         = boomhs::WorldOrientation{REVERSEX_FORWARD, REVERSEX_UP};

} // namespace opengl::world_orientation

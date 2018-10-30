#pragma once
#include <opengl/types.hpp>
#include <extlibs/static_string.hpp>

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

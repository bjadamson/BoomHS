#pragma once
#include <gfx/opengl/gl_log.hpp>
#include <gfx/opengl/glew.hpp>

// Functions within this namespace have global side effects within OpenGL's internal state.
//
// Here be dragons.
namespace gfx::opengl::global
{
static auto const vao_bind = [](auto const vao) { glBindVertexArray(vao); };
static auto const vao_unbind = []() { glBindVertexArray(0); };

static auto const texture_bind = [](auto const texture) {
  glBindTexture(texture.mode, texture.id);
};
static auto const texture_unbind = [](auto const texture) { glBindTexture(texture.mode, 0); };

} // ns gfx::opengl::global

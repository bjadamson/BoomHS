#pragma once
#include <engine/gfx/opengl/gl.hpp>

// Functions within this namespace have global side effects within OpenGL's internal state.
//
// Here be dragons.
namespace engine::gfx::opengl::global
{
  static auto const vao_bind = [](auto const vao) { glBindVertexArray(vao); };
  static auto const vao_unbind = []() { glBindVertexArray(0); };

  static auto const texture_bind = [](auto const tid) { glBindTexture(GL_TEXTURE_2D, tid); };
  static auto const texture_unbind = []() { glBindTexture(GL_TEXTURE_2D, 0); };

} // ns engine::gfx::opengl::global

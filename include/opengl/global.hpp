#pragma once
#include <extlibs/glew.hpp>

// Functions within this namespace have global side effects within OpenGL's internal state.
//
// Here be dragons.
namespace opengl::global
{
static auto const vao_bind   = [](auto& vao) { glBindVertexArray(vao.gl_raw_value()); };
static auto const vao_unbind = []() { glBindVertexArray(0); };

static auto const texture_bind = [](auto const& texture) {
  glBindTexture(texture.target, texture.id());
};
static auto const texture_unbind = [](auto const& texture) { glBindTexture(texture.target, 0); };

} // namespace opengl::global

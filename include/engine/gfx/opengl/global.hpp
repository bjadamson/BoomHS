#pragma once
#include <engine/gfx/opengl/gl_log.hpp>
#include <engine/gfx/opengl/glew.hpp>

// Functions within this namespace have global side effects within OpenGL's internal state.
//
// Here be dragons.
namespace engine::gfx::opengl::global
{
static auto const vao_bind = [](auto const vao) { glBindVertexArray(vao); };
static auto const vao_unbind = []() { glBindVertexArray(0); };

static auto const texture_bind = [](auto const texture) { glBindTexture(texture.mode, texture.id); };
static auto const texture_unbind = [](auto const texture) { glBindTexture(texture.mode, 0); };

namespace log
{
static auto const get_shader_log = [](GLuint const handle) {
  return impl::retrieve(handle, glGetShaderInfoLog);
};

static auto const get_program_log = [](GLuint const handle) {
  return impl::retrieve(handle, glGetProgramInfoLog);
};

static auto const get_errors = [](GLuint const program_id) { return impl::get_errors(program_id); };
static auto const clear_gl_errors = []() { impl::clear_gl_errors(); };
} // ns log

} // ns engine::gfx::opengl::global

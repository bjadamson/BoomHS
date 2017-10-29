#pragma once
#include <opengl/types.hpp>
#include <opengl/colors.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

class opengl_buffers
{
  GLuint vbo_ = 0, ebo_ = 0;
  static auto constexpr NUM_BUFFERS = 1;

  explicit opengl_buffers()
  {
    glGenBuffers(NUM_BUFFERS, &this->vbo_);
    glGenBuffers(NUM_BUFFERS, &this->ebo_);
  }

  NO_COPY(opengl_buffers);
  NO_MOVE_ASSIGN(opengl_buffers);
public:
  friend class shape;
  ~opengl_buffers()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->ebo_);
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
  }

  // move-construction OK.
  opengl_buffers(opengl_buffers &&other)
      : vbo_(other.vbo_)
      , ebo_(other.ebo_)
  {
    other.vbo_ = 0;
    other.ebo_ = 0;
  }

  inline auto vbo() const { return this->vbo_; }
  inline auto ebo() const { return this->ebo_; }
};

class shape {
  GLenum draw_mode_;
  opengl_buffers gl_buffers_;
  bool in_gpu_memory_ = false;

protected:
  explicit shape(GLenum const dm)
      : draw_mode_(dm)
  {
  }

public:

  auto constexpr draw_mode() const { return this->draw_mode_; }

  inline auto vbo() const { return this->gl_buffers_.vbo(); }
  inline auto ebo() const { return this->gl_buffers_.ebo(); }

  bool is_in_gpu_memory() const { return this->in_gpu_memory_; }
  void set_is_in_gpu_memory(bool const v) { this->in_gpu_memory_ = v; }
};

struct vertex_attributes_only {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX;
};

struct vertex_color_attributes {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX + color_t::NUM_FLOATS_PER_VERTEX;
};

struct vertex_uv_attributes {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX + uv_t::NUM_FLOATS_PER_VERTEX;
};

struct vertex_normal_uv_attributes {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX
    + normal_t::NUM_FLOATS_PER_VERTEX
    + uv_t::NUM_FLOATS_PER_VERTEX;
};

} // ns opengl


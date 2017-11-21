#pragma once
#include <array>
#include <opengl/types.hpp>
#include <opengl/colors.hpp>
#include <opengl/obj.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

class gpu_buffers
{
  GLuint vbo_ = 0, ebo_ = 0;
  static auto constexpr NUM_BUFFERS = 1;

  explicit gpu_buffers()
  {
    glGenBuffers(NUM_BUFFERS, &this->vbo_);
    glGenBuffers(NUM_BUFFERS, &this->ebo_);
  }

  NO_COPY(gpu_buffers);
  NO_MOVE_ASSIGN(gpu_buffers);
public:
  friend class shape;
  ~gpu_buffers()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->ebo_);
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
  }

  // move-construction OK.
  gpu_buffers(gpu_buffers &&other)
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
  GLuint num_indices_;
  gpu_buffers gpu_buffers_;
  bool in_gpu_memory_ = false;

public:
  explicit shape(GLenum const dm, GLuint const num_indices)
      : draw_mode_(dm)
      , num_indices_(num_indices)
  {
  }

  auto constexpr draw_mode() const { return this->draw_mode_; }

  inline auto vbo() const { return this->gpu_buffers_.vbo(); }
  inline auto ebo() const { return this->gpu_buffers_.ebo(); }
  inline auto num_indices() const { return this->num_indices_; }

  bool is_in_gpu_memory() const { return this->in_gpu_memory_; }
  void set_is_in_gpu_memory(bool const v) { this->in_gpu_memory_ = v; }
};

} // ns opengl

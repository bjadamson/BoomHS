#pragma once
#include <array>
#include <opengl/types.hpp>
#include <opengl/colors.hpp>
#include <opengl/obj.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

class GpuHandles
{
  GLuint vbo_ = 0, ebo_ = 0;
  static auto constexpr NUM_BUFFERS = 1;

  explicit GpuHandles()
  {
    glGenBuffers(NUM_BUFFERS, &this->vbo_);
    glGenBuffers(NUM_BUFFERS, &this->ebo_);
  }

  NO_COPY(GpuHandles);
  NO_MOVE_ASSIGN(GpuHandles);
public:
  friend class DrawInfo;
  ~GpuHandles()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->ebo_);
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
  }

  // move-construction OK.
  GpuHandles(GpuHandles &&other)
      : vbo_(other.vbo_)
      , ebo_(other.ebo_)
  {
    other.vbo_ = 0;
    other.ebo_ = 0;
  }

  inline auto vbo() const { return this->vbo_; }
  inline auto ebo() const { return this->ebo_; }
};

class DrawInfo {
  GLenum draw_mode_;
  GLuint num_indices_;
  GpuHandles handles_;

public:
  explicit DrawInfo(GLenum const dm, GLuint const num_indices)
      : draw_mode_(dm)
      , num_indices_(num_indices)
  {
  }

  inline auto draw_mode() const { return this->draw_mode_; }

  inline auto vbo() const { return this->handles_.vbo(); }
  inline auto ebo() const { return this->handles_.ebo(); }
  inline auto num_indices() const { return this->num_indices_; }
};

} // ns opengl
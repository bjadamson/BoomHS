#pragma once
#include <array>
#include <boomhs/types.hpp>
#include <opengl/colors.hpp>
#include <opengl/obj.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

class GpuBufferHandles
{
  GLuint vbo_ = 0, ebo_ = 0;
  static auto constexpr NUM_BUFFERS = 1;

  explicit GpuBufferHandles()
  {
    glGenBuffers(NUM_BUFFERS, &this->vbo_);
    glGenBuffers(NUM_BUFFERS, &this->ebo_);
  }

  NO_COPY(GpuBufferHandles);
  NO_MOVE_ASSIGN(GpuBufferHandles);
public:
  friend class DrawInfo;
  ~GpuBufferHandles()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->ebo_);
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
  }

  // move-construction OK.
  GpuBufferHandles(GpuBufferHandles &&other)
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
  GpuBufferHandles handles_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(DrawInfo);
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

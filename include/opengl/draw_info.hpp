#pragma once
#include <opengl/vao.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

class GpuBufferHandles
{
  GLuint vbo_ = 0, ebo_ = 0;
  static auto constexpr NUM_BUFFERS = 1;

  explicit GpuBufferHandles()
  {
    glGenBuffers(NUM_BUFFERS, &vbo_);
    glGenBuffers(NUM_BUFFERS, &ebo_);
  }

  NO_COPY(GpuBufferHandles);
public:
  friend class DrawInfo;
  ~GpuBufferHandles()
  {
    glDeleteBuffers(NUM_BUFFERS, &ebo_);
    glDeleteBuffers(NUM_BUFFERS, &vbo_);
  }

  // move-construction OK.
  GpuBufferHandles(GpuBufferHandles &&other)
      : vbo_(other.vbo_)
      , ebo_(other.ebo_)
  {
    other.vbo_ = 0;
    other.ebo_ = 0;
  }

  GpuBufferHandles&
  operator=(GpuBufferHandles &&other)
  {
    vbo_ = other.vbo_;
    ebo_ = other.ebo_;

    other.vbo_ = 0;
    other.ebo_ = 0;
    return *this;
  }

  auto vbo() const { return vbo_; }
  auto ebo() const { return ebo_; }
};

class DrawInfo
{
  GLenum draw_mode_;
  GLuint num_indices_;
  GpuBufferHandles handles_;
  VAO vao_;

public:
  NO_COPY(DrawInfo);
  explicit DrawInfo(GLenum const dm, GLuint const num_indices)
      : draw_mode_(dm)
      , num_indices_(num_indices)
  {
  }

  DrawInfo(DrawInfo &&other)
    : draw_mode_(other.draw_mode_)
    , num_indices_(other.num_indices_)
    , handles_(MOVE(other.handles_))
    , vao_(MOVE(other.vao_))
  {
  }

  DrawInfo& operator=(DrawInfo &&di)
  {
    draw_mode_ = di.draw_mode_;
    num_indices_ = di.num_indices_;
    handles_ = MOVE(di.handles_);
    vao_ = MOVE(di.vao_);
    return *this;
  }

  auto const& vao() const { return vao_; }

  auto draw_mode() const { return draw_mode_; }
  auto vbo() const { return handles_.vbo(); }
  auto ebo() const { return handles_.ebo(); }
  auto num_indices() const { return num_indices_; }
};

} // ns opengl

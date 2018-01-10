#pragma once
#include <opengl/texture.hpp>
#include <opengl/vao.hpp>
#include <stlw/type_macros.hpp>
#include <boost/optional.hpp>

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

  boost::optional<texture_info> texture_info_;

public:
  NO_COPY(DrawInfo);
  explicit DrawInfo(GLenum const dm, GLuint const num_indices, boost::optional<texture_info> &&ti)
      : draw_mode_(dm)
      , num_indices_(num_indices)
      , texture_info_(MOVE(ti))
  {
  }

  DrawInfo(DrawInfo &&other)
    : draw_mode_(other.draw_mode_)
    , num_indices_(other.num_indices_)
    , handles_(MOVE(other.handles_))
    , vao_(MOVE(other.vao_))
    , texture_info_(MOVE(other.texture_info_))
  {
  }

  DrawInfo& operator=(DrawInfo &&other)
  {
    draw_mode_ = other.draw_mode_;
    num_indices_ = other.num_indices_;
    other.draw_mode_ = -1;
    other.num_indices_ = 0;

    handles_ = MOVE(other.handles_);
    vao_ = MOVE(other.vao_);
    texture_info_ = MOVE(other.texture_info_);
    return *this;
  }

  auto const& vao() const { return vao_; }

  auto draw_mode() const { return draw_mode_; }
  auto vbo() const { return handles_.vbo(); }
  auto ebo() const { return handles_.ebo(); }
  auto num_indices() const { return num_indices_; }

  auto texture_info() const { return texture_info_; }
};

} // ns opengl

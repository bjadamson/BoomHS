#pragma once
#include <opengl/texture.hpp>
#include <opengl/vao.hpp>
#include <opengl/vertex_attribute.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/optional.hpp>

namespace opengl
{

class BufferHandles
{
  GLuint vbo_ = 0, ebo_ = 0;
  static auto constexpr NUM_BUFFERS = 1;

  explicit BufferHandles();
  NO_COPY(BufferHandles);
public:
  friend class DrawInfo;
  ~BufferHandles();

  // move-construction OK.
  BufferHandles(BufferHandles &&);

  BufferHandles&
  operator=(BufferHandles &&);

  auto vbo() const { return vbo_; }
  auto ebo() const { return ebo_; }
};

std::ostream&
operator<<(std::ostream &, BufferHandles const&);

class DrawInfo
{
  GLenum draw_mode_;
  std::size_t num_vertices_;
  GLuint num_indices_;
  BufferHandles handles_;
  VAO vao_;

  stlw::optional<TextureInfo> texture_info_;

public:
  NO_COPY(DrawInfo);
  explicit DrawInfo(GLenum const, std::size_t const, GLuint const, stlw::optional<TextureInfo> const&);

  DrawInfo(DrawInfo &&);
  DrawInfo& operator=(DrawInfo &&other);

  auto const& vao() const { return vao_; }
  auto draw_mode() const { return draw_mode_; }
  auto vbo() const { return handles_.vbo(); }
  auto ebo() const { return handles_.ebo(); }
  auto num_vertices() const { return num_vertices_; }
  auto num_indices() const { return num_indices_; }

  auto texture_info() const { return texture_info_; }

  void print_self(std::ostream&, VertexAttribute const&) const;
};

} // ns opengl

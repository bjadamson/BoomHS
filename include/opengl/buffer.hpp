#pragma once
#include <boomhs/obj.hpp>
#include <boomhs/colors.hpp>
#include <string>

namespace opengl
{
class VertexAttribute;

struct BufferFlags
{
  bool const vertices, normals, colors, uvs;

  explicit constexpr BufferFlags(bool const v, bool const n, bool const c, bool const u)
      : vertices(v)
      , normals(n)
      , colors(c)
      , uvs(u)
  {
  }

  std::string to_string() const;

  static BufferFlags from_va(VertexAttribute const&);
};

bool
operator==(BufferFlags const&, BufferFlags const&);

bool
operator!=(BufferFlags const&, BufferFlags const&);

std::ostream&
operator<<(std::ostream&, BufferFlags const&);

struct VertexBuffer
{
  boomhs::ObjVertices vertices;
  boomhs::ObjIndices  indices;
  BufferFlags const   flags;

private:
  VertexBuffer(BufferFlags const&);
  COPY_DEFAULT(VertexBuffer);

public:
  MOVE_DEFAULT(VertexBuffer);

  std::string to_string() const;

  // Public copy method
  VertexBuffer copy() const;

  void set_colors(boomhs::Color const&);
  static VertexBuffer
  create_interleaved(common::Logger&, boomhs::ObjData const&, BufferFlags const&);
};

} // namespace opengl

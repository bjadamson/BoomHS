#pragma once
#include <boomhs/obj.hpp>
#include <string>

namespace opengl
{
class VertexAttribute;

struct BufferFlags
{
  bool const vertices, normals, colors, uvs;

  explicit BufferFlags(bool const v, bool const n, bool const c, bool const u)
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
  using vertices_t = boomhs::ObjData::vertices_t;
  using indices_t  = boomhs::ObjData::indices_t;

  vertices_t vertices;
  indices_t  indices;

  VertexBuffer() = default;
  MOVE_CONSTRUCTIBLE_ONLY(VertexBuffer);

  std::string to_string() const;

  static VertexBuffer create_interleaved(stlw::Logger&, boomhs::ObjData const&, BufferFlags const&);
};

} // namespace opengl

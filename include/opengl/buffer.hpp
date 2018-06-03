#pragma once
#include <boomhs/obj.hpp>
#include <opengl/colors.hpp>
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

struct PositionsBuffer
{
  using vertices_t = boomhs::ObjData::vertices_t;
  vertices_t vertices;

  PositionsBuffer(vertices_t&&);

  glm::vec3 min() const;
  glm::vec3 max() const;
};

struct VertexBuffer
{
  using vertices_t = boomhs::ObjData::vertices_t;
  using indices_t  = boomhs::ObjData::indices_t;

  vertices_t        vertices;
  indices_t         indices;
  BufferFlags const flags;

private:
  VertexBuffer(BufferFlags const&);
  COPY_DEFAULT(VertexBuffer);

public:
  MOVE_DEFAULT(VertexBuffer);

  std::string to_string() const;

  // Public copy method
  VertexBuffer copy() const;

  // Returns all position values as a contiguos array following the pattern:
  // [x, y, z], [x, y, z], etc...
  PositionsBuffer positions() const;

  void set_colors(Color const&);

  static VertexBuffer create_interleaved(stlw::Logger&, boomhs::ObjData const&, BufferFlags const&);
};

} // namespace opengl

#pragma once
#include <boomhs/color.hpp>

#include <common/log.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <extlibs/glew.hpp>
#include <extlibs/tinyobj.hpp>

#include <optional>
#include <ostream>
#include <string>

namespace boomhs
{

using ObjVertices = std::vector<float>;
using ObjIndices  = std::vector<uint32_t>;

struct PositionsBuffer
{
  ObjVertices vertices;

  PositionsBuffer(ObjVertices&&);

  glm::vec3 min() const;
  glm::vec3 max() const;
};

class ObjData
{
  COPY_DEFAULT(ObjData);

public:
  unsigned int num_vertexes;
  ObjVertices  vertices;
  ObjVertices  colors;
  ObjVertices  normals;
  ObjVertices  uvs;
  ObjIndices   indices;

  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;

  ObjData() = default;
  MOVE_DEFAULT(ObjData);

  ObjData clone() const;

  // Returns all position values as a contiguos array following the pattern:
  // [x, y, z], [x, y, z], etc...
  PositionsBuffer positions() const;

  std::string to_string() const;

#define FOREACH_FACE_IMPL(fn)                                                                      \
  FOR(s, shapes.size())                                                                            \
  {                                                                                                \
    size_t      index_offset = 0;                                                                  \
    auto const& shape        = shapes[s];                                                          \
    auto const& mesh         = shape.mesh;                                                         \
    FOR(f, mesh.num_face_vertices.size()) { fn(shape, f); }                                        \
  }

  template <typename FN>
  void foreach_face(FN const& fn)
  {
    FOREACH_FACE_IMPL(fn);
  }

  template <typename FN>
  void foreach_face(FN const& fn) const
  {
    FOREACH_FACE_IMPL(fn);
  }
#undef FOREACH_FACE_IMPL
};

enum class LoadStatus
{
  MISSING_POSITION_ATTRIBUTES = 0,
  MISSING_COLOR_ATTRIBUTES,
  MISSING_NORMAL_ATTRIBUTES,
  MISSING_UV_ATTRIBUTES,

  TINYOBJ_ERROR,

  SUCCESS
};

std::string
loadstatus_to_string(LoadStatus const ls);

std::ostream&
operator<<(std::ostream&, LoadStatus const&);

using LoadResult = Result<ObjData, LoadStatus>;

LoadResult
load_objfile(common::Logger&, char const*, char const*);

LoadResult
load_objfile(common::Logger&, std::string const&, std::string const&);

} // namespace boomhs

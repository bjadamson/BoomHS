#pragma once
#include <boomhs/obj.hpp>
#include <common/log.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{
class Heightmap;

struct GenerateNormalData
{
  bool const       invert_normals;
  Heightmap const& heightmap;
  size_t const     num_vertexes;
};

struct MeshFactory
{
  MeshFactory() = delete;

  static ObjVertices generate_rectangle_mesh(common::Logger&, glm::vec2 const&, size_t);

  static ObjVertices generate_uvs(common::Logger&, glm::vec2 const&, size_t, bool);

  static ObjIndices generate_indices(common::Logger&, size_t);

  static ObjVertices generate_normals(common::Logger&, GenerateNormalData const&);
  static ObjVertices generate_flat_normals(common::Logger&, size_t);
};

} // namespace boomhs

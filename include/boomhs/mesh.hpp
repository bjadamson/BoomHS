#pragma once
#include <boomhs/obj.hpp>
#include <extlibs/glm.hpp>
#include <stlw/log.hpp>

namespace boomhs
{

struct GenerateNormalData
{
  bool const                  invert_normals;
  std::vector<uint8_t> const& height_data;
  size_t const                num_vertexes;
};

struct MeshFactory
{
  MeshFactory() = delete;

  static ObjData::vertices_t generate_rectangle_mesh(stlw::Logger&, glm::vec2 const&, size_t);

  static ObjData::vertices_t generate_uvs(glm::vec2 const&, size_t);

  static ObjData::indices_t generate_indices(size_t);

  static ObjData::vertices_t generate_normals(GenerateNormalData const&);
  static ObjData::vertices_t generate_flat_normals(size_t);
};

} // namespace boomhs

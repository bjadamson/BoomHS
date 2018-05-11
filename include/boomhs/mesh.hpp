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

  static ObjData::vertices_t generate_uvs(stlw::Logger&, glm::vec2 const&, size_t, bool);

  static ObjData::indices_t generate_indices(stlw::Logger&, size_t);

  static ObjData::vertices_t generate_normals(stlw::Logger&, GenerateNormalData const&);
  static ObjData::vertices_t generate_flat_normals(stlw::Logger&, size_t);
};

} // namespace boomhs

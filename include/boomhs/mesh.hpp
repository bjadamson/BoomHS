#pragma once
#include <boomhs/obj.hpp>
#include <stlw/log.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{

struct GenerateNormalData
{
  bool const invert_normals;
  std::vector<uint8_t> const& height_data;
};

struct MeshFactory
{
  MeshFactory() = delete;

  static ObjData::vertices_t
  generate_rectangle_mesh(stlw::Logger&, glm::vec2 const&, size_t);

  static ObjData::vertices_t
  generate_uvs(glm::vec2 const&, size_t);

  static ObjData::indices_t
  generate_indices(size_t);

  static ObjData::vertices_t
  generate_normals(glm::vec2 const&, GenerateNormalData const&);
};

} // ns boomhs

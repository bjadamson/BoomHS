#pragma once
#include <opengl/colors.hpp>
#include <opengl/glew.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_macros.hpp>
#include <optional>

namespace boomhs
{

struct ObjData
{
  using vertices_t = std::vector<float>;
  using indices_t = std::vector<uint32_t>;

  unsigned int num_vertices;
  vertices_t positions;
  vertices_t colors;
  vertices_t normals;
  vertices_t uvs;
  indices_t indices;

  ObjData() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ObjData);
};

struct ObjBuffer
{
  using vertices_t = ObjData::vertices_t;
  using indices_t = ObjData::indices_t;

  unsigned int num_vertices;
  vertices_t vertices;
  indices_t indices;

  ObjBuffer() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ObjBuffer);
};

struct LoadMeshConfig
{
  bool colors = false;
  bool normals = false;
  bool uvs = false;
};

ObjData
load_mesh(char const*, char const*, LoadMeshConfig const&);

ObjData
load_mesh(char const*, LoadMeshConfig const&);

} // ns boomhs

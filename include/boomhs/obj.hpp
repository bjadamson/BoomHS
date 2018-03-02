#pragma once
#include <opengl/colors.hpp>
#include <opengl/glew.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{

struct ObjBuffer
{
  using vertices_t = std::vector<float>;
  using indices_t = std::vector<uint32_t>;

  unsigned int num_vertices;
  vertices_t vertices;
  indices_t indices;

  MOVE_CONSTRUCTIBLE_ONLY(ObjBuffer);
};

struct LoadMeshConfig
{
  bool colors = false;
  bool normals = false;
  bool uvs = false;
};

ObjBuffer
load_mesh(char const*, char const*, LoadMeshConfig const&);

ObjBuffer
load_mesh(char const*, LoadMeshConfig const&);

} // ns boomhs

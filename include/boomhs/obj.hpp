#pragma once
#include <opengl/colors.hpp>
#include <opengl/glew.hpp>
#include <stlw/result.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_macros.hpp>
#include <optional>
#include <ostream>

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
operator<<(std::ostream &, LoadStatus const&);

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

using LoadResult = Result<ObjData, LoadStatus>;

LoadResult
load_objfile(char const*, char const*);

LoadResult
load_objfile(char const*);

} // ns boomhs

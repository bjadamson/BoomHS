#pragma once
#include <opengl/colors.hpp>

#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glew.hpp>
#include <optional>
#include <ostream>
#include <string>

namespace boomhs
{

struct ObjData
{
  using vertices_t = std::vector<float>;
  using indices_t  = std::vector<uint32_t>;

  unsigned int num_vertexes;
  vertices_t   vertices;
  vertices_t   colors;
  vertices_t   normals;
  vertices_t   uvs;
  indices_t    indices;

  ObjData() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ObjData);

  std::string to_string() const;
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
load_objfile(stlw::Logger&, char const*);

LoadResult
load_objfile(stlw::Logger&, std::string const&);

} // namespace boomhs

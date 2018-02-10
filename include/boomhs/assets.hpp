#pragma once
#include <opengl/draw_info.hpp>
#include <opengl/obj.hpp>
#include <opengl/factory.hpp>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>

#include <stlw/format.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_macros.hpp>

#include <boost/algorithm/string.hpp>
#include <array>
#include <string>
#include <vector>
#include <utility>

namespace boomhs
{

class ObjCache
{
  using pair_t = std::pair<std::string, opengl::obj>;
  std::vector<pair_t> objects_;
public:
  ObjCache() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ObjCache);

  void
  add_obj(std::string const&, opengl::obj &&);

  void
  add_obj(char const*, opengl::obj &&);

  opengl::obj const&
  get_obj(char const*) const;

  opengl::obj const&
  get_obj(std::string const&) const;
};

enum GeometryType
{
  Cube = 0,
  Mesh,
};

inline GeometryType
from_string(std::string &string)
{
  boost::to_lower(string);
  if (string == "cube") {
    return Cube;
  } else if (string == "mesh") {
    return Mesh;
  }
  std::abort();
}

struct LoadedEntities
{
  std::vector<uint32_t> data = {};

  LoadedEntities() = default;
  MOVE_CONSTRUCTIBLE_ONLY(LoadedEntities);
  BEGIN_END_FORWARD_FNS(data);

  bool empty() const { return data.empty(); }
};

// TODO: not final by any means..
struct Assets
{
  ObjCache obj_cache;
  LoadedEntities loaded_entities;
  opengl::TextureTable texture_table;

  opengl::GlobalLight global_light;
  opengl::Color background_color;

  MOVE_CONSTRUCTIBLE_ONLY(Assets);
};

class GpuHandleList
{
  std::vector<opengl::DrawInfo> drawinfos_;
  std::vector<uint32_t> entities_;

public:
  GpuHandleList() = default;
  MOVE_CONSTRUCTIBLE_ONLY(GpuHandleList);

  size_t
  add(uint32_t const, opengl::DrawInfo &&);

  bool empty() const { return drawinfos_.empty(); }

  opengl::DrawInfo const&
  get(uint32_t const entity) const;
};

class HandleManager
{
  GpuHandleList list_;
public:
  uint32_t plus_eid;
  uint32_t hashtag_eid;
  uint32_t river_eid;
  uint32_t stair_down_eid;
  uint32_t stair_up_eid;

  MOVE_CONSTRUCTIBLE_ONLY(HandleManager);
  explicit HandleManager(GpuHandleList &&list, uint32_t const plus, uint32_t const hashtag,
      uint32_t const river, uint32_t const stair_down, uint32_t const stair_up)
    : list_(MOVE(list))
    , plus_eid(plus)
    , hashtag_eid(hashtag)
    , river_eid(river)
    , stair_down_eid(stair_down)
    , stair_up_eid(stair_up)
  {
  }

  opengl::DrawInfo const&
  lookup(uint32_t const entity) const;
};

} // ns boomhs

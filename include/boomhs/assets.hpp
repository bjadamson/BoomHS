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

struct TileInfo
{
  TileType type;
  std::string mesh_name, vshader_name;
  opengl::Color color;
  opengl::Material material;
};

struct TileInfos
{
  static auto constexpr SIZE = static_cast<size_t>(TileType::MAX);
  std::array<TileInfo, SIZE> data_;

public:
  TileInfos() = default;
  MOVE_CONSTRUCTIBLE_ONLY(TileInfos);
  BEGIN_END_FORWARD_FNS(data_);

  bool empty() const { return data_.empty(); }

  TileInfo const&
  operator[](TileType) const;

  TileInfo&
  operator[](TileType);
};

// TODO: not final by any means..
struct Assets
{
  ObjCache obj_cache;
  LoadedEntities loaded_entities;
  opengl::TextureTable texture_table;
  TileInfos tile_infos;

  opengl::GlobalLight global_light;
  opengl::Color background_color;

  MOVE_CONSTRUCTIBLE_ONLY(Assets);
};

} // ns boomhs

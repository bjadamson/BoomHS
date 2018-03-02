#pragma once
#include <boomhs/obj_cache.hpp>
#include <boomhs/tile.hpp>

#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <string>

namespace boomhs
{
class EntityRegistry;

struct TileInfo
{
  TileType type;
  std::string mesh_name, vshader_name;
  opengl::Color color;
  opengl::Material material;
};

// This table holds all the per-type tile information. There will be one entry in this array per
// TileType. This information is "shared information" between tiles of the same type.
struct TileSharedInfoTable
{
  static auto constexpr SIZE = static_cast<size_t>(TileType::UNDEFINED);
  std::array<TileInfo, SIZE> data_;

public:
  TileSharedInfoTable() = default;
  MOVE_CONSTRUCTIBLE_ONLY(TileSharedInfoTable);
  BEGIN_END_FORWARD_FNS(data_);

  bool empty() const { return data_.empty(); }

  TileInfo const&
  operator[](TileType) const;

  TileInfo&
  operator[](TileType);
};

struct LevelAssets
{
  opengl::GlobalLight global_light;
  opengl::Color background_color;

  TileSharedInfoTable tile_table;

  ObjCache obj_cache;
  opengl::TextureTable texture_table;
  opengl::ShaderPrograms shader_programs;

  MOVE_CONSTRUCTIBLE_ONLY(LevelAssets);
};

struct LevelLoader
{
  LevelLoader() = delete;

  static stlw::result<LevelAssets, std::string>
  load_level(stlw::Logger &, EntityRegistry &, std::string const&);
};

} // ns boomhs

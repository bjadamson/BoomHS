#pragma once
#include <boomhs/assets.hpp>
#include <opengl/shader.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <entt/entt.hpp>
#include <string>

namespace boomhs
{

struct LevelAssets
{
  opengl::GlobalLight global_light;
  opengl::Color background_color;

  TileInfos tile_infos;

  ObjCache obj_cache;
  opengl::TextureTable texture_table;
  opengl::ShaderPrograms shader_programs;

  MOVE_CONSTRUCTIBLE_ONLY(LevelAssets);
};

stlw::result<LevelAssets, std::string>
load_level(stlw::Logger &, entt::DefaultRegistry &, std::string const&);

} // ns boomhs

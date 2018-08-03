#pragma once
#include <boomhs/fog.hpp>
#include <boomhs/material.hpp>
#include <boomhs/obj_store.hpp>

#include <boomhs/lighting.hpp>
#include <opengl/colors.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <string>

namespace boomhs
{
class EntityRegistry;

struct LevelAssets
{
  GlobalLight   global_light;
  Fog           fog;
  MaterialTable material_table;

  ObjStore               obj_store;
  opengl::TextureTable   texture_table;
  opengl::ShaderPrograms shader_programs;

  MOVE_CONSTRUCTIBLE_ONLY(LevelAssets);
};

struct LevelLoader
{
  LevelLoader() = delete;

  static Result<LevelAssets, std::string>
  load_level(stlw::Logger&, EntityRegistry&, std::string const&);
};

} // namespace boomhs

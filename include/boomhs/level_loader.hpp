#pragma once
#include <boomhs/fog.hpp>
#include <boomhs/material.hpp>
#include <boomhs/obj_store.hpp>

#include <boomhs/lighting.hpp>
#include <boomhs/colors.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <common/log.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <string>
#include <vector>

namespace boomhs
{
class EntityRegistry;

struct NameAttenuation
{
  std::string const name;
  Attenuation const value;
};
using AttenuationList = std::vector<NameAttenuation>;

struct LevelAssets
{
  GlobalLight     global_light;
  Fog             fog;
  MaterialTable   material_table;
  AttenuationList attenuations;

  ObjStore               obj_store;
  opengl::TextureTable   texture_table;
  opengl::ShaderPrograms shader_programs;

  MOVE_CONSTRUCTIBLE_ONLY(LevelAssets);
};

struct LevelLoader
{
  LevelLoader() = delete;

  static Result<LevelAssets, std::string>
  load_level(common::Logger&, EntityRegistry&, std::string const&);
};

} // namespace boomhs

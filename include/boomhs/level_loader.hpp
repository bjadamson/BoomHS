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

struct LevelData
{
  Assets assets;
  opengl::ShaderPrograms shader_programs;

  MOVE_CONSTRUCTIBLE_ONLY(LevelData);
};

stlw::result<LevelData, std::string>
load_level(stlw::Logger &, entt::DefaultRegistry &, std::string const&);

} // ns boomhs

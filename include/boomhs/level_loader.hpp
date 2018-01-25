#pragma once
#include <boomhs/assets.hpp>
#include <opengl/shader.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>

#include <cpptoml/cpptoml.h>
#include <entt/entt.hpp>

#include <string>
#include <utility>

namespace boomhs
{

struct LevelData
{
  Assets assets;
  opengl::ShaderPrograms shader_programs;
};

stlw::result<LevelData, std::string>
load_level(stlw::Logger &, entt::DefaultRegistry &, std::string const&);

} // ns boomhs

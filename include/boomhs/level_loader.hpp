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

using CppTableArray = std::shared_ptr<cpptoml::table_array>;
using CppTable = std::shared_ptr<cpptoml::table>;

using AssetPair = std::pair<Assets, opengl::ShaderPrograms>;

stlw::result<AssetPair, std::string>
load_assets(stlw::Logger &, entt::DefaultRegistry &);

} // ns boomhs

#pragma once
#include <boomhs/assets.hpp>
#include <cpptoml/cpptoml.h>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <string>

namespace boomhs
{

using CppTableArray = std::shared_ptr<cpptoml::table_array>;
using CppTable = std::shared_ptr<cpptoml::table>;

stlw::result<Assets, std::string>
load_assets(stlw::Logger &);

} // ns boomhs

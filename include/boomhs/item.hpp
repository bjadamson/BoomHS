#pragma once
#include <opengl/texture.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{

struct Item
{
  char const*         name = "UNNAMED";
  char const*         tooltip = "TOOLTIP NOT SET";
  bool                is_pickedup = false;
  opengl::TextureInfo ui_tinfo;
};

} // namespace boomhs

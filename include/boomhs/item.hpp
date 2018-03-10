#pragma once
#include <stlw/type_macros.hpp>
#include <opengl/texture.hpp>

namespace boomhs
{

struct Item
{
  char const* name = "UNNAMED";
  char const* tooltip = "TOOLTIP NOT SET";
  bool is_pickedup = false;
  opengl::TextureInfo ui_tinfo;
};

} // ns boomhs

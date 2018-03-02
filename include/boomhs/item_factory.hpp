#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/tile.hpp>

namespace opengl
{
class TextureTable;
} // ns opengl

namespace stlw
{
class float_generator;
} // ns stlw

namespace boomhs
{

struct ItemFactory
{
  ItemFactory() = delete;

  static EntityID
  create_torch(EntityRegistry &, stlw::float_generator &, opengl::TextureTable const&);
};

} // ns boomhs

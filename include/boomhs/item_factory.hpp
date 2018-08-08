#pragma once
#include <boomhs/entity.hpp>

namespace opengl
{
class TextureTable;
} // namespace opengl

namespace stlw
{
class float_generator;
} // namespace stlw

namespace boomhs
{

struct ItemFactory
{
  ItemFactory() = delete;

  static EntityID create_empty(EntityRegistry&, opengl::TextureTable&);

  static EntityID create_book(EntityRegistry&, opengl::TextureTable&);
  static EntityID create_spear(EntityRegistry&, opengl::TextureTable&);
  static EntityID create_torch(EntityRegistry&, opengl::TextureTable&);
};

} // namespace boomhs

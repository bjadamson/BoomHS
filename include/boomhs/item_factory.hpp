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

  static EntityID create_item(EntityRegistry&, opengl::TextureTable&, char const*, char const*,
                              char const*, char const*);

  static EntityID create_torch(EntityRegistry&, stlw::float_generator&, opengl::TextureTable&);
};

} // namespace boomhs

#include <boomhs/components.hpp>
#include <boomhs/item.hpp>
#include <boomhs/item_factory.hpp>
#include <boomhs/lighting.hpp>
#include <boomhs/material.hpp>
#include <boomhs/random.hpp>
#include <opengl/texture.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

EntityID
create_item(EntityRegistry& registry, TextureTable& ttable, char const* entity_name,
            char const* ui_name, char const* mesh_name, char const* texture, char const* shader)
{
  auto eid = ItemFactory::create_empty(registry, ttable);

  registry.get<Name>(eid).value = entity_name;

  registry.assign<IsRenderable>(eid);
  registry.assign<MeshRenderable>(eid, mesh_name);

  auto& tr        = registry.assign<TextureRenderable>(eid);
  auto  texture_o = ttable.find(texture);
  assert(texture_o);
  tr.texture_info = &*texture_o;

  registry.assign<ShaderName>(eid, shader);

  auto& item    = registry.get<Item>(eid);
  item.ui_tinfo = &*ttable.find(ui_name);
  assert(item.ui_tinfo);

  return eid;
}

} // namespace

namespace boomhs
{

EntityID
ItemFactory::create_empty(EntityRegistry& registry, TextureTable& ttable)
{
  auto eid = registry.create();
  registry.assign<Name>(eid, "Empty Item");

  Item& item       = registry.assign<Item>(eid);
  item.is_pickedup = false;
  item.ui_tinfo    = &*ttable.find("RedX");
  assert(item.ui_tinfo);

  item.name    = "RedX";
  item.tooltip = "This is some kind of item";

  // leave for caller to assign
  registry.assign<Transform>(eid);

  return eid;
}

EntityID
ItemFactory::create_book(EntityRegistry& registry, TextureTable& ttable)
{
  auto eid = create_item(registry, ttable, "Book EID", "BookUI", "B", "container", "3dtexture");
  registry.assign<Book>(eid);
  registry.assign<Material>(eid);

  auto& item   = registry.get<Item>(eid);
  item.name    = "Book";
  item.tooltip = "This is a book";

  return eid;
}

EntityID
ItemFactory::create_spear(EntityRegistry& registry, opengl::TextureTable& ttable)
{
  auto eid =
      create_item(registry, ttable, "Spear EID", "SpearUI", "hashtag", "container", "3dtexture");
  registry.assign<Weapon>(eid);
  registry.assign<Material>(eid);

  auto& item   = registry.get<Item>(eid);
  item.name    = "Spear";
  item.tooltip = "A mighty slaying spear!";

  return eid;
}

EntityID
ItemFactory::create_torch(EntityRegistry& registry, TextureTable& ttable)
{
  auto eid = create_item(registry, ttable, "Torch EID", "TorchUI", "star", "Lava", "torch");
  registry.assign<Torch>(eid);

  auto& item   = registry.get<Item>(eid);
  item.name    = "Torch";
  item.tooltip = "This is a torch";

  auto& pointlight         = registry.assign<PointLight>(eid);
  pointlight.light.diffuse = LOC3::YELLOW;

  auto& flicker         = registry.assign<LightFlicker>(eid);
  flicker.base_speed    = 1.0f;
  flicker.current_speed = flicker.base_speed;

  flicker.colors[0] = LOC3::RED;
  flicker.colors[1] = LOC3::YELLOW;

  auto& att     = pointlight.attenuation;
  att.constant  = 1.0f;
  att.linear    = 0.93f;
  att.quadratic = 0.46f;

  return eid;
}

} // namespace boomhs

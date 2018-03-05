#include <boomhs/item_factory.hpp>
#include <boomhs/components.hpp>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>
#include <stlw/random.hpp>

using namespace opengl;

namespace boomhs
{

EntityID
ItemFactory::create_torch(EntityRegistry &registry, stlw::float_generator &rng, TextureTable const& ttable)
{
  auto eid = registry.create();
  registry.assign<Torch>(eid);

  auto &isv = registry.assign<IsVisible>(eid);
  isv.value = true;

  auto &pointlight = registry.assign<PointLight>(eid);
  pointlight.light.diffuse = LOC::YELLOW;

  auto &flicker = registry.assign<LightFlicker>(eid);
  flicker.base_speed = 1.0f;
  flicker.current_speed = flicker.base_speed;

  flicker.colors[0] = LOC::RED;
  flicker.colors[1] = LOC::YELLOW;

  auto &att = pointlight.attenuation;
  att.constant = 1.0f;
  att.linear = 0.93f;
  att.quadratic = 0.46f;

  // leave for caller to assign
  registry.assign<Transform>(eid);

  auto &mesh = registry.assign<MeshRenderable>(eid);
  mesh.name = "O";

  auto &tr = registry.assign<TextureRenderable>(eid);
  auto texture_o = ttable.find("Lava");
  assert(texture_o);
  tr.texture_info = *texture_o;

  auto &sn = registry.assign<ShaderName>(eid);
  sn.value = "light_texture";

  return eid;
}

} // ns boomhs

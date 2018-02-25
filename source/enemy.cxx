#include <boomhs/enemy.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/tile.hpp>

namespace boomhs
{

void
Enemy::load_new(EntityRegistry &registry, char const* name, TilePosition const& tpos)
{
  auto eid = registry.create();

  // Enemies get a mesh
  auto &meshc = registry.assign<MeshRenderable>(eid);
  meshc.name = name;

  // shader
  auto &sn = registry.assign<ShaderName>(eid);
  sn.value = "3d_pos_normal_color";

  // transform
  auto &transform = registry.assign<Transform>(eid);
  transform.translation = glm::vec3{tpos.x, 0.5, tpos.y};

  // enemy TAG
  registry.assign<EnemyData>(eid);

  // visible
  auto &isv = registry.assign<IsVisible>(eid);
  isv.value = true;
}

} // ns boomhs

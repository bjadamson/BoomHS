#pragma once
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>

#include <boomhs/enemy.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/types.hpp>
#include <boomhs/tile.hpp>

#include <cassert>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

namespace boomhs
{

class EnttLookup
{
  uint32_t eid_ = UINT32_MAX;
  EntityRegistry &registry_;
public:
  explicit EnttLookup(uint32_t const eid, EntityRegistry &registry)
    : eid_(eid)
    , registry_(registry)
  {
  }

  template<typename T>
  T&
  lookup()
  {
    assert(eid_ != UINT32_MAX);
    assert(registry_.has<T>(eid_));
    return registry_.get<T>(eid_);
  }

  template<typename T>
  T const&
  lookup() const
  {
    assert(eid_ != UINT32_MAX);
    assert(registry_.has<T>(eid_));
    return registry_.get<T>(eid_);
  }

  void
  set_eid(uint32_t const eid)
  {
    eid_ = eid;
  }
};

struct TargetSelector
{
};

struct StairInfo
{
  TilePosition tile_position;
  TilePosition exit_position;
};

struct Player
{
  TilePosition tile_position;
};

struct ShaderName
{
  std::string value;
};

struct CubeRenderable
{
};

struct MeshRenderable
{
  std::string name;
};

struct SkyboxRenderable
{
};

struct TextureRenderable
{
  opengl::TextureInfo texture_info;
};

template<typename ...C>
auto
find_all_entities_with_component(EntityRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;

  std::vector<uint32_t> entities;
  auto const view = registry.view<C...>();
  for (auto const e : view) {
    entities.emplace_back(e);
  }
  return entities;
}

inline auto
find_enemies(EntityRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;
  return find_all_entities_with_component<Enemy, Transform>(registry);
}

inline auto
find_materials(EntityRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;
  return find_all_entities_with_component<Material, Transform>(registry);
}

inline auto
find_pointlights(EntityRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;

  return find_all_entities_with_component<PointLight, Transform>(registry);
}

inline auto
find_stairs(EntityRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;

  return find_all_entities_with_component<StairInfo>(registry);
}

class TileGrid;
std::vector<uint32_t>
find_stairs_withtype(EntityRegistry &, TileGrid const&, TileType const);

inline auto
find_upstairs(EntityRegistry &registry, TileGrid const& tgrid)
{
  return find_stairs_withtype(registry, tgrid, TileType::STAIR_UP);
}

inline auto
find_downstairs(EntityRegistry &registry, TileGrid const& tgrid)
{
  return find_stairs_withtype(registry, tgrid, TileType::STAIR_DOWN);
}

inline uint32_t
find_player(EntityRegistry &registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<Player>().size());

  auto view = registry.view<Player, Transform>();
  std::optional<uint32_t> entity{std::nullopt};
  for (auto const e : view) {
    // This assert ensures this loop only runs once.
    assert(std::nullopt == entity);
    entity = e;
  }
  assert(std::nullopt != entity);
  return *entity;
}

} // ns boomhs

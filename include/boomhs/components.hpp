#pragma once
#include <glm/glm.hpp>
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>
#include <boomhs/types.hpp>
#include <boomhs/tile.hpp>

#include <entt/entt.hpp>
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
  entt::DefaultRegistry &registry_;
public:
  explicit EnttLookup(uint32_t const eid, entt::DefaultRegistry &registry)
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

struct RiverWiggle
{
  float speed;
  float z_jiggle;

  glm::vec3 position;
};

struct RiverInfo
{
  glm::vec3 left, right, top, bottom;

  std::vector<RiverWiggle> wiggles;
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
find_all_entities_with_component(entt::DefaultRegistry &registry)
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
find_materials(entt::DefaultRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;
  return find_all_entities_with_component<Material, Transform>(registry);
}

inline auto
find_pointlights(entt::DefaultRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;

  return find_all_entities_with_component<PointLight, Transform>(registry);
}

inline auto
find_rivers(entt::DefaultRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;

  return find_all_entities_with_component<RiverInfo>(registry);
}

inline auto
find_stairs(entt::DefaultRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;

  return find_all_entities_with_component<StairInfo>(registry);
}

class TileMap;
std::vector<uint32_t>
find_stairs_withtype(entt::DefaultRegistry &, TileMap const&, TileType const);

inline auto
find_upstairs(entt::DefaultRegistry &registry, TileMap const& tmap)
{
  return find_stairs_withtype(registry, tmap, TileType::STAIR_UP);
}

inline auto
find_downstairs(entt::DefaultRegistry &registry, TileMap const& tmap)
{
  return find_stairs_withtype(registry, tmap, TileType::STAIR_DOWN);
}

inline uint32_t
find_player(entt::DefaultRegistry &registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<Player>().size());

  auto view = registry.view<Player, Transform>();
  stlw::optional<uint32_t> entity{stlw::none};
  for (auto const e : view) {
    // This assert ensures this loop only runs once.
    assert(stlw::none == entity);
    entity = e;
  }
  assert(stlw::none != entity);
  return *entity;
}

} // ns boomhs

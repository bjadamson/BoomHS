#pragma once
#include <glm/glm.hpp>
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>
#include <boomhs/types.hpp>

#include <entt/entt.hpp>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace boomhs
{

struct EnttLookup
{
  std::uint32_t eid_;
  entt::DefaultRegistry &registry_;
public:
  explicit EnttLookup(std::uint32_t const eid, entt::DefaultRegistry &registry)
    : eid_(eid)
    , registry_(registry)
  {
  }

  template<typename T>
  T&
  lookup() { return registry_.get<T>(eid_); }

  template<typename T>
  T const&
  lookup() const { return registry_.get<T>(eid_); }

  void
  set_eid(std::uint32_t const eid)
  {
    eid_ = eid;
  }
};

struct Player
{
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

  std::vector<std::uint32_t> entities;
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

inline std::uint32_t
find_player(entt::DefaultRegistry &registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<Player>().size());

  auto view = registry.view<Player, Transform>();
  boost::optional<uint32_t> entity{boost::none};
  for (auto const e : view) {
    // This assert ensures this loop only runs once.
    assert(boost::none == entity);
    entity = e;
  }
  assert(boost::none != entity);
  return *entity;
}

} // ns boomhs

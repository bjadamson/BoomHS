#pragma once
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>

#include <boomhs/entity.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/tile.hpp>
#include <boomhs/types.hpp>

#include <cassert>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

namespace boomhs
{

struct AxisAlignedBoundingBox
{
  glm::vec3 bounds[2];
};

using AABoundingBox = AxisAlignedBoundingBox;

struct Name
{
  static constexpr char const* DEFAULT = "unnamed";
  std::string                  value   = Name::DEFAULT;
};

struct Selectable
{
  bool selected = false;
};

struct IsVisible
{
  bool value = false;
};

struct Torch
{
  opengl::Attenuation default_attenuation{1.0f, 0.93f, 0.46f};
};

struct LightFlicker
{
  float base_speed    = 0.0f;
  float current_speed = 0.0f;

  std::array<opengl::Color, 2> colors;
};

struct JunkEntityFromFILE
{
  GLenum draw_mode = GL_TRIANGLES;
};

struct StairInfo
{
  TilePosition tile_position;
  TilePosition exit_position;
};

struct ShaderName
{
  std::string value;
};

struct CubeRenderable
{
  GLenum mode = GL_TRIANGLES;
};

struct MeshRenderable
{
  std::string name;
};

struct TextureRenderable
{
  opengl::TextureInfo* texture_info = nullptr;
};

template <typename... C>
auto
find_all_entities_with_component(EntityRegistry& registry)
{
  using namespace boomhs;
  using namespace opengl;

  std::vector<EntityID> entities;
  auto const            view = registry.view<C...>();
  for (auto const e : view) {
    entities.emplace_back(e);
  }
  return entities;
}

inline auto
all_nearby_entities(glm::vec3 const& pos, float const max_distance, EntityRegistry& registry)
{
  using namespace boomhs;
  using namespace opengl;

  std::vector<EntityID> entities;
  auto const            view = registry.view<Transform>();
  for (auto const e : view) {
    auto& transform = registry.get<Transform>(e);
    if (glm::distance(transform.translation, pos) <= max_distance) {
      entities.emplace_back(e);
    }
  }
  return entities;
}

inline auto
find_enemies(EntityRegistry& registry)
{
  using namespace boomhs;
  using namespace opengl;
  return find_all_entities_with_component<NPCData>(registry);
}

inline auto
find_materials(EntityRegistry& registry)
{
  using namespace boomhs;
  using namespace opengl;
  return find_all_entities_with_component<Material>(registry);
}

inline auto
find_pointlights(EntityRegistry& registry)
{
  using namespace boomhs;
  using namespace opengl;

  return find_all_entities_with_component<PointLight>(registry);
}

inline auto
find_stairs(EntityRegistry& registry)
{
  using namespace boomhs;
  using namespace opengl;

  return find_all_entities_with_component<StairInfo>(registry);
}

class TileGrid;
std::vector<EntityID>
find_stairs_withtype(EntityRegistry&, TileGrid const&, TileType const);

inline auto
find_upstairs(EntityRegistry& registry, TileGrid const& tgrid)
{
  return find_stairs_withtype(registry, tgrid, TileType::STAIR_UP);
}

inline auto
find_downstairs(EntityRegistry& registry, TileGrid const& tgrid)
{
  return find_stairs_withtype(registry, tgrid, TileType::STAIR_DOWN);
}

} // namespace boomhs

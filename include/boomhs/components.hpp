#pragma once
#include <boomhs/colors.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/texture.hpp>

#include <boomhs/entity.hpp>
#include <boomhs/lighting.hpp>
#include <boomhs/math.hpp>
#include <boomhs/transform.hpp>

#include <common/log.hpp>

#include <cassert>
#include <string>
#include <vector>

namespace boomhs
{

struct HealthPoints
{
  int current, max;
};

struct WidthHeightLength
{
  float const width;
  float const height;
  float const length;
};

// AxisAlignedBoundingBox
struct AABoundingBox
{
  Cube cube;

  opengl::DrawInfo draw_info;

  // ctor
  AABoundingBox(glm::vec3 const&, glm::vec3 const&, opengl::DrawInfo&&);

  static void add_to_entity(common::Logger&, opengl::ShaderPrograms&, EntityID, EntityRegistry&,
                            glm::vec3 const&, glm::vec3 const&);
};

struct OrbitalBody
{
  glm::vec3 radius;

  // Initial orbit offset for the body.
  float offset = 0.0f;

  explicit OrbitalBody(glm::vec3 const&, float);
};

struct Name
{
  std::string value;

  Name() = default;
  COPY_DEFAULT(Name);
  MOVE_DEFAULT(Name);

  Name(char const* v)
      : value(v)
  {
  }
  Name(std::string const& v)
      : value(v)
  {
  }
  Name(std::string&& v)
      : value(MOVE(v))
  {
  }
  bool empty() const { return value.empty(); }
};

inline bool
operator==(Name const& a, Name const& b)
{
  return a.value == b.value;
}

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
  Attenuation default_attenuation{1.0f, 0.93f, 0.46f};
};

struct LightFlicker
{
  float base_speed    = 0.0f;
  float current_speed = 0.0f;

  std::array<Color, 2> colors;
};

struct JunkEntityFromFILE
{
};

struct ShaderName
{
  std::string value;
};

struct CubeRenderable
{
  glm::vec3 min, max;
};

struct MeshRenderable
{
  std::string name;

  MeshRenderable(std::string&& n)
      : name(MOVE(n))
  {
  }
  MeshRenderable(std::string const& n)
      : name(n)
  {
  }
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
find_pointlights(EntityRegistry& registry)
{
  return find_all_entities_with_component<PointLight>(registry);
}

inline auto
find_orbital_bodies(EntityRegistry& registry)
{
  return find_all_entities_with_component<OrbitalBody>(registry);
}

} // namespace boomhs

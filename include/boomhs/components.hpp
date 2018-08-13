#pragma once
#include <boomhs/lighting.hpp>
#include <opengl/colors.hpp>
#include <opengl/texture.hpp>

#include <boomhs/entity.hpp>

#include <cassert>
#include <string>
#include <vector>

namespace boomhs
{

struct HealthPoints
{
  int current, max;
};

struct Transform
{
  glm::vec3 translation{0.0f, 0.0f, 0.0f};
  glm::quat rotation;
  glm::vec3 scale = glm::vec3{1.0f, 1.0f, 1.0f};

  glm::mat4 model_matrix() const
  {
    return math::calculate_modelmatrix(translation, rotation, scale);
  }

  void rotate_degrees(float const degrees, glm::vec3 const& axis)
  {
    rotation = glm::angleAxis(glm::radians(degrees), axis) * rotation;
  }
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
  glm::vec3 min;
  glm::vec3 max;

  // ctor
  AABoundingBox();
  AABoundingBox(glm::vec3 const&, glm::vec3 const&);

  // methods
  glm::vec3 dimensions() const;
  glm::vec3 center() const;
  glm::vec3 half_widths() const;

  glm::vec3 scaled_min(Transform const&) const;
  glm::vec3 scaled_max(Transform const&) const;

  static void add_to_entity(EntityID, EntityRegistry&, glm::vec3 const&, glm::vec3 const&);
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

  std::array<opengl::Color, 2> colors;
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
find_pointlights(EntityRegistry& registry)
{
  using namespace boomhs;
  using namespace opengl;

  return find_all_entities_with_component<PointLight>(registry);
}

} // namespace boomhs

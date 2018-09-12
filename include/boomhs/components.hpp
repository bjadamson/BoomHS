#pragma once
#include <boomhs/colors.hpp>

#include <boomhs/entity.hpp>
#include <boomhs/lighting.hpp>

#include <common/log.hpp>

#include <cassert>
#include <string>

namespace opengl
{
struct TextureInfo;
} // namespace opengl

namespace boomhs
{

// Attached to other entities to keep two entities the same relative distance from one another over
// time.
//
// This component is used to automatically update an entity (passed into the constructor).
struct FollowTransform
{
  EntityID  target_eid;
  glm::vec3 target_offset;

  explicit FollowTransform(EntityID);
};

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

  explicit Name(char const* v)
      : value(v)
  {
  }
  explicit Name(std::string const& v)
      : value(v)
  {
  }
  explicit Name(std::string&& v)
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

struct ShaderName
{
  std::string value;

  explicit ShaderName(char const* v)
      : value(v)
  {
  }
  explicit ShaderName(std::string const& v)
      : value(v)
  {
  }
  explicit ShaderName(std::string&& v)
      : value(MOVE(v))
  {
  }
};

struct Selectable
{
  bool selected = false;
};

// Dictates whether an Entity will be considered for rendering or not.
//
// A Entity with this component attached will be considered for rendering by the EntityRenderer.
//
// If the field "hidden" is set to true, the Entity will not be considered for rendering any
// further.
struct IsRenderable
{
  bool hidden = false;

  IsRenderable() = default;
  explicit IsRenderable(bool const h)
      : hidden(h)
  {
  }
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

inline auto
find_orbital_bodies(EntityRegistry& registry)
{
  return find_all_entities_with_component<OrbitalBody>(registry);
}

} // namespace boomhs

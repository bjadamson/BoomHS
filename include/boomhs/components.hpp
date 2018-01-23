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

template<typename C>
boost::optional<std::uint32_t>
find_entity_with_component(std::uint32_t const entity, entt::DefaultRegistry &registry)
{
  boost::optional<std::uint32_t> result_o = boost::none;
  auto const view = registry.view<C>();
  for (auto const e : view) {
    if (entity == e) {
      result_o = e;
      continue;
    }
    else if (boost::none != result_o) {
      assert(entity != e);
    }
  }
  return result_o;
}

inline std::vector<uint32_t>
find_materials(entt::DefaultRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;

  std::vector<std::uint32_t> materials;
  auto view = registry.view<Material, Transform>();
  for (auto const entity : view) {
    materials.emplace_back(entity);
  }
  //std::cerr << "found '" << materials.size() << "' materials\n";
  return materials;
}


inline std::vector<uint32_t>
find_pointlights(entt::DefaultRegistry &registry)
{
  using namespace boomhs;
  using namespace opengl;

  std::vector<std::uint32_t> point_lights;
  auto view = registry.view<PointLight, Transform>();
  for (auto const entity : view) {
    point_lights.emplace_back(entity);
  }
  //std::cerr << "found '" << point_lights.size() << "' point lights\n";
  return point_lights;
}

} // ns boomhs

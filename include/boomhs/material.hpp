#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <opengl/colors.hpp>
#include <common/type_macros.hpp>

#include <extlibs/glm.hpp>
#include <vector>

namespace boomhs
{

struct Material
{
  glm::vec3 ambient   = glm::vec3{1.0f};
  glm::vec3 diffuse   = glm::vec3{1.0f};
  glm::vec3 specular  = glm::vec3{1.0f};
  float     shininess = 1.0f;

  Material() = default;
  MOVE_DEFAULT(Material);
  COPY_DEFAULT(Material);

  Material(glm::vec3 const&, glm::vec3 const&, glm::vec3 const&, float);

  //
  // The alpha value for the colors is truncated.
  Material(opengl::Color const&, opengl::Color const&, opengl::Color const&, float const);
};

struct NameMaterial
{
  std::string const name;
  Material          material;
};

struct MaterialTable
{
  std::vector<NameMaterial> data_;

public:
  MaterialTable() = default;
  MOVE_CONSTRUCTIBLE_ONLY(MaterialTable);
  INDEX_OPERATOR_FNS(data_);
  BEGIN_END_FORWARD_FNS(data_);

  void add(NameMaterial&&);

  Material&       find(std::string const&);
  Material const& find(std::string const&) const;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
std::vector<EntityID>
find_materials(EntityRegistry&);

} // namespace boomhs

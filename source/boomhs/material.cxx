#include <boomhs/material.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// Material
Material::Material(glm::vec3 const& amb, glm::vec3 const& diff, glm::vec3 const& spec,
                   float const shiny)
    : ambient(amb)
    , diffuse(diff)
    , specular(spec)
    , shininess(shiny)
{
}

Material::Material(ColorRGB const& amb, ColorRGB const& diff, ColorRGB const& spec,
                   float const shiny)
    : Material(amb.vec3(), diff.vec3(), spec.vec3(), shiny)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// MaterialTable
void
MaterialTable::add(NameMaterial&& nm)
{
  data_.emplace_back(MOVE(nm));
}

#define FIND_IMPL(MATERIAL_NAME, BEGIN, END)                                                       \
  auto const cmp = [&MATERIAL_NAME](NameMaterial const& nm) { return nm.name == MATERIAL_NAME; };  \
                                                                                                   \
  auto const it = std::find_if(BEGIN, END, cmp);                                                   \
  assert(it != END);                                                                               \
  return it->material;

Material&
MaterialTable::find(std::string const& material_name)
{
  FIND_IMPL(material_name, begin(), end());
}

Material const&
MaterialTable::find(std::string const& material_name) const
{
  FIND_IMPL(material_name, cbegin(), cend());
}
#undef FIND_IMPL

EntityArray
find_materials(EntityRegistry& registry)
{
  return find_all_entities_with_component<Material>(registry);
}

} // namespace boomhs

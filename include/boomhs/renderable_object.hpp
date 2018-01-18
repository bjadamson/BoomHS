#pragma once
#include <boomhs/types.hpp>
#include <boomhs/world_object.hpp>
#include <opengl/colors.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{

struct Material
{
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  float shininess;

  Material(glm::vec3 const& amb, glm::vec3 const& diff, glm::vec3 const& spec, float const shiny)
    : ambient(amb)
    , diffuse(diff)
    , specular(spec)
    , shininess(shiny)
  {
  }

  //
  // The alpha value for the colors is truncated.
  Material(opengl::Color const& amb, opengl::Color const& diff, opengl::Color const& spec,
      float const shiny)
    : Material(amb.rgb(), diff.rgb(), spec.rgb(), shiny)
  {
  }
};

struct RenderableObject
{
  WorldObject world_object;
  Material material;

  explicit RenderableObject(WorldObject &&wo, Material &&mat)
    : world_object(MOVE(wo))
    , material(MOVE(mat))
  {
  }

  MOVE_CONSTRUCTIBLE_ONLY(RenderableObject);
};

} // ns boomhs

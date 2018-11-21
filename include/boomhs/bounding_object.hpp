#pragma once
#include <boomhs/math.hpp>

#include <opengl/draw_info.hpp>

namespace boomhs
{

// AxisAlignedBoundingBox
struct AABoundingBox
{
  Cube cube;

  opengl::DrawInfo draw_info;

  // ctor
  AABoundingBox(glm::vec3 const&, glm::vec3 const&, opengl::DrawInfo&&);

  static AABoundingBox& add_to_entity(common::Logger&, opengl::ShaderPrograms&, EntityID,
                                      EntityRegistry&, glm::vec3 const&, glm::vec3 const&);
};

} // namespace boomhs

#pragma once
#include <boomhs/colors.hpp>

#include <opengl/shapes.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// Arrow
struct ArrowTemplate
{
  boomhs::Color const& color;

  glm::vec3 start;
  glm::vec3 end;

  float const tip_length_factor = 4.0f;
};

class VertexFactory
{
  VertexFactory() = delete;
public:

  // Arrows
  using ArrowVertices                         = std::array<float, 42>;
  using ArrowIndices                          = std::array<GLuint, 6>;
  static constexpr ArrowIndices ARROW_INDICES = {{0, 1, 2, 3, 4, 5}};

  static ArrowVertices build(ArrowTemplate const&);
};

} // namespace boomhs

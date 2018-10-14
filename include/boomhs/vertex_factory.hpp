#pragma once
#include <boomhs/colors.hpp>

#include <opengl/shapes.hpp>
#include <extlibs/glm.hpp>

using vertices_t = float;
using indices_t  = unsigned int;

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

struct LineTemplate
{
  glm::vec3 start;
  glm::vec3 end;
};

class VertexFactory
{
  VertexFactory() = delete;
public:

  // Arrow
  using ArrowVertices                         = std::array<vertices_t, 42>;
  using ArrowIndices                          = std::array<indices_t,  6>;
  static constexpr ArrowIndices ARROW_INDICES = {{0, 1, 2, 3, 4, 5}};
  static ArrowVertices build(ArrowTemplate const&);

  // Line
  using LineVertices                        = std::array<vertices_t, 6>;
  using LineIndices                         = std::array<indices_t,  2>;
  static constexpr LineIndices LINE_INDICES = {{0, 1}};
  static LineVertices build(LineTemplate const&);
};

} // namespace boomhs

#pragma once
#include <array>
#include <vector>

namespace boomhs
{

using vertices_t = float;
using indices_t  = unsigned int;

using CubeVertices         = std::array<vertices_t, 24>;
using CubeIndices          = std::array<indices_t, 36>;
using CubeWireframeIndices = std::array<indices_t, 24>;

using RectangleUvVertices  = std::array<vertices_t, 30>;

using RectangleIndices     = std::array<indices_t, 6>;
using RectangleLineIndices = std::array<indices_t, 4>;

using VerticsArray = std::vector<vertices_t>;

struct RectBuffer
{
  VerticsArray vertices;
};

struct RectLineBuffer
{
  std::array<vertices_t, 12> vertices;
};

} // namespace boomhs

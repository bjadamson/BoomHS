#pragma once
#include <common/algorithm.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glm.hpp>

#include <array>
#include <string>
#include <sstream>

namespace boomhs
{

// Template for defining a polygon that has a static number of vertices.
template <typename V, size_t N>
struct PolygonVertices
{
  std::array<V, N> vertices;

  template <typename ...P>
  constexpr PolygonVertices(P&&... points) noexcept
      : vertices(common::make_array<V>(FORWARD(points)))
  {
    static_assert(N > 1, "Polygon should have more than 1 vertex.");

    auto constexpr NUM_POINTS_PASSED_TO_CTOR = sizeof...(P);
    static_assert(N == NUM_POINTS_PASSED_TO_CTOR, "Not enough points provided to constructor.");
  }

  DEFINE_ARRAY_LIKE_WRAPPER_FNS(vertices);

  auto to_string() const
  {
    std::stringstream ss;
    ss << "{";

    FOR(i, vertices.size()) {
      if (i > 0) { ss << ", "; }

      ss << glm::to_string(vertices[i]);
    }
    ss << "}";
    return ss.str();
  }
};

} // namspace boomhs

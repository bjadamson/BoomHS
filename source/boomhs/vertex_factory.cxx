#include <boomhs/vertex_factory.hpp>
#include <boomhs/math.hpp>

using namespace boomhs;

namespace
{

struct ArrowEndpoints
{
  glm::vec3 start;
  glm::vec3 end;
};

auto
calculate_arrow_endpoints(ArrowTemplate const& arrow)
{
  auto const adjust_if_zero = [=](glm::vec3 const& v) {
    auto constexpr EPSILON = std::numeric_limits<float>::epsilon();
    auto constexpr EPSILON_VEC = glm::vec3{EPSILON, EPSILON, EPSILON};
    bool const is_zero = glm::all(glm::epsilonEqual(v, math::constants::ZERO, EPSILON));
    return is_zero ? EPSILON_VEC : v;
  };

  // Normalizing a zero vector is undefined. Therefore if the user passes us a zero vector, since
  // we are creating an arrow, pretend the point is EPSILON away from the true origin (so
  // normalizing the crossproduct doesn't yield vector's with NaN for their components).
  auto const A = adjust_if_zero(arrow.start);
  auto const B = adjust_if_zero(arrow.end);

  glm::vec3 const v = A - B;
  glm::vec3 const rev = -v;

  glm::vec3 const cross1 = glm::normalize(glm::cross(A, B));
  glm::vec3 const cross2 = glm::normalize(glm::cross(B, A));

  glm::vec3 const vp1 = glm::normalize(rev + cross1);
  glm::vec3 const vp2 = glm::normalize(rev + cross2) ;

  float const factor = arrow.tip_length_factor;
  glm::vec3 const p1 = B - (vp1 / factor);
  glm::vec3 const p2 = B - (vp2 / factor);

  return ArrowEndpoints{p1, p2};
}

} // ns anon

namespace boomhs
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Arrow
VertexFactory::ArrowVertices
VertexFactory::build(ArrowTemplate const& arrow)
{
  auto endpoints = calculate_arrow_endpoints(arrow);
  auto const& p1 = endpoints.start, p2 = endpoints.end;

  auto const& color = arrow.color;
  auto const& start = arrow.start;
  auto const& end   = arrow.end;
#define COLOR color.r(), color.g(), color.b(), color.a()
#define START start.x,   start.y,   start.z
#define END   end.x,     end.y,     end.z

#define P1 p1.x, p1.y, p1.z
#define P2 p2.x, p2.y, p2.z
  return common::make_array<float>(
      // START -> END
      START, COLOR,
      END, COLOR,

      // END -> P1
      END, COLOR,
      P1, COLOR,

      // END -> P2
      END, COLOR,
      P2, COLOR
      );
#undef COLOR
#undef START
#undef END

#undef P1
#undef P2
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Line
VertexFactory::LineVertices
VertexFactory::build(LineTemplate const& line)
{
  auto const& start = line.start;
  auto const& end   = line.end;

  return common::make_array<float>(
      start.x, start.y, start.z,
      end.x,   end.y,   end.z);
}

VertexFactory::GridVerticesIndices
VertexFactory::build(GridTemplate const& grid)
{
  auto const& color      = grid.color;
  auto const& dimensions = grid.dimensions;

  VertexFactory::GridVerticesIndices result;
  auto& v = result.vertices;
  auto& i = result.indices;

  size_t count = 0u;
  auto const add_point = [&](glm::vec3 const& pos) {
    v.emplace_back(pos.x);
    v.emplace_back(pos.y);
    v.emplace_back(pos.z);

    v.emplace_back(color.r());
    v.emplace_back(color.g());
    v.emplace_back(color.b());
    v.emplace_back(color.a());

    i.emplace_back(count++);
  };

  auto const add_line = [&add_point](glm::vec3 const& p0, glm::vec3 const& p1) {
    add_point(p0);
    add_point(p1);
  };

  auto const add_lines_between = [&](auto const& pos) {
    VEC3 const P0{pos.x + 0, pos.y, pos.z + 0};
    VEC3 const P1{pos.x + 1, pos.y, pos.z + 0};
    VEC3 const P2{pos.x + 1, pos.y, pos.z + 1};
    VEC3 const P3{pos.x + 0, pos.y, pos.z + 1};

    add_line(P0, P1);
    add_line(P3, P2);
    add_line(P2, P1);
    add_line(P3, P0);
  };

  auto const loop_over_xz_plane = [&](auto const y) {
    FOR(x, dimensions.x) {
      FOR(z, dimensions.z) {
        add_lines_between(VEC3{x, y, z});
      }
    }
  };

  FOR(y, dimensions.y) {
    loop_over_xz_plane(y);
  }

  return result;
}

} // namespace boomhs

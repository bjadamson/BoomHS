#pragma once
#include <boomhs/polygon.hpp>
#include <boomhs/transform.hpp>

// Implementation of various algorithms dealing with 2-dimensional rectangles.
//
// NOTE:
// Implementation of these algorithms is left as non-member functions on a central rectangle "Type"
// is intentional to keep algorithms de-coupled from vertex-type, to allow re-use of these
// algorithms among various Rectangle types.
namespace boomhs::rectangle
{

// The vertices of a rectangle.
template <typename V>
struct RectVertices {
  static auto constexpr num_vertices = 4;
  using vertex_type                  = V;
  using value_type                   = typename V::value_type;
  using array_type                   = PolygonVertices<V, num_vertices>;

  // fields
  value_type left, top;
  value_type right, bottom;
};

} // namespace boomhs::rectangle

// Compute various information from/about rectangles.
//
// NOTE: Most of these functions tend to have a member function defined.
namespace boomhs::rectangle
{

// Compute the vertex (object-space).
template <typename V>
auto constexpr
left_top(rectangle::RectVertices<V> const& rv)
{
  return V{rv.left, rv.top};
}

// Compute the vertex (object-space).
template <typename V>
auto constexpr
left_bottom(rectangle::RectVertices<V> const& rv)
{
  return V{rv.left, rv.bottom};
}

// Compute the vertex (object-space).
template <typename V>
auto constexpr
right_top(rectangle::RectVertices<V> const& rv)
{
  return V{rv.right, rv.top};
}

// Compute the vertex (object-space).
template <typename V>
auto constexpr
right_bottom(rectangle::RectVertices<V> const& rv)
{
  return V{rv.right, rv.bottom};
}

// Function to map a vertex index to a vertex point.
//
// NOTE: This function is the single source of vertex-ordering for rectangles, other functions use
// (or should) use this function so vertex-index to vertex-point ordering remains consistent across
// the project.
template <typename V>
auto constexpr
rect_vertex_index_to_point(rectangle::RectVertices<V> const& rv, size_t const index)
{
  // Sanity check, ensure that a valid index has been passed in.
  assert(index <= rectangle::RectVertices<V>::num_vertices);

  // Map each index to a vertex.
  switch (index) {
    case 0:
      return left_top(rv);
    case 1:
      return left_bottom(rv);
    case 2:
      return right_bottom(rv);
    case 3:
      return right_top(rv);
    default:
      break;
  }

  /* INVALID to index this far into a rectangle. Rectangle only has 4 points. */
  std::abort();

  /* Satisfy Compiler */
  return V{};
}

// Yield the vertex points (counter-clockwise order) in object-space.
template<typename V>
auto constexpr
vertex_points(rectangle::RectVertices<V> const& rv)
{
  // Collect the points into an array.
  auto const p0 = rect_vertex_index_to_point(rv, 0);
  auto const p1 = rect_vertex_index_to_point(rv, 1);
  auto const p2 = rect_vertex_index_to_point(rv, 2);
  auto const p3 = rect_vertex_index_to_point(rv, 3);

  using ArrayType = typename rectangle::RectVertices<V>::array_type;
  return ArrayType{p0, p1, p2, p3};
}

// Move the rectangle by (x, y)
template <typename V>
void
move(rectangle::RectVertices<V>& rv,
    typename rectangle::RectVertices<V>::value_type const& x,
    typename rectangle::RectVertices<V>::value_type const& y)
{
  rv.left  += x;
  rv.right += x;

  rv.top    += y;
  rv.bottom += y;
}

template <typename V>
void move(rectangle::RectVertices<V> const& rv, V const& v)
{
  move(rv, v.x, v.y);
}

// Compute the with of the rectangle
template <typename V>
auto constexpr
width(rectangle::RectVertices<V> const& rv)
{
  auto const l = rv.left;
  auto const r = rv.right;

  return std::abs(r - l);
}

template <typename V>
auto constexpr
height(rectangle::RectVertices<V> const& rv)
{
  auto const b = rv.bottom;
  auto const t = rv.top;

  return std::abs(b - t);
}

template <typename V>
auto constexpr
size(rectangle::RectVertices<V> const& rv)
{
  auto const w = width(rv);
  auto const h = height(rv);
  return V{w, h};
}

template <typename V>
auto constexpr
half_size(rectangle::RectVertices<V> const& rv)
{
  return size(rv) / 2;
}

template <typename V>
auto constexpr
half_width(rectangle::RectVertices<V> const& rv)
{
  return width(rv) / 2;
}

template <typename V>
auto constexpr
half_height(rectangle::RectVertices<V> const& rv)
{
  return height(rv) / 2;
}

template <typename V>
auto constexpr
center(rectangle::RectVertices<V> const& rv)
{
  auto const lt = left_top(rv);
  auto const hs = half_size(rv);
  return lt + hs;
}

template <typename V>
auto constexpr
center_left(rectangle::RectVertices<V> const& rv)
{
  auto const l = rv.left;
  auto const y = center(rv).y;
  return V{l, y};
}

template <typename V>
auto constexpr
center_right(rectangle::RectVertices<V> const& rv)
{
  auto const r = rv.right;
  auto const y = center(rv).y;
  return V{r, y};
}

template <typename V>
auto constexpr
center_top(rectangle::RectVertices<V> const& rv)
{
  auto const x = center(rv).x;
  auto const t = rv.top;
  return V{x, t};
}

template <typename V>
auto constexpr
center_bottom(rectangle::RectVertices<V> const& rv)
{
  auto const x = center(rv).x;
  auto const b = rv.bottom;
  return V{x, b};
}

// Compute a pair containing:
//
// (first) element:
//   The Rectangle's center.
//
// (second) element:
//   The rectangle's half-size.
template <typename V>
auto
center_and_scaled_half_size(rectangle::RectVertices<V> const& rv, Transform2D const& tr)
{
  auto const& s   = tr.scale;
  auto const hs   = half_size(rv);
  auto const s_hs = glm::vec2{
    s.x * hs.x,
    s.y * hs.y
  };

  auto const c = center(rv);
  return PAIR(c, s_hs);
}

// Compute the left-top vertex of the rectangle, after the provided Transform's scaling has
// been applied.
template <typename V>
V constexpr
left_top_scaled(rectangle::RectVertices<V> const& rv, Transform2D const& tr)
{
  auto const [c, shs] = center_and_scaled_half_size(rv, tr);

  V p;
  p.x = c.x - shs.x;
  p.y = c.y - shs.y;
  return p;
}

// Compute the right-top vertex of the rectangle, after the provided Transform's scaling has
// been applied.
template <typename V>
V constexpr
right_top_scaled(rectangle::RectVertices<V> const& rv, Transform2D const& tr)
{
  auto const [c, shs] = center_and_scaled_half_size(rv, tr);

  V p;
  p.x = c.x + shs.x;
  p.y = c.y - shs.y;
  return p;
}

// Compute the left-bottom vertex of the rectangle, after the provided Transform's scaling has
// been applied.
template <typename V>
V constexpr
left_bottom_scaled(rectangle::RectVertices<V> const& rv, Transform2D const& tr)
{
  auto const [c, shs] = center_and_scaled_half_size(rv, tr);

  V p;
  p.x = c.x - shs.x;
  p.y = c.y + shs.y;
  return p;
}

// Compute the right-bottom vertex of the rectangle, after the provided Transform's scaling has
// been applied.
template <typename V>
V constexpr
right_bottom_scaled(rectangle::RectVertices<V> const& rv, Transform2D const& tr)
{
  auto const [c, shs] = center_and_scaled_half_size(rv, tr);

  V p;
  p.x = c.x + shs.x;
  p.y = c.y + shs.y;
  return p;
}

} // namespace boomhs::rectangle


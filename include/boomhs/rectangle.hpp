#pragma once
#include <boomhs/rectangle_algorithms.hpp>
#include <boomhs/transform.hpp>
#include <common/algorithm.hpp>
#include <common/type_macros.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glm.hpp>
#include <array>

// Define/implement various 2dimensional rectangle types.
//
// Algorithm's are seperated from the actual rectangle types to allow reuse.
//
// User's may directly call the functions in boomhs::rectangle if they wish, but the functionality
// has been re-exported as member functions on Rectangle objects.
namespace boomhs
{

// Template for defining 2D Rectangle types with a polymorphic Vertex type V.
//
// This class template maps the functions from rectangle_algorithm's namespace to member functions
// on a Rectangle type.
//
// This seperation of algorithm and object is intentional. The details of how the algorithm's are
// implemented is seperated from the "object-ness" of the rectangle itself.
template <typename V>
struct RectT : public rectangle::RectVertices<V>
{
  // Re-export the public defined types for RectVertices
  using vertex_type = typename rectangle::RectVertices<V>::vertex_type;
  using value_type  = typename rectangle::RectVertices<V>::value_type;
  using array_type  = typename rectangle::RectVertices<V>::array_type;

  // methods
  void move(value_type const& x, value_type const& y) { rectangle::move(*this, x, y); }
  void move(V const& v)                               { rectangle::move(*this, v); }

  auto constexpr left_top() const     { return rectangle::left_top(*this); }
  auto constexpr left_bottom() const  { return rectangle::left_bottom(*this); }
  auto constexpr right_top() const    { return rectangle::right_top(*this); }
  auto constexpr right_bottom() const { return rectangle::right_bottom(*this); }


  auto constexpr width() const     { return rectangle::width(*this); }
  auto constexpr height() const    { return rectangle::height(*this); }
  auto constexpr size() const      { return rectangle::size(*this); }
  auto constexpr size_vec4() const { return glm::vec4{left_top(), size()}; }

  auto constexpr half_size() const   { return rectangle::half_size(*this); }
  auto constexpr half_width() const  { return rectangle::half_width(*this); }
  auto constexpr half_height() const { return rectangle::half_height(*this); }

  auto constexpr center() const       { return rectangle::center(*this); }
  auto constexpr center_left() const  { return rectangle::center_left(*this); }
  auto constexpr center_right() const { return rectangle::center_right(*this); }

  auto constexpr center_top() const    { return rectangle::center_top(*this); }
  auto constexpr center_bottom() const { return rectangle::center_bottom(*this); }

  V constexpr left_top_scaled(Transform2D const& tr) const     { return rectangle::left_top_scaled(*this, tr); }
  V constexpr right_top_scaled(Transform2D const& tr) const    { return rectangle::right_top_scaled(*this, tr); }
  V constexpr left_bottom_scaled(Transform2D const& tr) const  { return rectangle::left_bottom_scaled(*this, tr); }
  V constexpr right_bottom_scaled(Transform2D const& tr) const { return rectangle::right_bottom_scaled(*this, tr); }

  // Map the vertex index to the vertex point.
  auto constexpr operator[](size_t const i) const
  {
    return rectangle::rect_vertex_index_to_point(*this, i);
  }

  // Yield the vertex points in object-space (counter-clockwise order).
  array_type constexpr points() const
  {
    return rectangle::vertex_points(*this);
  }
};

template <typename V>
constexpr RectT<V>
operator/(RectT<V> const& rect, typename RectT<V>::value_type const& d)
{
  auto constexpr left   = rect.left()   / d;
  auto constexpr top    = rect.top()    / d;
  auto constexpr right  = rect.right()  / d;
  auto constexpr bottom = rect.bottom() / d;
  return RectT{left, top, right, bottom};
}

// The "format" of printing one of these rectangles is uniform between rectangle types, but this
// uniformity (only having to write the body of this function once, code duplication) can only be
// captured by a macro.
#define DEFINE_RECT_TO_STRING_MEMBER_IMPL(FMT_IDENTIFIER)                                          \
std::string to_string() const                                                                      \
{                                                                                                  \
  return fmt::sprintf(                                                                             \
      "("#FMT_IDENTIFIER","#FMT_IDENTIFIER")"", ""("#FMT_IDENTIFIER","#FMT_IDENTIFIER") "          \
      "(w:"#FMT_IDENTIFIER",h:"#FMT_IDENTIFIER")",                                                 \
      left, top,                                                                                   \
      right, bottom,                                                                               \
      width(), height());                                                                              \
}

// Define a RectFloat with various helper constructors, that uses the Rectangle template defined
// above to share a common implementation with other rectangle types.
class RectFloat : public RectT<glm::vec2>
{
  float constexpr cast(int const v) const { return static_cast<float>(v); }
public:

  // ctor
  constexpr RectFloat(float const l, float const t, float const r, float const b)
      : RectT{l, t, r, b}
  {
  }

  constexpr RectFloat(glm::vec2 const& p0, glm::vec2 const& p1)
      : RectFloat(p0.x, p0.y, p1.x, p1.y)
  {
  }

  // conversion ctor
  constexpr RectFloat(int const l, int const t, int const r, int const b)
      : RectFloat(cast(l), cast(t), cast(r), cast(b))
  {
  }

  DEFINE_RECT_TO_STRING_MEMBER_IMPL(%f);
};

// Define a RectInt with various helper constructors, that uses the Rectangle template defined
// above to share a common implementation with other rectangle types.
struct RectInt : public RectT<glm::ivec2>
{
  constexpr RectInt(int const l, int const t, int const r, int const b)
      : RectT{l, t, r, b}
  {
  }

  constexpr RectInt(glm::ivec2 const& tl, glm::ivec2 const& br)
      : RectInt(tl.x, tl.y, br.x, br.y)
  {
  }

  auto constexpr
  float_rect() const { return RectFloat{left, top, right, bottom}; }

  static RectInt
  from_floats(float const l, float const t, float const r, float const b)
  {
    auto const cast = [](auto const& v) { return static_cast<int>(v); };
    glm::ivec2 const tl{cast(l), cast(t)};
    glm::ivec2 const br{cast(r), cast(b)};
    return RectInt{tl, br};
  }

  float constexpr float_left() const { return left; }
  float constexpr float_right() const { return right; }

  float constexpr float_top() const { return top; }
  float constexpr float_bottom() const { return bottom; }

  DEFINE_RECT_TO_STRING_MEMBER_IMPL(%i);
};

#undef DEFINE_RECT_TO_STRING_MEMBER_IMPL

} // namespace boomhs

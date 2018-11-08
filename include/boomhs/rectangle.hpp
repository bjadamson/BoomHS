#pragma once
#include <boomhs/transform.hpp>
#include <common/algorithm.hpp>
#include <common/type_macros.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glm.hpp>
#include <array>

namespace boomhs
{

template <typename V, size_t N>
struct PolygonVertices
{
  std::array<V, N> vertices;

  template <typename ...P>
  constexpr PolygonVertices(P&&... points) noexcept
      : vertices(common::make_array<V>(FORWARD(points)))
  {
    auto constexpr NUM_POINTS_PASSED_TO_CTOR = sizeof...(P);
    static_assert(N == NUM_POINTS_PASSED_TO_CTOR, "Not enough points provided.");
  }

  DEFINE_ARRAY_LIKE_WRAPPER_FNS(vertices);

  auto to_string() const
  {
    static_assert(N == 4, "Exactly 4 points required (for now).");
    return fmt::sprintf("{%s, %s, %s, %s}",
        glm::to_string(vertices[0]),
        glm::to_string(vertices[1]),
        glm::to_string(vertices[2]),
        glm::to_string(vertices[3]));
  }
};



template <typename V>
struct RectT
{
  static auto constexpr num_vertices = 4;
  using vertex_type                  = V;
  using value_type                   = typename V::value_type;
  using array_type                   = PolygonVertices<V, num_vertices>;

  // fields
  value_type left, top;
  value_type right, bottom;

  // methods
  auto constexpr left_top() const { return V{left, top}; }
  auto constexpr left_bottom() const { return V{left, bottom}; }

  auto constexpr right_top() const { return V{right, top}; }
  auto constexpr right_bottom() const { return V{right, bottom}; }

  auto constexpr width() const { return std::abs(right - left); }
  auto constexpr height() const { return std::abs(bottom - top); }
  auto constexpr size() const { return V{width(), height()}; }

  auto constexpr half_size() const { return size() / 2; }
  auto constexpr half_width() const { return width() / 2; }
  auto constexpr half_height() const { return height() / 2; }

  auto constexpr center() const { return left_top() + half_size(); }
  auto constexpr center_left() const { return V{left, center().y}; }
  auto constexpr center_right() const { return V{right, center().y}; }

  auto constexpr center_top() const { return V{center().x, top}; }
  auto constexpr center_bottom() const { return V{center().x, bottom}; }

  V constexpr scaled_left_top(Transform const& tr) const
  {
    auto const& s = tr.scale;
    auto const c  = center();
    auto const hs = half_size();

    auto p = V{left, top};
    p.x    = c.x - (s.x * hs.x);
    p.y    = c.y - (s.y * hs.y);
    return p;
  }

  V constexpr scaled_right_bottom(Transform const& tr) const
  {
    auto const& s = tr.scale;
    auto const c  = center();
    auto const hs = half_size();

    auto p = V{right, bottom};
    p.x    = c.x + (s.x * hs.x);
    p.y    = c.y + (s.y * hs.y);
    return p;
  }

  /*
  RectT<V> constexpr
  scaled_rect(Transform const& tr) const
  {
    auto const lt = scaled_left_top(tr);
    auto const rb = scaled_right_bottom(tr);

    return RectT<V>{lt, rb};
  }

  RectT<V> constexpr
  scaled_half_vertices(Transform const& tr) const
  {
    return scaled_vertices(tr) / V{2};
  }
  */

  void move(value_type const& x, value_type const& y)
  {
    left  += x;
    right += x;

    top    += y;
    bottom += y;
  }

  void move(V const& v)
  {
    move(v.x, v.y);
  }

  V constexpr p0() const { return left_bottom(); }
  V constexpr p1() const { return right_bottom(); }
  V constexpr p2() const { return right_top(); }
  V constexpr p3() const { return left_top(); }

  auto constexpr operator[](size_t const i) const
  {
    switch (i) {
      case 0:
        return p0();
      case 1:
        return p1();
      case 2:
        return p2();
      case 3:
        return p3();
      default:
        break;
    }

    /* INVALID to index this far into a rectangle. Rectangle only has 4 points. */
    std::abort();

    /* Satisfy Compiler */
    return V{};
  }

  auto constexpr points() const {
    return vertices_type(p0(), p1(), p2(), p3());
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

#define DEFINE_RECT_TO_STRING_MEMBER_IMPL(FMT_IDENTIFIER)                                          \
std::string to_string() const                                                                      \
{                                                                                                  \
  return fmt::sprintf(                                                                             \
      "("#FMT_IDENTIFIER","#FMT_IDENTIFIER")"", ""("#FMT_IDENTIFIER","#FMT_IDENTIFIER") "          \
      "(w:"#FMT_IDENTIFIER",h:"#FMT_IDENTIFIER")",                                                 \
      left, top,                                                                                   \
      right, bottom,                                                                               \
      width(), height());                                                                          \
}

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

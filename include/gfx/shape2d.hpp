#pragma once
#include <gfx/types.hpp>
#include <gfx/shape.hpp>

namespace gfx
{

template<typename V>
struct triangle : public shape {
  static auto constexpr NUM_VERTICES = 3;
  V bottom_left, bottom_right, top_middle;
private:
  template<typename T>
  friend class triangle_factory;

  explicit constexpr triangle(enum draw_mode const dm, struct model const& m, V const& bl, V const& br, V const& tm)
    : shape(dm, m)
    , bottom_left(bl)
    , bottom_right(br)
    , top_middle(tm)
  {}
};

template <typename V>
struct rectangle : public shape {
  static auto constexpr NUM_VERTICES = 4;
  V bottom_left, bottom_right, top_right, top_left;

private:
  template<typename T>
  friend class rectangle_factory;

  explicit constexpr rectangle(enum draw_mode const dm, struct model const &m, V const &bl, V const &br, V const &tr,
                               V const &tl)
      : shape(dm, m)
      , bottom_left(bl)
      , bottom_right(br)
      , top_right(tr)
      , top_left(tl)
  {
  }
};

template <typename V>
struct polygon : public shape {
  stlw::sized_buffer<V> vertex_attributes;
  int num_vertices() const { return this->vertex_attributes.length(); }

  template<typename T>
  friend struct polygon_factory;

private:
  explicit polygon(enum draw_mode const dm, struct model const &m, int const num_vertices)
      : shape(dm, m)
      , vertex_attributes(num_vertices)
  {
  }
};

} // ns gfx

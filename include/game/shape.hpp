#pragma once
#include <game/data_types.hpp>
#include <stlw/type_macros.hpp>

namespace game {

struct shape {
  world_coordinate coord_;
protected:
  explicit constexpr shape(world_coordinate const& wc) : coord_(wc) {}
public:
  auto constexpr const& wc() const { return this->coord_; }
};

struct vertice {
  vertex vertex;
  color color;
  texture_coord uv;

  vertice() = default;
  explicit constexpr vertice(class vertex const& v, class color const& c, texture_coord const& t)
    : vertex(v)
    , color(c)
    , uv(t)
  {}
};

// clang-format off
struct triangle : public shape {
  static auto constexpr NUM_VERTICES = 3;
  vertice bottom_left, bottom_right, top_middle;
private:
  friend class shape_factory;
  explicit constexpr triangle(world_coordinate const& wc, vertice const& bl, vertice const& br,
      vertice const& tm)
    : shape(wc)
    , bottom_left(bl)
    , bottom_right(br)
    , top_middle(tm)
  {}
};
// clang-format on

struct rectangle : public shape {
  static auto constexpr NUM_VERTICES = 4;
  vertice bottom_left, bottom_right, top_right, top_left;
private:
  friend class shape_factory;
  explicit constexpr rectangle(world_coordinate const& wc, vertice const& bl, vertice const& br,
      vertice const& tr, vertice const& tl)
    : shape(wc)
    , bottom_left(bl)
    , bottom_right(br)
    , top_right(tr)
    , top_left(tl)
  {}
};

struct polygon : public shape {
  stlw::sized_buffer<vertice> vertices;
  int num_vertices() const { return this->vertices.length(); }
private:
  friend class shape_factory;
  explicit polygon(world_coordinate const& wc, int const num_vertices)
    : shape(wc)
    , vertices(num_vertices)
  {
  }

public:
  friend struct shape_factory;
};

struct shape_factory {
  shape_factory() = delete;

  static constexpr auto make_triangle(world_coordinate const& wc)
  {
    constexpr float radius = 0.5;

    // clang-format off
    vertex const v_bottom_left {wc.x() - radius, wc.y() - radius, wc.z(), wc.w()};
    vertex const v_bottom_right{wc.x() + radius, wc.y() - radius, wc.z(), wc.w()};
    vertex const v_top_middle  {wc.x()         , wc.y() + radius, wc.z(), wc.w()};

    color const c_bottom_left {game::color::DEFAULT_TEXTURE()};
    color const c_bottom_right{game::color::DEFAULT_TEXTURE()};
    color const c_top_middle  {game::color::DEFAULT_TEXTURE()};

    texture_coord const uv_bottom_left {0.0f, 0.0f};
    texture_coord const uv_bottom_right{1.0f, 0.0f};
    texture_coord const uv_top_middle  {0.5f, 1.0f};

    vertice const bottom_left{v_bottom_left, c_bottom_left, uv_bottom_left};
    vertice const bottom_right{v_bottom_right, c_bottom_right, uv_bottom_right};
    vertice const top_middle{v_top_middle, c_top_middle, uv_top_middle};

    // clang-format on
    return triangle{wc, bottom_left, bottom_right, top_middle};
  }

  static auto make_rectangle(world_coordinate const& wc)
  {
    constexpr float width = 0.25;
    constexpr float height = 0.39;

    // clang-format off
    vertex const v_bottom_left {wc.x() - width, wc.y() - height, wc.z(), wc.w()};
    vertex const v_bottom_right{wc.x() + width, wc.y() - height, wc.z(), wc.w()};
    vertex const v_top_right   {wc.x() + width, wc.y() + height, wc.z(), wc.w()};
    vertex const v_top_left    {wc.x() - width, wc.y() + height, wc.z(), wc.w()};

    color const c_bottom_left {game::color::DEFAULT_TEXTURE()};
    color const c_bottom_right{game::color::DEFAULT_TEXTURE()};
    color const c_top_right   {game::color::DEFAULT_TEXTURE()};
    color const c_top_left    {game::color::DEFAULT_TEXTURE()};

    texture_coord const uv_bottom_left {0.0f, 0.0f};
    texture_coord const uv_bottom_right{1.0f, 0.0f};
    texture_coord const uv_top_right   {1.0f, 1.0f};
    texture_coord const uv_top_left    {0.0f, 1.0f};

    vertice const bottom_left {v_bottom_left,  c_bottom_left,  uv_bottom_left};
    vertice const bottom_right{v_bottom_right, c_bottom_right, uv_bottom_right};
    vertice const top_right   {v_top_right,    c_top_right,    uv_top_right};
    vertice const top_left    {v_top_left,     c_top_left,     uv_top_left};
    // clang-format on

    return rectangle{wc, bottom_left, bottom_right, top_right, top_left};
  }

  static auto make_polygon(world_coordinate const& wc, int const num_vertices)
  {
    constexpr float width = 0.25;
    constexpr float radius = width;
    auto const V = num_vertices;
    auto const C = V; // Assume for now #colors == #vertices
    auto const E = V + 1; // num_edges

    auto const cosfn = [&radius, &wc, &E](auto const a) {
      auto const pos = radius * static_cast<float>(std::cos(2*M_PI*a/E));
      return wc.x() + pos;
    };
    auto const sinfn = [&radius, &wc, &E](auto const a) {
      auto const pos = radius * static_cast<float>(std::sin(2*M_PI*a/E));
      return wc.y() + pos;
    };

    polygon poly{wc, num_vertices};
    for (auto i{0}, j{0}; i < V; ++i) {
      auto const x = cosfn(i);
      auto const y = sinfn(i);
      vertex const v{x, y, wc.z(), wc.w()};
      color const  c{game::color::DEFAULT_TEXTURE()};
      texture_coord const uv{x, y}; // TODO: figure this mapping out.

      poly.vertices[i] = vertice{v, c, uv};
    }
    return poly;
  }
};

} // ns game

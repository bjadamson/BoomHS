#pragma once
#include <engine/gfx/colors.hpp>
#include <game/data_types.hpp>
#include <stlw/type_macros.hpp>

namespace game
{

struct shape {
  world_coordinate coord_;

protected:
  explicit constexpr shape(world_coordinate const &wc)
      : coord_(wc)
  {
  }

public:
  auto constexpr const &wc() const { return this->coord_; }
};

struct vertice {
  vertex vertex;
  color color;
  texture_coord uv;

  vertice() = default;
  explicit constexpr vertice(class vertex const &v, class color const &c, texture_coord const &t)
      : vertex(v)
      , color(c)
      , uv(t)
  {
  }
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
  explicit constexpr rectangle(world_coordinate const &wc, vertice const &bl, vertice const &br,
                               vertice const &tr, vertice const &tl)
      : shape(wc)
      , bottom_left(bl)
      , bottom_right(br)
      , top_right(tr)
      , top_left(tl)
  {
  }
};

struct polygon : public shape {
  stlw::sized_buffer<vertice> vertices;
  int num_vertices() const { return this->vertices.length(); }
private:
  friend class shape_factory;
  explicit polygon(world_coordinate const &wc, int const num_vertices)
      : shape(wc)
      , vertices(num_vertices)
  {
  }

public:
  friend struct shape_factory;
};

class shape_factory
{
  shape_factory() = delete;

  static constexpr auto
  construct_rectangle(world_coordinate const &wc, std::array<vertex, 4> const &vertices,
                      std::array<color, 4> const &colors, std::array<texture_coord, 4> const &uv)
  {
    vertice const bottom_left{vertices[0], colors[0], uv[0]};
    vertice const bottom_right{vertices[1], colors[1], uv[1]};
    vertice const top_right{vertices[2], colors[2], uv[2]};
    vertice const top_left{vertices[3], colors[3], uv[3]};

    return rectangle{wc, bottom_left, bottom_right, top_right, top_left};
  }

  static constexpr auto
  construct_triangle(world_coordinate const &wc, std::array<vertex, 3> const &vertices,
                     std::array<color, 3> const &colors, std::array<texture_coord, 3> const &uv)
  {
    vertice const bottom_left{vertices[0], colors[0], uv[0]};
    vertice const bottom_right{vertices[1], colors[1], uv[1]};
    vertice const top_middle{vertices[2], colors[2], uv[2]};

    return triangle{wc, bottom_left, bottom_right, top_middle};
  }

public:
  static constexpr auto
  make_triangle(world_coordinate const &wc, float const radius, std::array<color, 3> const &colors)
  {
    constexpr auto N = 3;

    // clang-format off
    std::array<vertex, N> const array_vertices = {
      vertex{wc.x() - radius, wc.y() - radius, wc.z(), wc.w()}, // bottom-left
      vertex{wc.x() + radius, wc.y() - radius, wc.z(), wc.w()}, // bottom-right
      vertex{wc.x()         , wc.y() + radius, wc.z(), wc.w()}  // top-middle
    };

    std::array<color, N> const array_colors = {
      colors[0], // top-left
      colors[1], // bottom-left
      colors[2]  // top-middle
    };

    std::array<texture_coord, N> const array_uvs = {
      texture_coord{0.0f, 0.0f}, // bottom-left
      texture_coord{1.0f, 0.0f}, // bottom-right
      texture_coord{0.5f, 1.0f}  // top-middle
    };

    // clang-format on
    return construct_triangle(wc, array_vertices, array_colors, array_uvs);
  }

  static constexpr auto
  make_triangle(world_coordinate const &wc, std::array<float, 3> const &colors)
  {
    auto constexpr alpha = 1.0f;
    std::array<float, 4> const bottom_left{colors[0], colors[1], colors[2], alpha};
    std::array<float, 4> const bottom_right{bottom_left};
    std::array<float, 4> const top_middle{bottom_left};

    constexpr float radius = 0.5;

    auto const array_colors =
        std::array<color, 3>{color{bottom_left}, color{bottom_right}, color{top_middle}};
    return make_triangle(wc, radius, array_colors);
  }

  static constexpr auto make_triangle(world_coordinate const &wc)
  {
    return make_triangle(wc, ::engine::gfx::LIST_OF_COLORS::CORAL);
  }

  template <typename T>
  static constexpr auto make_triangle(world_coordinate const &wc, T const &data)
  {
    auto constexpr radius = 0.5f;
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tm = data[2];
    std::array<float, 4> const bottom_left{bl.first[0], bl.first[1], bl.first[2], bl.second};
    std::array<float, 4> const bottom_right{br.first[0], br.first[1], br.first[2], br.second};
    std::array<float, 4> const top_middle{tm.first[0], tm.first[1], tm.first[2], tm.second};

    auto const array_colors =
        std::array<color, 3>{color{bottom_left}, color{bottom_right}, color{top_middle}};
    return make_triangle(wc, radius, array_colors);
  }

  static constexpr auto make_rectangle(world_coordinate const &wc, std::array<color, 4> const& colors,
      std::array<texture_coord, 4> const& uvs, float const height, float const width)
  {
    constexpr auto N = 4;

    // clang-format off
    std::array<vertex, N> const vertices = {
      vertex{wc.x() - width, wc.y() - height, wc.z(), wc.w()}, // bottom-left
      vertex{wc.x() + width, wc.y() - height, wc.z(), wc.w()}, // bottom-right
      vertex{wc.x() + width, wc.y() + height, wc.z(), wc.w()}, // top-right
      vertex{wc.x() - width, wc.y() + height, wc.z(), wc.w()}  // top-left
    };

    return construct_rectangle(wc, vertices, colors, uvs);
  }

  static constexpr auto
  make_rectangle(world_coordinate const &wc, std::array<float, 3> const &c,
                 float const height = 0.39f, float const width = 0.25f, float const alpha = 1.0f)
  {
    constexpr auto N = 4;

    // clang-format off
    std::array<color, N> const colors = {
      color{c, alpha}, // bottom-left
      color{c, alpha}, // bottom-right
      color{c, alpha}, // top-right
      color{c, alpha}, // top-left
    };

    std::array<texture_coord, N> constexpr uvs = {
      texture_coord{0.0f, 0.0f}, // bottom-left
      texture_coord{1.0f, 0.0f}, // bottom-right
      texture_coord{1.0f, 1.0f}, // top-right
      texture_coord{0.0f, 1.0f}, // top-left
    };
    // clang-format on

    return make_rectangle(wc, colors, uvs, height, width);
  }

  template <typename T>
  static constexpr auto make_rectangle(world_coordinate const &wc, T const &data,
                                       float const height = 0.39f, float const width = 0.25f)
  {
    auto constexpr radius = 0.5f;
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tr = data[2];
    auto const &tl = data[3];
    std::array<float, 4> const bottom_left{bl.first[0], bl.first[1], bl.first[2], bl.second};
    std::array<float, 4> const bottom_right{br.first[0], br.first[1], br.first[2], br.second};
    std::array<float, 4> const top_right{tr.first[0], tr.first[1], tr.first[2], tr.second};
    std::array<float, 4> const top_left{tl.first[0], tl.first[1], tl.first[2], tl.second};

    std::array<color, 4> const colors = {
        color{bottom_left}, color{bottom_right}, color{top_right}, color{top_left},
    };
    std::array<texture_coord, 4> constexpr uvs = {
        texture_coord{0.0f, 0.0f}, // bottom-left
        texture_coord{1.0f, 0.0f}, // bottom-right
        texture_coord{1.0f, 1.0f}, // top-right
        texture_coord{0.0f, 1.0f}, // top-left
    };
    return make_rectangle(wc, colors, uvs, height, width);
  }

  static constexpr auto make_rectangle(world_coordinate const &wc)
  {
    return make_rectangle(wc, ::engine::gfx::LIST_OF_COLORS::RED);
  }

  static auto make_polygon(world_coordinate const &wc, int const num_vertices, float const alpha,
                           float const width, std::array<float, 3> const &c)
  {
    float const radius = width;
    auto const C = num_vertices;     // Assume for now #colors == #vertices
    auto const E = num_vertices + 1; // num_edges

    auto const cosfn = [&radius, &wc, &E](auto const a) {
      auto const pos = radius * static_cast<float>(std::cos(2 * M_PI * a / E));
      return wc.x() + pos;
    };
    auto const sinfn = [&radius, &wc, &E](auto const a) {
      auto const pos = radius * static_cast<float>(std::sin(2 * M_PI * a / E));
      return wc.y() + pos;
    };

    polygon poly{wc, num_vertices};
    for (auto i{0}, j{0}; i < num_vertices; ++i) {
      auto const x = cosfn(i);
      auto const y = sinfn(i);

      color const col{c[0], c[1], c[2], alpha};
      vertex const v{x, y, wc.z(), wc.w()};
      texture_coord const uv{x, y};
      poly.vertices[i] = vertice{v, col, uv};
    }
    return poly;
  }

  static auto make_polygon(world_coordinate const &wc, int const num_vertices,
                           float const alpha = 1.0f, float const width = 0.25f)
  {
    return make_polygon(wc, num_vertices, alpha, width, ::engine::gfx::LIST_OF_COLORS::PINK);
  }

  static auto make_polygon(world_coordinate const &wc, int const num_vertices,
                           std::array<float, 3> const &color, float const alpha = 1.0f,
                           float const width = 0.25f)
  {
    return make_polygon(wc, num_vertices, alpha, width, color);
  }
};

} // ns game

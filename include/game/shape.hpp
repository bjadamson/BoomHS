#pragma once
#include <engine/gfx/colors.hpp>
#include <game/data_types.hpp>
#include <stlw/type_macros.hpp>

namespace game
{

class height_width
{
  std::pair<float, float> pair_;
public:
  constexpr height_width(float const h, float const w) : pair_(h, w) {}

  auto height() const { return this->pair_.first; }
  auto width() const { return this->pair_.second; }
};

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

struct vertex_color_attributes {
  vertex vertex;
  color color;

  vertex_color_attributes() = default;
  explicit constexpr vertex_color_attributes(class vertex const &v, class color const &c)
      : vertex(v)
      , color(c)
  {
  }
};

struct vertex_uv_attributes {
  vertex vertex;
  texture_coord uv;

  vertex_uv_attributes() = default;
  explicit constexpr vertex_uv_attributes(class vertex const &v, texture_coord const &t)
      : vertex(v)
      , uv(t)
  {
  }
};

// clang-format off
template<typename V>
struct triangle : public shape {
  static auto constexpr NUM_VERTICES = 3;
  V bottom_left, bottom_right, top_middle;
private:
  friend class triangle_factory;
  explicit constexpr triangle(world_coordinate const& wc, V const& bl, V const& br, V const& tm)
    : shape(wc)
    , bottom_left(bl)
    , bottom_right(br)
    , top_middle(tm)
  {}
};
// clang-format on

template<typename V>
struct rectangle : public shape {
  static auto constexpr NUM_VERTICES = 4;
  V bottom_left, bottom_right, top_right, top_left;

private:
  friend class shape_factory;
  friend class rectangle_factory;
  explicit constexpr rectangle(world_coordinate const &wc, V const &bl, V const &br, V const &tr,
      V const &tl)
    : shape(wc)
    , bottom_left(bl)
    , bottom_right(br)
    , top_right(tr)
    , top_left(tl)
  {
  }
};

template<typename V>
struct polygon : public shape {
  stlw::sized_buffer<V> vertex_attributes;
  int num_vertices() const { return this->vertex_attributes.length(); }
  friend struct polygon_factory;

private:
  explicit polygon(world_coordinate const &wc, int const num_vertices)
      : shape(wc)
      , vertex_attributes(num_vertices)
  {
  }
};

class polygon_factory
{
  polygon_factory() = delete;

  struct properties
  {
    GLint const num_vertices;
    std::array<float, 3> const colors;

    float const alpha = 1.0f;
    float const width = 0.25f;
  };

  static auto
  construct(world_coordinate const& wc, properties const& props)
  {
    float const radius = props.width;
    auto const num_vertices = props.num_vertices;

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

    polygon<vertex_color_attributes> poly{wc, num_vertices};
    for (auto i{0}, j{0}; i < num_vertices; ++i) {
      auto const x = cosfn(i);
      auto const y = sinfn(i);

      vertex const v{x, y, wc.z(), wc.w()};
      color const col{props.colors[0], props.colors[1], props.colors[2], props.alpha};
      texture_coord const uv{x, y};
      poly.vertex_attributes[i] = vertex_color_attributes{v, col};//, uv};
    }
    return poly;
  }

public:
  static auto make(world_coordinate const &wc, int const num_vertices,
                           std::array<float, 3> const &color)
  {
    properties const prop{num_vertices, color};
    return construct(wc, prop);
  }

  static auto make(world_coordinate const &wc, int const num_vertices)
  {
    auto constexpr COLOR = ::engine::gfx::LIST_OF_COLORS::PINK;
    properties const prop{num_vertices, COLOR};
    return construct(wc, prop);
  }
};

class triangle_factory
{
  struct properties
  {
    std::array<float, 4> const& color_bottom_left;
    std::array<float, 4> const& color_bottom_right = color_bottom_left;
    std::array<float, 4> const& color_top_middle = color_bottom_left;

    float const radius = 0.5f;

    // clang-format off
    std::array<texture_coord, 3> const uv = {
      texture_coord{0.0f, 0.0f}, // bottom-left
      texture_coord{1.0f, 0.0f}, // bottom-right
      texture_coord{0.5f, 1.0f}  // top-middle
    };
    // clang-format on
  };

  static constexpr auto
  construct(world_coordinate const &wc, properties const& props)
  {
    auto const radius = props.radius;
    std::array<vertex, 3> const vertices = {
      vertex{wc.x() - radius, wc.y() - radius, wc.z(), wc.w()}, // bottom-left
      vertex{wc.x() + radius, wc.y() - radius, wc.z(), wc.w()}, // bottom-right
      vertex{wc.x()         , wc.y() + radius, wc.z(), wc.w()}  // top-middle
    };
    auto const& uv = props.uv;

    vertex_color_attributes const bottom_left{vertices[0], color{props.color_bottom_left}};
    vertex_color_attributes const bottom_right{vertices[1], color{props.color_bottom_right}};
    vertex_color_attributes const top_middle{vertices[2], color{props.color_top_middle}};

    return triangle<vertex_color_attributes>{wc, bottom_left, bottom_right, top_middle};
  }

public:
  static constexpr auto
  make(world_coordinate const &wc, float const radius, std::array<float, 3> const &c)
  {
    auto constexpr ALPHA = 1.0f;
    auto const color = std::array<float, 4>{c[0], c[1], c[2], ALPHA};
    properties const p{color, color, color, radius};
    return construct(wc, p);
  }

  static constexpr auto
  make(world_coordinate const &wc, std::array<float, 4> const& c)
  {
    properties const p{c};
    return construct(wc, p);
  }

  static constexpr auto
  make(world_coordinate const &wc, std::array<float, 3> const& c)
  {
    auto constexpr ALPHA = 1.0f;
    auto const color = std::array<float, 4>{c[0], c[1], c[2], ALPHA};
    properties const p{color, color, color};
    return construct(wc, p);
  }

  static constexpr auto
  make(world_coordinate const& wc)
  {
    return make(wc, ::engine::gfx::LIST_OF_COLORS::RED);
  }

  template <typename T>
  static constexpr auto
  make(world_coordinate const &wc, T const &data)
  {
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tm = data[2];
    std::array<float, 4> const bottom_left{bl[0], bl[1], bl[2], bl[3]};
    std::array<float, 4> const bottom_right{br[0], br[1], br[2], br[3]};
    std::array<float, 4> const top_middle{tm[0], tm[1], tm[2], tm[3]};

    properties const p{bottom_left, bottom_right, top_middle};
    return construct(wc, p);
  }
};

class rectangle_factory
{
  rectangle_factory() = delete;

  struct properties
  {
    height_width const dimensions;
    std::array<float, 4> const& bottom_left;
    std::array<float, 4> const& bottom_right = bottom_left;
    std::array<float, 4> const& top_right = bottom_left;
    std::array<float, 4> const& top_left = bottom_left;

    // clang-format off
    std::array<texture_coord, 4> const uv = {
      texture_coord{0.0f, 0.0f}, // bottom-left
      texture_coord{1.0f, 0.0f}, // bottom-right
      texture_coord{1.0f, 1.0f}, // top-right
      texture_coord{0.0f, 1.0f}, // top-left
    };
    // clang-format on
  };

  static constexpr auto
  construct(world_coordinate const &wc, properties const& props)
  {
    auto const height = props.dimensions.height();
    auto const width = props.dimensions.width();
    std::array<vertex, 4> const vertices = {
      vertex{wc.x() - width, wc.y() - height, wc.z(), wc.w()}, // bottom-left
      vertex{wc.x() + width, wc.y() - height, wc.z(), wc.w()}, // bottom-right
      vertex{wc.x() + width, wc.y() + height, wc.z(), wc.w()}, // top-right
      vertex{wc.x() - width, wc.y() + height, wc.z(), wc.w()}  // top-left
    };
    auto const& colors = props.bottom_left;
    auto const& uv = props.uv;

    vertex_color_attributes const bottom_left{vertices[0], color{props.bottom_left}};
    vertex_color_attributes const bottom_right{vertices[1], color{props.bottom_right}};
    vertex_color_attributes const top_right{vertices[2], color{props.top_right}};
    vertex_color_attributes const top_left{vertices[3], color{props.top_left}};

    return rectangle<vertex_color_attributes>{wc, bottom_left, bottom_right, top_right, top_left};
  }
public:

  static constexpr auto
  make(world_coordinate const &wc, std::array<float, 3> const &c,
                 float const height = 0.39f, float const width = 0.25f, float const alpha = 1.0f)
  {
    properties const p{{height, width}, std::array<float, 4>{c[0], c[1], c[2], alpha}};
    return construct(wc, p);
  }

  template <typename T>
  static constexpr auto make(world_coordinate const &wc, T const &data, float const height,
      float const width)
  {
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tr = data[2];
    auto const &tl = data[3];
    std::array<float, 4> const bottom_left{bl[0], bl[1], bl[2], bl[3]};
    std::array<float, 4> const bottom_right{br[0], br[1], br[2], br[3]};
    std::array<float, 4> const top_right{tr[0], tr[1], tr[2], tr[3]};
    std::array<float, 4> const top_left{tl[0], tl[1], tl[2], tl[3]};

    properties const p{{height, width}, bottom_left, bottom_right, top_right, top_left};
    return construct(wc, p);
  }

  static constexpr auto make(world_coordinate const &wc, std::array<float, 4> const& color,
      float const height, float const width)
  {
    properties const p{{height, width}, color};
    return construct(wc, p);
  }

  static constexpr auto make(world_coordinate const &wc, float const height, float const width)
  {
    auto constexpr ALPHA = 1.0f;
    auto const c = ::engine::gfx::LIST_OF_COLORS::RED;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};

    return make(wc, color, height, width);
  }
};

} // ns game

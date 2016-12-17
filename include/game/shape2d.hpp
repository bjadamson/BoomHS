#pragma once
#include <engine/gfx/types.hpp>
#include <game/shape.hpp>
#include <game/data_types.hpp>
#include <array>

namespace game
{

// clang-format off
template<typename V>
struct triangle : public shape {
  static auto constexpr NUM_VERTICES = 3;
  V bottom_left, bottom_right, top_middle;
private:
  friend class triangle_factory;
  explicit constexpr triangle(drawmode const dm,struct model const& m, V const& bl, V const& br, V const& tm)
    : shape(dm, m)
    , bottom_left(bl)
    , bottom_right(br)
    , top_middle(tm)
  {}
};
// clang-format on

struct triangle_factory {
  static float constexpr DEFAULT_RADIUS = 0.25;

private:
  struct color_properties {
    std::array<float, 4> const &color_bottom_left;
    std::array<float, 4> const &color_bottom_right = color_bottom_left;
    std::array<float, 4> const &color_top_middle = color_bottom_left;

    float const radius = DEFAULT_RADIUS;
  };

  struct uv_properties {
    float const radius = DEFAULT_RADIUS;

    // clang-format off
    std::array<texture_coord, 3> const uv = {
      texture_coord{0.0f, 0.0f}, // bottom-left
      texture_coord{1.0f, 0.0f}, // bottom-right
      texture_coord{0.5f, 1.0f}  // top-middle
    };
    // clang-format on
  };

  struct wireframe_properties {
    float const radius = DEFAULT_RADIUS;
  };

  static constexpr auto calculate_vertices(glm::vec3 const &m, float const radius)
  {
    std::array<vertex, 3> const vertices = {
        vertex{m.x - radius, m.y - radius, m.z, 1.0f}, // bottom-left
        vertex{m.x + radius, m.y - radius, m.z, 1.0f}, // bottom-right
        vertex{m.x, m.y + radius, m.z, 1.0f}           // top-middle
    };
    return vertices;
  }

  static constexpr auto construct(drawmode const dm, model const &m, color_properties const &props)
  {
    auto const vertices = calculate_vertices(m.translation, props.radius);
    vertex_color_attributes const bottom_left{vertices[0], color{props.color_bottom_left}};
    vertex_color_attributes const bottom_right{vertices[1], color{props.color_bottom_right}};
    vertex_color_attributes const top_middle{vertices[2], color{props.color_top_middle}};

    return triangle<vertex_color_attributes>{dm, m, bottom_left, bottom_right, top_middle};
  }

  static constexpr auto construct(drawmode const dm, model const &m, uv_properties const &props)
  {
    auto const vertices = calculate_vertices(m.translation, props.radius);

    vertex_uv_attributes const bottom_left{vertices[0], props.uv[0]};
    vertex_uv_attributes const bottom_right{vertices[1], props.uv[1]};
    vertex_uv_attributes const top_middle{vertices[2], props.uv[2]};

    return triangle<vertex_uv_attributes>{dm, m, bottom_left, bottom_right, top_middle};
  }

  static constexpr auto construct(drawmode const dm, model const &m, wireframe_properties const &props)
  {
    auto const vertices = calculate_vertices(m.translation, props.radius);

    vertex_attributes_only const bottom_left{vertices[0]};
    vertex_attributes_only const bottom_right{vertices[1]};
    vertex_attributes_only const top_middle{vertices[2]};

    return triangle<vertex_attributes_only>{dm, m, bottom_left, bottom_right, top_middle};
  }

public:
  static constexpr auto
  make(drawmode const dm, model const &m, float const radius, std::array<float, 3> const &c)
  {
    auto constexpr ALPHA = 1.0f;
    auto const color = std::array<float, 4>{c[0], c[1], c[2], ALPHA};
    color_properties const p{color, color, color, radius};
    return construct(dm, m, p);
  }

  static constexpr auto make(drawmode const dm, model const &m, std::array<float, 4> const &c)
  {
    color_properties const p{c};
    return construct(dm, m, p);
  }

  static constexpr auto make(drawmode const dm, model const &m, std::array<float, 3> const &c)
  {
    auto constexpr ALPHA = 1.0f;
    auto const color = std::array<float, 4>{c[0], c[1], c[2], ALPHA};
    color_properties const p{color, color, color};
    return construct(dm, m, p);
  }

  static constexpr auto make(drawmode const dm, model const& m, glm::vec4 const& c)
  {
    return make(dm, m, std::array<float, 4>{c[0], c[1], c[2], c[3]});
  }

  static constexpr auto make(drawmode const dm, model const &m)
  {
    return make(dm, m, ::engine::gfx::LIST_OF_COLORS::RED);
  }

  template <typename T>
  static constexpr auto make(drawmode const dm, model const &m, T const &data)
  {
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tm = data[2];
    std::array<float, 4> const bottom_left{bl[0], bl[1], bl[2], bl[3]};
    std::array<float, 4> const bottom_right{br[0], br[1], br[2], br[3]};
    std::array<float, 4> const top_middle{tm[0], tm[1], tm[2], tm[3]};

    color_properties const p{bottom_left, bottom_right, top_middle};
    return construct(dm, m, p);
  }

  static constexpr auto make(drawmode const dm, model const &m, bool const use_texture)
  {
    uv_properties const p;
    return construct(dm, m, p);
  }

  static constexpr auto make(drawmode const dm, model const &m, bool const, bool const)
  {
    wireframe_properties const p;
    return construct(dm, m, p);
  }
};

template <typename V>
struct rectangle : public shape {
  static auto constexpr NUM_VERTICES = 4;
  V bottom_left, bottom_right, top_right, top_left;

private:
  friend class rectangle_factory;
  explicit constexpr rectangle(drawmode const dm, struct model const &m, V const &bl, V const &br, V const &tr,
                               V const &tl)
      : shape(dm, m)
      , bottom_left(bl)
      , bottom_right(br)
      , top_right(tr)
      , top_left(tl)
  {
  }
};

class rectangle_factory
{
  rectangle_factory() = delete;

  using height_width = ::engine::gfx::height_width;

  struct color_properties {
    height_width const dimensions;
    std::array<float, 4> const &bottom_left;
    std::array<float, 4> const &bottom_right = bottom_left;
    std::array<float, 4> const &top_right = bottom_left;
    std::array<float, 4> const &top_left = bottom_left;
  };

  struct uv_properties {
    height_width const dimensions;

    // clang-format off
    std::array<texture_coord, 4> const uv = {
      texture_coord{0.0f, 0.0f}, // bottom-left
      texture_coord{1.0f, 0.0f}, // bottom-right
      texture_coord{1.0f, 1.0f}, // top-right
      texture_coord{0.0f, 1.0f}, // top-left
    };
    // clang-format on
  };

  struct wireframe_properties {
    height_width const dimensions;
    GLint const num_vertices;

    float const alpha = 1.0f;
    float const width = 0.25f;
  };

  static constexpr auto calculate_vertices(glm::vec3 const &m, height_width const &hw)
  {
    auto const height = hw.height;
    auto const width = hw.width;
    return std::array<vertex, 4>{
        vertex{m.x - width, m.y - height, m.z, 1.0f}, // bottom-left
        vertex{m.x + width, m.y - height, m.z, 1.0f}, // bottom-right
        vertex{m.x + width, m.y + height, m.z, 1.0f}, // top-right
        vertex{m.x - width, m.y + height, m.z, 1.0f}  // top-left
    };
  }

  static constexpr auto construct(drawmode const dm, model const &m, color_properties const &props)
  {
    auto const vertices = calculate_vertices(m.translation, props.dimensions);

    vertex_color_attributes const bottom_left{vertices[0], color{props.bottom_left}};
    vertex_color_attributes const bottom_right{vertices[1], color{props.bottom_right}};
    vertex_color_attributes const top_right{vertices[2], color{props.top_right}};
    vertex_color_attributes const top_left{vertices[3], color{props.top_left}};

    return rectangle<vertex_color_attributes>{dm, m, bottom_left, bottom_right, top_right, top_left};
  }

  static constexpr auto construct(drawmode const dm, model const &m, uv_properties const &props)
  {
    auto const vertices = calculate_vertices(m.translation, props.dimensions);

    vertex_uv_attributes const bottom_left{vertices[0], props.uv[0]};
    vertex_uv_attributes const bottom_right{vertices[1], props.uv[1]};
    vertex_uv_attributes const top_right{vertices[2], props.uv[2]};
    vertex_uv_attributes const top_left{vertices[3], props.uv[3]};

    return rectangle<vertex_uv_attributes>{dm, m, bottom_left, bottom_right, top_right, top_left};
  }

  static constexpr auto construct(drawmode const dm, model const &m, wireframe_properties const &props)
  {
    auto const vertices = calculate_vertices(m.translation, props.dimensions);

    vertex_attributes_only const bottom_left{vertices[0]};
    vertex_attributes_only const bottom_right{vertices[1]};
    vertex_attributes_only const top_right{vertices[2]};
    vertex_attributes_only const top_left{vertices[3]};
    return rectangle<vertex_attributes_only>{dm, m, bottom_left, bottom_right, top_right, top_left};
  }

public:
  static constexpr auto
  make(drawmode const dm, model const &m, std::array<float, 3> const &c, float const height = 0.39f,
       float const width = 0.25f, float const alpha = 1.0f)
  {
    color_properties const p{{height, width}, std::array<float, 4>{c[0], c[1], c[2], alpha}};
    return construct(dm, m, p);
  }

  template <typename T>
  static constexpr auto
  make(drawmode const dm, model const &m, T const &data, float const height, float const width)
  {
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tr = data[2];
    auto const &tl = data[3];
    std::array<float, 4> const bottom_left{bl[0], bl[1], bl[2], bl[3]};
    std::array<float, 4> const bottom_right{br[0], br[1], br[2], br[3]};
    std::array<float, 4> const top_right{tr[0], tr[1], tr[2], tr[3]};
    std::array<float, 4> const top_left{tl[0], tl[1], tl[2], tl[3]};

    color_properties const p{{height, width}, bottom_left, bottom_right, top_right, top_left};
    return construct(dm, m, p);
  }

  static constexpr auto make(drawmode const dm, model const &m, std::array<float, 4> const &color,
                             float const height, float const width)
  {
    color_properties const p{{height, width}, color};
    return construct(dm, m, p);
  }

  static constexpr auto make(drawmode const dm, model const &m, float const height, float const width)
  {
    auto constexpr ALPHA = 1.0f;
    auto const c = ::engine::gfx::LIST_OF_COLORS::RED;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};

    return make(dm, m, color, height, width);
  }

  static constexpr auto
  make(drawmode const dm, model const &m, float const height, float const width, bool const)
  {
    uv_properties const p{{height, width}};
    return construct(dm, m, p);
  }

  static constexpr auto
  make(drawmode const dm, model const &m, float const height, float const width, bool const, bool const)
  {
    wireframe_properties const p{{height, width}};
    return construct(dm, m, p);
  }

  template <typename T>
  static constexpr auto
  make(drawmode const dm, model const &m, float const height, float const width, T const &data)
  {
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tr = data[2];
    auto const &tl = data[3];
    std::array<float, 4> const bottom_left{bl[0], bl[1], bl[2], bl[3]};
    std::array<float, 4> const bottom_right{br[0], br[1], br[2], br[3]};
    std::array<float, 4> const top_right{tr[0], tr[1], tr[2], tr[3]};
    std::array<float, 4> const top_left{tl[0], tl[1], tl[2], tl[3]};

    color_properties const p{{height, width}, bottom_left, bottom_right, top_right, top_left};
    return construct(dm, m, p);
  }
};

template <typename V>
struct polygon : public shape {
  stlw::sized_buffer<V> vertex_attributes;
  int num_vertices() const { return this->vertex_attributes.length(); }
  friend struct polygon_factory;

private:
  explicit polygon(drawmode const dm, struct model const &m, int const num_vertices)
      : shape(dm, m)
      , vertex_attributes(num_vertices)
  {
  }
};

class polygon_factory
{
  polygon_factory() = delete;

  struct color_properties {
    GLint const num_vertices;
    std::array<float, 3> const colors;

    float const alpha = 1.0f;
    float const width = 0.25f;
  };

  struct uv_properties {
    GLint const num_vertices;

    float const alpha = 1.0f;
    float const width = 0.25f;
  };

  struct wireframe_properties {
    GLint const num_vertices;
    float const alpha = 1.0f;
    float const width = 0.25f;
  };

  template<typename R, typename P, typename FN>
  static auto
  construct_polygon(drawmode const dm, model const &m, P const& properties, FN const& fill_vertice)
  {
    float const width = properties.width;
    auto const num_vertices = properties.num_vertices;

    auto const C = num_vertices;     // Assume for now #colors == #vertices
    auto const E = num_vertices + 1; // num_edges
    auto const t = m.translation;

    auto const cosfn = [&width, &t, &E](auto const a) {
      auto const pos = width * static_cast<float>(std::cos(2 * M_PI * a / E));
      return t.x + pos;
    };
    auto const sinfn = [&width, &t, &E](auto const a) {
      auto const pos = width * static_cast<float>(std::sin(2 * M_PI * a / E));
      return t.y + pos;
    };

    polygon<R> poly{dm, m, num_vertices};
    for (auto i{0} ; i < num_vertices; ++i) {
      auto const x = cosfn(i);
      auto const y = sinfn(i);

      vertex const v{x, y, t.z, 1.0f};
      fill_vertice(poly, v, i);
    }
    return poly;
  }

  static auto construct(drawmode const dm, model const &m, color_properties const &props)
  {
    using R = vertex_color_attributes;
    auto const fill_vertice = [&](auto &poly, auto const& v, auto const i) {
      color const col{props.colors[0], props.colors[1], props.colors[2], props.alpha};
      poly.vertex_attributes[i] = R{v, col};
    };
    return construct_polygon<R>(dm, m, props, fill_vertice);
  }

  static auto construct(drawmode const dm, model const &m, uv_properties const &props)
  {
    using R = vertex_uv_attributes;
    auto const fill_vertice = [&](auto &poly, auto const& v, auto const i) {
      texture_coord const uv{v.x, v.y};
      poly.vertex_attributes[i] = vertex_uv_attributes{v, uv};
    };
    return construct_polygon<R>(dm, m, props, fill_vertice);
  }

  static auto construct(drawmode const dm, model const &m, wireframe_properties const &props)
  {
    using R = vertex_attributes_only;
    auto const fill_vertice = [&](auto &poly, auto const& v, auto const i) {
      poly.vertex_attributes[i] = R{v};
    };
    return construct_polygon<R>(dm, m, props, fill_vertice);
  }

public:
  static auto
  make(drawmode const dm, model const &m, int const num_vertices, std::array<float, 3> const &color)
  {
    color_properties const prop{num_vertices, color};
    return construct(dm, m, prop);
  }

  static auto make(drawmode const dm, model const &m, int const num_vertices)
  {
    auto constexpr COLOR = ::engine::gfx::LIST_OF_COLORS::LAWN_GREEN;
    color_properties const prop{num_vertices, COLOR};
    return construct(dm, m, prop);
  }

  static auto make(drawmode const dm, model const &m, int const num_vertices, bool const)
  {
    uv_properties const p{num_vertices};
    return construct(dm, m, p);
  }

  static auto make(drawmode const dm, model const &m, int const num_vertices, bool const, bool const)
  {
    wireframe_properties const p{num_vertices};
    return construct(dm, m, p);
  }
};

} // ns game

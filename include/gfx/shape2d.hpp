#pragma once
#include <gfx/types.hpp>
#include <gfx/shape.hpp>
#include <array>

namespace gfx
{

// clang-format off
template<typename V>
struct triangle : public shape {
  static auto constexpr NUM_VERTICES = 3;
  V bottom_left, bottom_right, top_middle;
private:
  friend class triangle_factory;
  explicit constexpr triangle(enum draw_mode const dm, struct model const& m, V const& bl, V const& br, V const& tm)
    : shape(dm, m)
    , bottom_left(bl)
    , bottom_right(br)
    , top_middle(tm)
  {}
};
// clang-format on

struct triangle_properties
{
  draw_mode const draw_mode;
  model const &model;
  float const radius = 0.25;
};

struct triangle_factory {

private:
  struct color_properties {
    std::array<float, 4> const &color_bottom_left;
    std::array<float, 4> const &color_bottom_right = color_bottom_left;
    std::array<float, 4> const &color_top_middle = color_bottom_left;
  };

  struct uv_properties {
    std::array<uv_d, 3> const uv = {
      // clang-format off
      uv_d{0.0f, 0.0f}, // bottom-left
      uv_d{1.0f, 0.0f}, // bottom-right
      uv_d{0.5f, 1.0f}  // top-middle
    };
    // clang-format on
  };

  struct wireframe_properties {
  };

  static constexpr auto calculate_vertices(glm::vec3 const &m, float const radius)
  {
    std::array<vertex_d, 3> const vertices = {
        vertex_d{m.x - radius, m.y - radius, m.z, 1.0f}, // bottom-left
        vertex_d{m.x + radius, m.y - radius, m.z, 1.0f}, // bottom-right
        vertex_d{m.x, m.y + radius, m.z, 1.0f}           // top-middle
    };
    return vertices;
  }

  static constexpr auto construct(triangle_properties const& tprops, color_properties const &cprops)
  {
    auto const vertices = calculate_vertices(tprops.model.translation, tprops.radius);

    vertex_color_attributes const bottom_left{vertices[0], color_d{cprops.color_bottom_left}};
    vertex_color_attributes const bottom_right{vertices[1], color_d{cprops.color_bottom_right}};
    vertex_color_attributes const top_middle{vertices[2], color_d{cprops.color_top_middle}};

    return triangle<vertex_color_attributes>{tprops.draw_mode, tprops.model, bottom_left, bottom_right, top_middle};
  }

  static constexpr auto construct(triangle_properties const& tprops, uv_properties const &cprops)
  {
    auto const vertices = calculate_vertices(tprops.model.translation, tprops.radius);

    vertex_uv_attributes const bottom_left{vertices[0], cprops.uv[0]};
    vertex_uv_attributes const bottom_right{vertices[1], cprops.uv[1]};
    vertex_uv_attributes const top_middle{vertices[2], cprops.uv[2]};

    return triangle<vertex_uv_attributes>{tprops.draw_mode, tprops.model, bottom_left, bottom_right, top_middle};
  }

  static constexpr auto construct(triangle_properties const& tprops, wireframe_properties const &cprops)
  {
    auto const vertices = calculate_vertices(tprops.model.translation, tprops.radius);

    vertex_attributes_only const bottom_left{vertices[0]};
    vertex_attributes_only const bottom_right{vertices[1]};
    vertex_attributes_only const top_middle{vertices[2]};

    return triangle<vertex_attributes_only>{tprops.draw_mode, tprops.model, bottom_left, bottom_right, top_middle};
  }

public:
  static constexpr auto make(triangle_properties const& tprops, color_t, std::array<float, 4> const &c)
  {
    color_properties const p{c};
    return construct(tprops, p);
  }

  static constexpr auto make(triangle_properties const& tprops, color_t, std::array<float, 3> const &c)
  {
    auto constexpr ALPHA = 1.0f;
    auto const color = std::array<float, 4>{c[0], c[1], c[2], ALPHA};
    color_properties const p{color, color, color};
    return construct(tprops, p);
  }

  static constexpr auto make(triangle_properties const& tprops, color_t, glm::vec4 const& c)
  {
    return make(tprops, color_t{}, std::array<float, 4>{c[0], c[1], c[2], c[3]});
  }

  static constexpr auto make(triangle_properties const& tprops, color_t)
  {
    return make(tprops, color_t{}, LIST_OF_COLORS::RED);
  }

  template <typename T>
  static constexpr auto make(triangle_properties const& tprops, color_t, T const &data)
  {
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tm = data[2];
    std::array<float, 4> const bottom_left{bl[0], bl[1], bl[2], bl[3]};
    std::array<float, 4> const bottom_right{br[0], br[1], br[2], br[3]};
    std::array<float, 4> const top_middle{tm[0], tm[1], tm[2], tm[3]};

    color_properties const p{bottom_left, bottom_right, top_middle};
    return construct(tprops, p);
  }

  static constexpr auto make(triangle_properties const& tprops, uv_t)
  {
    uv_properties const p;
    return construct(tprops, p);
  }

  static constexpr auto make(triangle_properties const& tprops, wireframe_t)
  {
    wireframe_properties const p;
    return construct(tprops, p);
  }
};

template <typename V>
struct rectangle : public shape {
  static auto constexpr NUM_VERTICES = 4;
  V bottom_left, bottom_right, top_right, top_left;

private:
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

struct rectangle_properties
{
  draw_mode const draw_mode;
  model const &model;
  height_width const dimensions = {0.39f, 0.25f};
};

class rectangle_factory
{
  rectangle_factory() = delete;

  using height_width = height_width;

  struct color_properties {
    color_d const &bottom_left;
    color_d const &bottom_right = bottom_left;
    color_d const &top_right = bottom_left;
    color_d const &top_left = bottom_left;
  };

  struct uv_properties {
    // clang-format off
    std::array<uv_d, 4> const uv = {
      uv_d{0.0f, 0.0f}, // bottom-left
      uv_d{1.0f, 0.0f}, // bottom-right
      uv_d{1.0f, 1.0f}, // top-right
      uv_d{0.0f, 1.0f}, // top-left
    };
    // clang-format on
  };

  struct wireframe_properties {
    float const alpha = 1.0f;
  };

  static constexpr auto calculate_vertices(glm::vec3 const &m, height_width const &hw)
  {
    auto const height = hw.height;
    auto const width = hw.width;
    return std::array<vertex_d, 4>{
        vertex_d{m.x - width, m.y - height, m.z, 1.0f}, // bottom-left
        vertex_d{m.x + width, m.y - height, m.z, 1.0f}, // bottom-right
        vertex_d{m.x + width, m.y + height, m.z, 1.0f}, // top-right
        vertex_d{m.x - width, m.y + height, m.z, 1.0f}  // top-left
    };
  }

  static constexpr auto construct(rectangle_properties const& rprops, color_properties const &cprops)
  {
    auto const vertices = calculate_vertices(rprops.model.translation, rprops.dimensions);

    vertex_color_attributes const bottom_left{vertices[0], color_d{cprops.bottom_left}};
    vertex_color_attributes const bottom_right{vertices[1], color_d{cprops.bottom_right}};
    vertex_color_attributes const top_right{vertices[2], color_d{cprops.top_right}};
    vertex_color_attributes const top_left{vertices[3], color_d{cprops.top_left}};

    return rectangle<vertex_color_attributes>{rprops.draw_mode, rprops.model, bottom_left, bottom_right, top_right, top_left};
  }

  static constexpr auto construct(rectangle_properties const& rprops, uv_properties const &props)
  {
    auto const vertices = calculate_vertices(rprops.model.translation, rprops.dimensions);

    vertex_uv_attributes const bottom_left{vertices[0], props.uv[0]};
    vertex_uv_attributes const bottom_right{vertices[1], props.uv[1]};
    vertex_uv_attributes const top_right{vertices[2], props.uv[2]};
    vertex_uv_attributes const top_left{vertices[3], props.uv[3]};

    return rectangle<vertex_uv_attributes>{rprops.draw_mode, rprops.model, bottom_left, bottom_right, top_right, top_left};
  }

  static constexpr auto construct(rectangle_properties const& rprops, wireframe_properties const &props)
  {
    auto const vertices = calculate_vertices(rprops.model.translation, rprops.dimensions);

    vertex_attributes_only const bottom_left{vertices[0]};
    vertex_attributes_only const bottom_right{vertices[1]};
    vertex_attributes_only const top_right{vertices[2]};
    vertex_attributes_only const top_left{vertices[3]};
    return rectangle<vertex_attributes_only>{rprops.draw_mode, rprops.model, bottom_left, bottom_right, top_right, top_left};
  }

public:
  static constexpr auto
  make(rectangle_properties const& rprops, color_t, color_properties const& cprops)
  {
    return construct(rprops, cprops);
  }

  template <typename T>
  static constexpr auto
  make(rectangle_properties const& rprops, color_t, T const &data)
  {
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tr = data[2];
    auto const &tl = data[3];
    color_d const bottom_left{bl[0], bl[1], bl[2], bl[3]};
    color_d const bottom_right{br[0], br[1], br[2], br[3]};
    color_d const top_right{tr[0], tr[1], tr[2], tr[3]};
    color_d const top_left{tl[0], tl[1], tl[2], tl[3]};

    color_properties const p{bottom_left, bottom_right, top_right, top_left};
    return construct(rprops, p);
  }

  static constexpr auto make(rectangle_properties const& rprops, color_t, std::array<float, 4> const &color)
  {
    color_properties const p{color};
    return construct(rprops, p);
  }

  static constexpr auto make(rectangle_properties const& rprops, color_t, std::array<float, 3> const &c)
  {
    auto constexpr ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return make(rprops, color_t{}, color);
  }

  static constexpr auto make(rectangle_properties const& rprops)
  {
    auto const color = LIST_OF_COLORS::RED;
    return make(rprops, color_t{}, color);
  }

  static constexpr auto
  make(rectangle_properties const& rprops, uv_t)
  {
    uv_properties const p;
    return construct(rprops, p);
  }

  static constexpr auto
  make(rectangle_properties const& rprops, wireframe_t)
  {
    wireframe_properties const p;
    return construct(rprops, p);
  }
};

template <typename V>
struct polygon : public shape {
  stlw::sized_buffer<V> vertex_attributes;
  int num_vertices() const { return this->vertex_attributes.length(); }
  friend struct polygon_factory;

private:
  explicit polygon(enum draw_mode const dm, struct model const &m, int const num_vertices)
      : shape(dm, m)
      , vertex_attributes(num_vertices)
  {
  }
};

struct polygon_properties
{
  draw_mode const draw_mode;
  model const &model;
  int const num_vertices;

  float const width = 0.25f;
};

class polygon_factory
{
  polygon_factory() = delete;

  struct color_properties {
    color_d colors;
  };

  struct uv_properties {
    float const alpha = 1.0f;
  };

  struct wireframe_properties {
    float const alpha = 1.0f;
  };

  template<typename R, typename P, typename FN>
  static auto
  construct_polygon(polygon_properties const pprops, P const& cprops, FN const& fill_vertice)
  {
    float const width = pprops.width;
    auto const num_vertices = pprops.num_vertices;

    auto const C = num_vertices;     // Assume for now #colors == #vertices
    auto const E = num_vertices + 1; // num_edges
    auto const t = pprops.model.translation;

    auto const cosfn = [&width, &t, &E](auto const a) {
      auto const pos = width * static_cast<float>(std::cos(2 * M_PI * a / E));
      return t.x + pos;
    };
    auto const sinfn = [&width, &t, &E](auto const a) {
      auto const pos = width * static_cast<float>(std::sin(2 * M_PI * a / E));
      return t.y + pos;
    };

    polygon<R> poly{pprops.draw_mode, pprops.model, num_vertices};
    for (auto i{0} ; i < num_vertices; ++i) {
      auto const x = cosfn(i);
      auto const y = sinfn(i);

      vertex_d const v{x, y, t.z, 1.0f};
      fill_vertice(poly, v, i);
    }
    return poly;
  }

  static auto construct(polygon_properties const pprops, color_properties const &cprops)
  {
    using R = vertex_color_attributes;
    auto const fill_vertice = [&](auto &poly, auto const& v, auto const i) {
      poly.vertex_attributes[i] = R{v, cprops.colors};
    };
    return construct_polygon<R>(pprops, cprops, fill_vertice);
  }

  static auto construct(polygon_properties const pprops, uv_properties const &cprops)
  {
    using R = vertex_uv_attributes;
    auto const fill_vertice = [&](auto &poly, auto const& v, auto const i) {
      uv_d const uv{v.x, v.y};
      poly.vertex_attributes[i] = vertex_uv_attributes{v, uv};
    };
    return construct_polygon<R>(pprops, cprops, fill_vertice);
  }

  static auto construct(polygon_properties const pprops, wireframe_properties const &cprops)
  {
    using R = vertex_attributes_only;
    auto const fill_vertice = [&](auto &poly, auto const& v, auto const i) {
      poly.vertex_attributes[i] = R{v};
    };
    return construct_polygon<R>(pprops, cprops, fill_vertice);
  }

public:
  static auto
  make(polygon_properties const pprop, color_t, float const r, float const g, float const b, float const a)
  {
    color_properties const cprop{{r, g, b, a}};
    return construct(pprop, cprop);
  }

  static auto
  make(polygon_properties const pprop, color_t, float const r, float const g, float const b)
  {
    color_properties const cprop{{r, g, b}};
    return construct(pprop, cprop);
  }

  static auto
  make(polygon_properties const props, color_t, std::array<float, 3> const& c)
  {
    return make(props, color_t{}, c[0], c[1], c[2]);
  }

  static auto make(polygon_properties const props, color_t)
  {
    return make(props, color_t{}, LIST_OF_COLORS::LAWN_GREEN);
  }

  static auto make(polygon_properties const props, uv_t)
  {
    uv_properties const uv;
    return construct(props, uv);
  }

  static auto make(polygon_properties const props, wireframe_t)
  {
    wireframe_properties const wf;
    return construct(props, wf);
  }
};

} // ns gfx

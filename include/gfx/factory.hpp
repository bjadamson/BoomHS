#pragma once
#include <array>
#include <stlw/type_macros.hpp>
#include <gfx/types.hpp>
#include <gfx/shape.hpp>
#include <gfx/shape2d.hpp>
#include <gfx/shape3d.hpp>

namespace gfx
{

struct triangle_properties
{
  draw_mode const draw_mode;
  model const &model;
  float const radius = 0.25;
};

template<typename C>
class triangle_factory
{
  C &context_;

  struct color_properties {
    std::array<float, 4> const &color_bottom_left;
    std::array<float, 4> const &color_bottom_right = color_bottom_left;
    std::array<float, 4> const &color_top_middle = color_bottom_left;
  };

  struct uv_properties {
    // clang-format off
    std::array<uv_d, 3> const uv = {
      uv_d{0.0f, 0.0f}, // bottom-left
      uv_d{1.0f, 0.0f}, // bottom-right
      uv_d{0.5f, 1.0f}  // top-middle
    };
    // clang-format on
  };

  struct wireframe_properties {};

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
  explicit triangle_factory(C &c) : context_(c) {}
  MOVE_CONSTRUCTIBLE_ONLY(triangle_factory);
  auto make(triangle_properties const& tprops, color_t, std::array<float, 4> const &c) const
  {
    color_properties const p{c};
    return triangle_factory::construct(tprops, p);
  }

  auto make(triangle_properties const& tprops, color_t, std::array<float, 3> const &c) const
  {
    auto constexpr ALPHA = 1.0f;
    auto const color = std::array<float, 4>{c[0], c[1], c[2], ALPHA};
    color_properties const p{color, color, color};
    return triangle_factory::construct(tprops, p);
  }

  auto make(triangle_properties const& tprops, color_t, glm::vec4 const& c) const
  {
    return make(tprops, color_t{}, std::array<float, 4>{c[0], c[1], c[2], c[3]});
  }

  auto make(triangle_properties const& tprops, color_t) const
  {
    return make(tprops, color_t{}, LIST_OF_COLORS::RED);
  }

  template <typename T>
  auto make(triangle_properties const& tprops, color_t, T const &data) const
  {
    auto const &bl = data[0];
    auto const &br = data[1];
    auto const &tm = data[2];
    std::array<float, 4> const bottom_left{bl[0], bl[1], bl[2], bl[3]};
    std::array<float, 4> const bottom_right{br[0], br[1], br[2], br[3]};
    std::array<float, 4> const top_middle{tm[0], tm[1], tm[2], tm[3]};

    color_properties const p{bottom_left, bottom_right, top_middle};
    return triangle_factory::construct(tprops, p);
  }

  auto make(triangle_properties const& tprops, uv_t) const
  {
    uv_properties const p;
    return triangle_factory::construct(tprops, p);
  }

  auto make(triangle_properties const& tprops, wireframe_t) const
  {
    wireframe_properties const p;
    return triangle_factory::construct(tprops, p);
  }
};

template<typename C>
auto
make_tf(C &c) { return triangle_factory<C>{c}; }

struct rectangle_properties
{
  draw_mode const draw_mode;
  model const &model;
  height_width const dimensions = {0.39f, 0.25f};
};

template<typename C>
class rectangle_factory
{
  C &context_;

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
  explicit rectangle_factory(C &c) : context_(c) {}
  MOVE_CONSTRUCTIBLE_ONLY(rectangle_factory);

  constexpr auto
  make(rectangle_properties const& rprops, color_t, color_properties const& cprops) const
  {
    return construct(rprops, cprops);
  }

  template <typename T>
  constexpr auto
  make(rectangle_properties const& rprops, color_t, T const &data) const
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

  constexpr auto make(rectangle_properties const& rprops, color_t, std::array<float, 4> const &color) const
  {
    color_properties const p{color};
    return construct(rprops, p);
  }

  constexpr auto make(rectangle_properties const& rprops, color_t, std::array<float, 3> const &c) const
  {
    auto constexpr ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return make(rprops, color_t{}, color);
  }

  constexpr auto make(rectangle_properties const& rprops) const
  {
    auto const color = LIST_OF_COLORS::RED;
    return make(rprops, color_t{}, color);
  }

  constexpr auto
  make(rectangle_properties const& rprops, uv_t) const
  {
    uv_properties const p;
    return construct(rprops, p);
  }

  constexpr auto
  make(rectangle_properties const& rprops, wireframe_t) const
  {
    wireframe_properties const p;
    return construct(rprops, p);
  }
};

struct polygon_properties
{
  draw_mode const draw_mode;
  model const &model;
  int const num_vertices;

  float const width = 0.25f;
};

template<typename C>
class polygon_factory
{
  C &context_;

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
  explicit polygon_factory(C &c) : context_(c) {}
  MOVE_CONSTRUCTIBLE_ONLY(polygon_factory);

  auto
  make(polygon_properties const pprop, color_t, float const r, float const g, float const b, float const a) const
  {
    color_properties const cprop{{r, g, b, a}};
    return construct(pprop, cprop);
  }

  auto
  make(polygon_properties const pprop, color_t, float const r, float const g, float const b) const
  {
    color_properties const cprop{{r, g, b}};
    return construct(pprop, cprop);
  }

  auto
  make(polygon_properties const props, color_t, std::array<float, 3> const& c) const
  {
    return make(props, color_t{}, c[0], c[1], c[2]);
  }

  auto make(polygon_properties const props, color_t) const
  {
    return make(props, color_t{}, LIST_OF_COLORS::LAWN_GREEN);
  }

  auto make(polygon_properties const props, uv_t) const
  {
    uv_properties const uv;
    return construct(props, uv);
  }

  auto make(polygon_properties const props, wireframe_t) const
  {
    wireframe_properties const wf;
    return construct(props, wf);
  }
};

struct cube_properties
{
  draw_mode const draw_mode;
  model const &model;
  width_height_length const dimensions;
};

template<typename C>
class cube_factory
{
  C &context_;
  using width_height_length = width_height_length;

  struct color_properties {
    using c = std::array<float, 4>;
    std::array<c, 8> const colors;
  };

  struct uv_properties {
  };

  struct wireframe_properties {
    float const alpha = 1.0f;
  };

  static constexpr auto
  calculate_vertices(width_height_length const &hw)
  {
    auto const h = hw.height;
    auto const w = hw.width;
    auto const l = hw.length;

    // clang-format off
    return std::array<vertex_d, 8> {
      vertex_d{-w, -h, l, 1.0f}, // front bottom-left
      vertex_d{ w, -h, l, 1.0f}, // front bottom-right
      vertex_d{ w,  h, l, 1.0f}, // front top-right
      vertex_d{-w,  h, l, 1.0f}, // front top-left

      vertex_d{-w, -h, -l, 1.0f}, // back bottom-left
      vertex_d{ w, -h, -l, 1.0f}, // back bottom-right
      vertex_d{ w, h,  -l, 1.0f}, // back top-right
      vertex_d{-w, h,  -l, 1.0f}  // back top-left
    };
    // clang-format on
  }

  static constexpr auto construct(cube_properties const& cube_props, color_properties const &props)
  {
    auto const vertices = calculate_vertices(cube_props.dimensions);

    // clang-format off
    vertex_color_attributes const f_bottom_left  {vertices[0], color_d{props.colors[0]}};
    vertex_color_attributes const f_bottom_right {vertices[1], color_d{props.colors[1]}};
    vertex_color_attributes const f_top_right    {vertices[2], color_d{props.colors[2]}};
    vertex_color_attributes const f_top_left     {vertices[3], color_d{props.colors[3]}};

    vertex_color_attributes const b_bottom_left  {vertices[4], color_d{props.colors[4]}};
    vertex_color_attributes const b_bottom_right {vertices[5], color_d{props.colors[5]}};
    vertex_color_attributes const b_top_right    {vertices[6], color_d{props.colors[6]}};
    vertex_color_attributes const b_top_left     {vertices[7], color_d{props.colors[7]}};

    auto arr = stlw::make_array<vertex_color_attributes>(
        f_bottom_left, f_bottom_right, f_top_right, f_top_left,
        b_bottom_left, b_bottom_right, b_top_right, b_top_left);
    return cube<vertex_color_attributes>{cube_props.draw_mode, cube_props.model, std::move(arr)};
    // clang-format on
  }

  static constexpr auto construct(cube_properties const& cube_props, uv_properties const &props)
  {
    auto const vertices = calculate_vertices(cube_props.dimensions);

    // clang-format off
    vertex_attributes_only const f_bottom_left  {vertices[0]};
    vertex_attributes_only const f_bottom_right {vertices[1]};
    vertex_attributes_only const f_top_right    {vertices[2]};
    vertex_attributes_only const f_top_left     {vertices[3]};

    vertex_attributes_only const b_bottom_left  {vertices[4]};
    vertex_attributes_only const b_bottom_right {vertices[5]};
    vertex_attributes_only const b_top_right    {vertices[6]};
    vertex_attributes_only const b_top_left     {vertices[7]};

    auto arr = stlw::make_array<vertex_attributes_only>(
        f_bottom_left, f_bottom_right, f_top_right, f_top_left,
        b_bottom_left, b_bottom_right, b_top_right, b_top_left);
    return cube<vertex_attributes_only>{cube_props.draw_mode, cube_props.model, std::move(arr)};
    // clang-format on
  }

  static constexpr auto construct(cube_properties const& cube_props, wireframe_properties const &props)
  {
    auto const vertices = calculate_vertices(cube_props.dimensions);

    // clang-format off
    vertex_attributes_only const f_bottom_left  {vertices[0]};
    vertex_attributes_only const f_bottom_right {vertices[1]};
    vertex_attributes_only const f_top_right    {vertices[2]};
    vertex_attributes_only const f_top_left     {vertices[3]};

    vertex_attributes_only const b_bottom_left  {vertices[4]};
    vertex_attributes_only const b_bottom_right {vertices[5]};
    vertex_attributes_only const b_top_right    {vertices[6]};
    vertex_attributes_only const b_top_left     {vertices[7]};

    auto arr = stlw::make_array<vertex_attributes_only>(
        f_bottom_left, f_bottom_right, f_top_right, f_top_left,
        b_bottom_left, b_bottom_right, b_top_right, b_top_left);
    return cube<vertex_attributes_only>{cube_props.draw_mode, cube_props.model, std::move(arr)};
    // clang-format on
  }

public:
  explicit cube_factory(C &c) : context_(c) {}
  MOVE_CONSTRUCTIBLE_ONLY(cube_factory);
  constexpr auto make_spotted(cube_properties const& cube_props, color_t,
      std::array<float, 3> const &c) const
  {
    // TODO: this may be an advanced color function, IDK...
    auto const ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};

    std::array<typename color_properties::c, 8> const colors{
        std::array<float, 4>{1.0f, 0.0f, 0.0f, ALPHA}, color,
        std::array<float, 4>{0.0f, 1.0f, 0.0f, ALPHA}, color,
        std::array<float, 4>{0.2f, 0.5f, 0.2f, ALPHA}, color,
        std::array<float, 4>{0.6f, 0.4f, 0.8f, ALPHA}};
    color_properties const p{colors};
    return construct(cube_props, p);
  }

  constexpr auto make_solid(cube_properties const& cube_props, color_t, std::array<float, 3> const &c) const
  {
    // TODO: this may be an advanced color function, IDK...
    auto const ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};

    std::array<typename color_properties::c, 8> const colors{
        color, color,
        color, color,
        color, color,
        color, color};
    color_properties const p{colors};
    return construct(cube_props, p);
  }

  constexpr auto make(cube_properties const& cube_props, uv_t) const
  {
    uv_properties const uv;
    return construct(cube_props, uv);
  }

  constexpr auto make(cube_properties const& cube_props, wireframe_t) const
  {
    wireframe_properties const wf;
    return construct(cube_props, wf);
  }
};


namespace factories
{

#define DEFINE_FACTORY_METHODS(factory_type)                                                       \
  template<typename ...Args>                                                                       \
  auto constexpr make_triangle(triangle_properties const& properties, Args &&... args)             \
  {                                                                                                \
    auto const tf = make_tf(this->ctx_.backend());                                                 \
    return tf.make(properties, factory_type{}, std::forward<Args>(args)...);                       \
  }                                                                                                \
                                                                                                   \
  template<typename ...Args>                                                                       \
  auto constexpr make_rectangle(rectangle_properties const& properties, Args &&... args)           \
  {                                                                                                \
    auto const rf = make_rf(this->ctx_.backend());                                                 \
    return rf.make(properties, factory_type{}, std::forward<Args>(args)...);                       \
  }                                                                                                \
                                                                                                   \
  template<typename ...Args>                                                                       \
  auto constexpr make_polygon(polygon_properties const& properties, Args &&... args)               \
  {                                                                                                \
    auto const pf = make_pf(this->ctx_.backend());                                                 \
    return pf.make(properties, factory_type{}, std::forward<Args>(args)...);                       \
  }                                                                                                \
                                                                                                   \
  template<typename ...Args>                                                                       \
  auto constexpr make_cube(cube_properties const& properties, Args &&... args)                     \
  {                                                                                                \
    auto const cf = make_cf(this->ctx_.backend());                                                 \
    return cf.make(properties, factory_type{}, std::forward<Args>(args)...);                       \
  }

template<typename C>
class color
{
  C ctx_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(color);
  explicit constexpr color(C &&c) : ctx_(std::move(c)) {}

  DEFINE_FACTORY_METHODS(color_t);
};

template<typename C>
class uv
{
  C ctx_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(uv);
  explicit constexpr uv(C &&c) : ctx_(std::move(c)) {}

  DEFINE_FACTORY_METHODS(uv_t);
};

template<typename C>
class wireframe
{
  C ctx_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(wireframe);
  explicit constexpr wireframe(C &&c) : ctx_(std::move(c)) {}

  DEFINE_FACTORY_METHODS(wireframe_t);
};

#undef DEFINE_FACTORY_METHODS

} // ns factories

template<typename T>
class shape_factory
{
  rectangle_factory<T> rf_;
  polygon_factory<T> pf_;
  cube_factory<T> cf_;
public:
  explicit shape_factory(rectangle_factory<T> &&rf, polygon_factory<T> &&pf, cube_factory<T> &&cf)
    : rf_(std::move(rf))
    , pf_(std::move(pf))
    , cf_(std::move(cf))
  {}

  MOVE_CONSTRUCTIBLE_ONLY(shape_factory);

  template<typename ...Args>
  auto make_rectangle(rectangle_properties const& properties, Args &&... args) const
  {
    return this->rf_.make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_polygon(polygon_properties const& properties, Args &&... args) const
  {
    return this->pf_.make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_spotted_cube(cube_properties const& properties, Args &&... args) const
  {
    return this->cf_.make_spotted(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_color_cube(cube_properties const& properties, Args &&... args) const
  {
    return this->cf_.make_solid(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_textured_cube(cube_properties const& properties, Args &&... args) const
  {
    return this->cf_.make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_wireframe_cube(cube_properties const& properties, Args &&... args) const
  {
    return this->cf_.make(properties, std::forward<Args>(args)...);
  }
};

template<typename C>
auto make_shape_factory(C &context)
{
  return shape_factory<C>{rectangle_factory<C>{context}, polygon_factory<C>{context},
    cube_factory<C>{context}};
}

template<typename C>
auto make_color_factory(C &&context)
{
  return factories::color<C>{std::move(context)};
}

template<typename C>
auto make_uv_factory(C &&context)
{
  return factories::uv<C>{std::move(context)};
}

template<typename C>
auto make_wireframe_factory(C &&context)
{
  return factories::wireframe<C>{std::move(context)};
}

} // ns gfx

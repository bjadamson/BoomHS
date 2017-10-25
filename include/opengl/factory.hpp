#pragma once
#include <array>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>
#include <opengl/glew.hpp>
#include <opengl/obj.hpp>
#include <opengl/types.hpp>
#include <opengl/shape.hpp>
#include <opengl/shape2d.hpp>
#include <opengl/shape3d.hpp>

namespace opengl
{

struct triangle_properties
{
  GLenum const draw_mode;
  model const &model;
  float const radius = 0.25;
};

class triangle_factory
{
  struct color_properties {
    std::array<float, 4> const &color_bottom_left;
    std::array<float, 4> const &color_bottom_right = color_bottom_left;
    std::array<float, 4> const &color_top_middle = color_bottom_left;
  };

  struct uv_properties {
    // clang-format off
    std::array<float, 6> const uv = {
      0.0f, 0.0f, // bottom-left
      1.0f, 0.0f, // bottom-right
      0.5f, 1.0f  // top-middle
    };
    // clang-format on
  };

  struct wireframe_properties {};

  static constexpr auto calculate_vertices(glm::vec3 const &m, float const radius)
  {
    std::array<float, 12> const vertices = {
        m.x - radius, m.y - radius, m.z, 1.0f, // bottom-left
        m.x + radius, m.y - radius, m.z, 1.0f, // bottom-right
        m.x, m.y + radius, m.z, 1.0f           // top-middle
    };
    return vertices;
  }

  static auto construct(triangle_properties const& tprops, color_properties const &cprops)
  {
    auto const vertices = calculate_vertices(tprops.model.translation, tprops.radius);
    auto const& c0 = cprops.color_bottom_left;
    auto const& c1 = cprops.color_bottom_right;
    auto const& c2 = cprops.color_top_middle;

    auto arr = stlw::make_array<float>(
        vertices[0], vertices[1], vertices[2], vertices[3], c0[0], c0[1], c0[2], c0[3],
        vertices[4], vertices[5], vertices[6], vertices[7], c1[0], c1[1], c1[2], c1[3],
        vertices[8], vertices[9], vertices[10], vertices[11], c2[0], c2[1], c2[2], c2[3]
        );
    return triangle<vertex_color_attributes, arr.size()>{tprops.draw_mode, tprops.model, MOVE(arr)};
  }

  static auto construct(triangle_properties const& tprops, uv_properties const &cprops)
  {
    auto const vertices = calculate_vertices(tprops.model.translation, tprops.radius);
    auto const& uv = cprops.uv;

    auto arr = stlw::make_array<float>(
        vertices[0], vertices[1], vertices[2], vertices[3], uv[0], uv[1],
        vertices[4], vertices[5], vertices[6], vertices[7], uv[2], uv[3],
        vertices[8], vertices[9], vertices[10], vertices[11], uv[4], uv[5]
        );

    return triangle<vertex_uv_attributes, arr.size()>{tprops.draw_mode, tprops.model, MOVE(arr)};
  }

  static auto construct(triangle_properties const& tprops, wireframe_properties const &cprops)
  {
    auto const vertices = calculate_vertices(tprops.model.translation, tprops.radius);

    auto arr = stlw::make_array<float>(
        vertices[0], vertices[1], vertices[2], vertices[3],
        vertices[4], vertices[5], vertices[6], vertices[7],
        vertices[8], vertices[9], vertices[10], vertices[11]
        );
    return triangle<vertex_attributes_only, arr.size()>{tprops.draw_mode, tprops.model, MOVE(arr)};
  }

public:
  triangle_factory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(triangle_factory);
  auto make(triangle_properties const& tprops, color_t, std::array<float, 4> const &c)
  {
    color_properties const p{c};
    return triangle_factory::construct(tprops, p);
  }

  auto make(triangle_properties const& tprops, color_t, std::array<float, 3> const &c)
  {
    auto constexpr ALPHA = 1.0f;
    auto const color = std::array<float, 4>{c[0], c[1], c[2], ALPHA};
    color_properties const p{color, color, color};
    return triangle_factory::construct(tprops, p);
  }

  auto make(triangle_properties const& tprops, color_t, glm::vec4 const& c)
  {
    return make(tprops, color_t{}, std::array<float, 4>{c[0], c[1], c[2], c[3]});
  }

  auto make(triangle_properties const& tprops, color_t)
  {
    return make(tprops, color_t{}, LIST_OF_COLORS::RED);
  }

  template <typename T>
  auto make(triangle_properties const& tprops, color_t, T const &data)
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

  auto make(triangle_properties const& tprops, uv_t)
  {
    uv_properties const p;
    return triangle_factory::construct(tprops, p);
  }

  auto make(triangle_properties const& tprops, wireframe_t)
  {
    wireframe_properties const p;
    return triangle_factory::construct(tprops, p);
  }
};

struct rectangle_properties
{
  GLenum const draw_mode;
  model const &model;
  height_width const dimensions = {0.39f, 0.25f};
};

class rectangle_factory
{
  using height_width = height_width;
  struct color_properties {
    color_d const &bottom_left;
    color_d const &bottom_right = bottom_left;
    color_d const &top_right = bottom_left;
    color_d const &top_left = bottom_left;
  };

  struct uv_properties {
    // clang-format off
    std::array<float, 8> const uv = {
      0.0f, 0.0f, // bottom-left
      1.0f, 0.0f, // bottom-right
      1.0f, 1.0f, // top-right
      0.0f, 1.0f, // top-left
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

    auto const bl = stlw::make_array<float>(m.x - width, m.y - height, m.z, 1.0f);
    auto const br = stlw::make_array<float>(m.x + width, m.y - height, m.z, 1.0f);
    auto const tr = stlw::make_array<float>(m.x + width, m.y + height, m.z, 1.0f);
    auto const tl = stlw::make_array<float>(m.x - width, m.y + height, m.z, 1.0f);

#define RECT_VERTICES_0 bl[0], bl[1], bl[2], bl[3]
#define RECT_VERTICES_1 br[0], br[1], br[2], br[3]
#define RECT_VERTICES_2 tr[0], tr[1], tr[2], tr[3]
#define RECT_VERTICES_3 tl[0], tl[1], tl[2], tl[3]

    // Two triangles produce the rectangle.
    return stlw::make_array<float>(
        RECT_VERTICES_0,
        RECT_VERTICES_1,
        RECT_VERTICES_2,

        RECT_VERTICES_2,
        RECT_VERTICES_3,
        RECT_VERTICES_0
        );
  }

  static auto construct(rectangle_properties const& rprops, color_properties const &cprops)
  {
    auto const vertices = calculate_vertices(rprops.model.translation, rprops.dimensions);

    auto const& c0 = cprops.bottom_left;
    auto const& c1 = cprops.bottom_right;
    auto const& c2 = cprops.top_right;
    auto const& c3 = cprops.top_left;

    // clang-format off
    // 0, 1, 2, 2, 3, 0
    auto arr = stlw::make_array<float>(
        vertices[0], vertices[1], vertices[2],  vertices[3],  c0.r, c0.g, c0.b, c0.a,
        vertices[4], vertices[5], vertices[6],  vertices[7],  c1.r, c1.g, c1.b, c1.a,
        vertices[8], vertices[9], vertices[10], vertices[11], c2.r, c2.g, c2.b, c2.a,

        vertices[12], vertices[13], vertices[14], vertices[15], c2.r, c2.g, c2.b, c2.a,
        vertices[16], vertices[17], vertices[18], vertices[19], c3.r, c3.g, c3.b, c3.a,
        vertices[20], vertices[21], vertices[22], vertices[23], c0.r, c0.g, c0.b, c0.a
        );
    // clang-format on

    return rectangle<vertex_color_attributes, arr.size()>{rprops.draw_mode, rprops.model, MOVE(arr)};
  }

  static auto construct(rectangle_properties const& rprops, uv_properties const &props)
  {
    auto const vertices = calculate_vertices(rprops.model.translation, rprops.dimensions);

    auto const& uv = props.uv;
    auto const bl = stlw::make_array<float>(uv[0], uv[1]);
    auto const br = stlw::make_array<float>(uv[2], uv[3]);
    auto const tr = stlw::make_array<float>(uv[4], uv[5]);
    auto const tl = stlw::make_array<float>(uv[6], uv[7]);

    auto arr = stlw::make_array<float>(
        vertices[0], vertices[1], vertices[2], vertices[3], bl[0], bl[1],
        vertices[4], vertices[5], vertices[6], vertices[7], br[0], br[1],
        vertices[8], vertices[9], vertices[10], vertices[11], tr[0], tr[1],

        vertices[12], vertices[13], vertices[14], vertices[15], tr[0], tr[1],
        vertices[16], vertices[17], vertices[18], vertices[19], tl[0], tl[1],
        vertices[20], vertices[21], vertices[22], vertices[23], bl[0], bl[1]
        );
    return rectangle<vertex_uv_attributes, arr.size()>{rprops.draw_mode, rprops.model, MOVE(arr)};
  }

  static auto construct(rectangle_properties const& rprops, wireframe_properties const &props)
  {
    auto const vertices = calculate_vertices(rprops.model.translation, rprops.dimensions);

    auto arr = stlw::make_array<float>(
        vertices[0], vertices[1], vertices[2], vertices[3],
        vertices[4], vertices[5], vertices[6], vertices[7],
        vertices[8], vertices[9], vertices[10], vertices[11],

        vertices[12], vertices[13], vertices[14], vertices[15],
        vertices[16], vertices[17], vertices[18], vertices[19],
        vertices[20], vertices[21], vertices[22], vertices[23]
        );
    return rectangle<vertex_attributes_only, arr.size()>{rprops.draw_mode, rprops.model, MOVE(arr)};
  }

public:
  rectangle_factory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(rectangle_factory);

  auto
  make(rectangle_properties const& rprops, color_t, color_properties const& cprops)
  {
    return construct(rprops, cprops);
  }

  template <typename T>
  auto
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

  auto make(rectangle_properties const& rprops, color_t, std::array<float, 4> const &color)
  {
    color_properties const p{color};
    return construct(rprops, p);
  }

  auto make(rectangle_properties const& rprops, color_t, std::array<float, 3> const &c)
  {
    auto constexpr ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return make(rprops, color_t{}, color);
  }

  auto make(rectangle_properties const& rprops)
  {
    auto const color = LIST_OF_COLORS::RED;
    return make(rprops, color_t{}, color);
  }

  auto
  make(rectangle_properties const& rprops, uv_t)
  {
    uv_properties const p;
    return construct(rprops, p);
  }

  auto
  make(rectangle_properties const& rprops, wireframe_t)
  {
    wireframe_properties const p;
    return construct(rprops, p);
  }
};

struct polygon_properties
{
  GLenum const draw_mode;
  model const &model;
  int const num_vertices;

  float const width = 0.25f;
};

class polygon_factory
{
  struct color_properties {
    color_d colors;
  };

  struct uv_properties {
    float const alpha = 1.0f;
  };

  struct wireframe_properties {
    float const alpha = 1.0f;
  };

  template<typename R, typename FN>
  static auto
  construct_polygon(polygon_properties const pprops, int const floats_per_vertice,
      FN const& fill_after_positions)
  {
    float const width = pprops.width;
    auto const num_vertices = pprops.num_vertices;

    auto const num_edges = num_vertices + 1;
    auto const t = pprops.model.translation;

    auto const calc_angle = [&num_edges](auto const angle) {
      return 2 * M_PI * angle / num_edges;
    };

    auto const cosfn = [&](auto const a) {
      auto const angle = static_cast<float>(std::cos(calc_angle(a)));
      auto const pos = width * angle;
      return t.x + pos;
    };
    auto const sinfn = [&](auto const a) {
      auto const angle = static_cast<float>(std::sin(calc_angle(a)));
      auto const pos = width * angle;
      return t.y + pos;
    };

    auto const num_floats = num_vertices * floats_per_vertice;
    polygon<R> poly{pprops.draw_mode, pprops.model, num_vertices, num_floats};

    int vertices_filled{0};
    std::cerr << "num_vertices '" << num_vertices << "'\n";
    std::cerr << "num_floats '" << num_floats << "'\n";
    std::cerr << "floats_per_vertice '" << floats_per_vertice << "'\n";
    FOR(i, num_floats) {
      std::cerr << "poly.vertices()[i] is '" << poly.vertices()[i] << "'\n";
    }
    std::cerr << "begin\n";
    for (auto i{0}; i < num_floats;) {
      auto const x = cosfn(i);
      auto const y = sinfn(i);

      poly.vertices()[i++] = x;
      poly.vertices()[i++] = y;
      poly.vertices()[i++] = t.z;
      poly.vertices()[i++] = 1.0;

      fill_after_positions(poly, i);
      vertices_filled += floats_per_vertice;

      if (i == num_floats) {
        std::cerr << "i == num_floats\n";
      }
    }
    FOR(i, num_floats) {
      std::cerr << "poly.vertices()[" << i << "] is '" << poly.vertices()[i] << "'\n";
    }
    assert(vertices_filled == num_floats);
    std::cerr << "end\n";
    std::cerr << "vertices_filled '" << vertices_filled << "'\n";
    return poly;
  }

  static auto construct(polygon_properties const pprops, color_properties const &cprops)
  {
    using R = vertex_color_attributes;

    auto const fill_color = [&](auto &poly, auto &i) {
      poly.vertices()[i++] = cprops.colors.r;
      poly.vertices()[i++] = cprops.colors.g;
      poly.vertices()[i++] = cprops.colors.b;
      poly.vertices()[i++] = cprops.colors.a;
    };

    // 4 for position, 4 for colors
    static auto constexpr FLOATS_PER_VERTICE = 4 + 4; // TODO: can we read this from somewhere??
    return construct_polygon<R>(pprops, FLOATS_PER_VERTICE, fill_color);
  }

  static auto construct(polygon_properties const pprops, uv_properties const &cprops)
  {
    using R = vertex_uv_attributes;
    auto const fill_uv = [&](auto &poly, auto &i) {
      poly.vertices()[i++] = 1.0;
      poly.vertices()[i++] = 1.0;
    };
    // 4 for position, 2 for uv
    static auto constexpr FLOATS_PER_VERTICE = 4 + 2; // TODO: can we read this from somewhere??
    return construct_polygon<R>(pprops, FLOATS_PER_VERTICE, fill_uv);
  }

  static auto construct(polygon_properties const pprops, wireframe_properties const &cprops)
  {
    using R = vertex_attributes_only;
    auto const no_op = [&](auto &poly, auto &i) {
      // Nothing, wire-frame color gets passed in as a uniform var.
    };
    // 4 for position only
    static auto constexpr FLOATS_PER_VERTICE = 4; // TODO: can we read this from somewhere??
    return construct_polygon<R>(pprops, FLOATS_PER_VERTICE, no_op);
  }

public:
  polygon_factory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(polygon_factory);

  auto
  make(polygon_properties const pprop, color_t, float const r, float const g, float const b, float const a)
  {
    color_properties const cprop{{r, g, b, a}};
    return construct(pprop, cprop);
  }

  auto
  make(polygon_properties const pprop, color_t, float const r, float const g, float const b)
  {
    color_properties const cprop{{r, g, b}};
    return construct(pprop, cprop);
  }

  auto
  make(polygon_properties const props, color_t, std::array<float, 3> const& c)
  {
    return make(props, color_t{}, c[0], c[1], c[2]);
  }

  auto make(polygon_properties const props, color_t)
  {
    return make(props, color_t{}, LIST_OF_COLORS::LAWN_GREEN);
  }

  auto make(polygon_properties const props, uv_t)
  {
    uv_properties const uv;
    return construct(props, uv);
  }

  auto make(polygon_properties const props, wireframe_t)
  {
    wireframe_properties const wf;
    return construct(props, wf);
  }
};

struct mesh_properties
{
  GLenum const draw_mode;
  model const &model;

  obj const& object_data;
};

class mesh_factory
{
  static auto
  construct_mesh(mesh_properties const mprops)
  {
    return mesh<vertex_normal_uv_attributes>{mprops.draw_mode, mprops.model, mprops.object_data};
  }

public:
  mesh_factory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(mesh_factory);

  auto
  make(mesh_properties const mprop, uv_t)
  {
    return construct_mesh(mprop);
  }
};

struct cube_properties
{
  GLenum const draw_mode;
  model const &model;
  width_height_length const dimensions;
};

class cube_factory
{
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
    auto const arr = stlw::make_array<float>(
      -w, -h, l, 1.0f, // front bottom-left
       w, -h, l, 1.0f, // front bottom-right
       w,  h, l, 1.0f, // front top-right
      -w,  h, l, 1.0f, // front top-left

      -w, -h, -l, 1.0f, // back bottom-left
       w, -h, -l, 1.0f, // back bottom-right
       w,  h, -l, 1.0f, // back top-right
      -w,  h, -l, 1.0f  // back top-left
    );

#define CUBE_ROW_0 arr[0], arr[1], arr[2], arr[3]
#define CUBE_ROW_1 arr[4], arr[5], arr[6], arr[7]
#define CUBE_ROW_2 arr[8], arr[9], arr[10], arr[11]
#define CUBE_ROW_3 arr[12], arr[13], arr[14], arr[15]
#define CUBE_ROW_4 arr[16], arr[17], arr[18], arr[19]
#define CUBE_ROW_5 arr[20], arr[21], arr[22], arr[23]
#define CUBE_ROW_6 arr[24], arr[25], arr[26], arr[27]
#define CUBE_ROW_7 arr[28], arr[29], arr[30], arr[31]
#define CUBE_ROW_8 arr[32], arr[33], arr[34], arr[35]
    return stlw::make_array<float>(
        CUBE_ROW_2,
        CUBE_ROW_3,
        CUBE_ROW_6,
        CUBE_ROW_7,

        CUBE_ROW_1,
        CUBE_ROW_0,
        CUBE_ROW_4,
        CUBE_ROW_5
        );
    // clang-format on
  }

  static auto construct(cube_properties const& cube_props, color_properties const &props)
  {
    auto vertices = calculate_vertices(cube_props.dimensions);

    // clang-format off
    auto const& colors = props.colors;
    auto arr = stlw::make_array<float>(
        vertices[0], vertices[1], vertices[2], vertices[3],
        colors[0][0], colors[0][1], colors[0][2], colors[0][3],

        vertices[4], vertices[5], vertices[6], vertices[7],
        colors[1][0], colors[1][1], colors[1][2], colors[1][3],

        vertices[8], vertices[9], vertices[10], vertices[11],
        colors[2][0], colors[2][1], colors[2][2], colors[2][3],

        vertices[12], vertices[13], vertices[14], vertices[15],
        colors[3][0], colors[3][1], colors[3][2], colors[3][3],

        vertices[16], vertices[17], vertices[18], vertices[19],
        colors[4][0], colors[4][1], colors[4][2], colors[4][3],

        vertices[20], vertices[21], vertices[22], vertices[23],
        colors[5][0], colors[5][1], colors[5][2], colors[5][3],

        vertices[24], vertices[25], vertices[26], vertices[27],
        colors[6][0], colors[6][1], colors[6][2], colors[6][3],

        vertices[28], vertices[29], vertices[30], vertices[31],
        colors[7][0], colors[7][1], colors[7][2], colors[7][3],

        vertices[32], vertices[33], vertices[34], vertices[35],
        colors[8][0], colors[8][1], colors[8][2], colors[8][3]
          );
    return cube<vertex_color_attributes, arr.size()>{cube_props.draw_mode, cube_props.model,
      MOVE(arr)};
    // clang-format on
  }

  static auto construct(cube_properties const& cube_props, uv_properties const &props)
  {
    auto const vertices = calculate_vertices(cube_props.dimensions);

    // clang-format off
    auto arr = stlw::make_array<float>(
        vertices[0], vertices[1], vertices[2], vertices[3],
        vertices[4], vertices[5], vertices[6], vertices[7],
        vertices[8], vertices[9], vertices[10], vertices[11],
        vertices[12], vertices[13], vertices[14], vertices[15],
        vertices[16], vertices[17], vertices[18], vertices[19],
        vertices[20], vertices[21], vertices[22], vertices[23],
        vertices[24], vertices[25], vertices[26], vertices[27],
        vertices[28], vertices[29], vertices[30], vertices[31],
        vertices[32], vertices[33], vertices[34], vertices[35]
        );
    return cube<vertex_attributes_only, arr.size()>{cube_props.draw_mode, cube_props.model,
      MOVE(arr)};
    // clang-format on
  }

  static auto construct(cube_properties const& cube_props, wireframe_properties const &props)
  {
    auto const vertices = calculate_vertices(cube_props.dimensions);

    // clang-format off
    auto arr = stlw::make_array<float>(
        vertices[0], vertices[1], vertices[2], vertices[3],
        vertices[4], vertices[5], vertices[6], vertices[7],
        vertices[8], vertices[9], vertices[10], vertices[11],
        vertices[12], vertices[13], vertices[14], vertices[15],
        vertices[16], vertices[17], vertices[18], vertices[19],
        vertices[20], vertices[21], vertices[22], vertices[23],
        vertices[24], vertices[25], vertices[26], vertices[27],
        vertices[28], vertices[29], vertices[30], vertices[31],
        vertices[32], vertices[33], vertices[34], vertices[35]
        );
    return cube<vertex_attributes_only, arr.size()>{cube_props.draw_mode, cube_props.model,
      MOVE(arr)};
    // clang-format on
  }

public:
  cube_factory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(cube_factory);

  /*
  auto make_spotted(cube_properties const& cube_props, color_t,
      std::array<float, 3> const &c)
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
  */

  auto make(cube_properties const& cube_props, color_t, std::array<float, 3> const &c)
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

  auto make(cube_properties const& cube_props, uv_t)
  {
    uv_properties const uv;
    return construct(cube_props, uv);
  }

  auto make(cube_properties const& cube_props, wireframe_t)
  {
    wireframe_properties const wf;
    return construct(cube_props, wf);
  }
};

namespace factories
{

template<typename S, typename P>
struct pipeline_shape_pair
{
  S shape;
  P &pipeline;

  using PIPE = P;
};

template<typename S, typename P>
auto make_pipeline_shape_pair(S &&s, P &p) { return pipeline_shape_pair<S, P>{MOVE(s), p}; }

#define DEFINE_FACTORY_METHODS(factory_type)                                                       \
  template<typename L, typename ...Args>                                                           \
  auto make_triangle(L &logger, triangle_properties const& properties, Args &&... args)            \
  {                                                                                                \
    auto tf = triangle_factory{};                                                                  \
    auto shape = tf.make(properties, factory_type{}, std::forward<Args>(args)...);                 \
    auto pair = make_pipeline_shape_pair(MOVE(shape), this->pipeline_);                            \
    copy_to_gpu(logger, pair);                                                                     \
    return pair;                                                                                   \
  }                                                                                                \
                                                                                                   \
  template<typename L, typename ...Args>                                                           \
  auto make_rectangle(L &logger, rectangle_properties const& properties, Args &&... args)          \
  {                                                                                                \
    auto rf = rectangle_factory{};                                                                 \
    auto shape = rf.make(properties, factory_type{}, std::forward<Args>(args)...);                 \
    auto pair = make_pipeline_shape_pair(MOVE(shape), this->pipeline_);                            \
    copy_to_gpu(logger, pair);                                                                     \
    return pair;                                                                                   \
  }                                                                                                \
                                                                                                   \
  template<typename L, typename ...Args>                                                           \
  auto make_polygon(L &logger, polygon_properties const& properties, Args &&... args)              \
  {                                                                                                \
    auto pf = polygon_factory{};                                                                   \
    auto shape = pf.make(properties, factory_type{}, std::forward<Args>(args)...);                 \
    auto pair = make_pipeline_shape_pair(MOVE(shape), this->pipeline_);                            \
    copy_to_gpu(logger, pair);                                                                     \
    return pair;                                                                                   \
  }                                                                                                \
                                                                                                   \
  template<typename L, typename ...Args>                                                           \
  auto make_cube(L &logger, cube_properties const& properties, Args &&... args)                    \
  {                                                                                                \
    auto cf = cube_factory{};                                                                      \
    auto shape = cf.make(properties, factory_type{}, std::forward<Args>(args)...);                 \
    auto pair = make_pipeline_shape_pair(MOVE(shape), this->pipeline_);                            \
    copy_to_gpu(logger, pair);                                                                     \
    return pair;                                                                                   \
  }                                                                                                \
  template<typename L, typename ...Args>                                                           \
  auto make_mesh(L &logger, mesh_properties const& properties, Args &&... args)                    \
  {                                                                                                \
    auto mf = mesh_factory{};                                                                      \
    auto shape = mf.make(properties, factory_type{}, std::forward<Args>(args)...);                 \
    auto pair = make_pipeline_shape_pair(MOVE(shape), this->pipeline_);                            \
    copy_to_gpu(logger, pair);                                                                     \
    return pair;                                                                                   \
  }

template<typename P>
class color
{
  P &pipeline_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(color);
  explicit constexpr color(P &p) : pipeline_(p) {}

  DEFINE_FACTORY_METHODS(color_t);
};

template<typename P>
class texture
{
  P &pipeline_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(texture);
  explicit constexpr texture(P &p) : pipeline_(p) {}

  DEFINE_FACTORY_METHODS(uv_t);
};

template<typename P>
class wireframe
{
  P &pipeline_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(wireframe);
  explicit constexpr wireframe(P &p) : pipeline_(p) {}

  DEFINE_FACTORY_METHODS(wireframe_t);
};

#undef DEFINE_FACTORY_METHODS

} // ns factories

template<typename P0, typename P1, typename P2, typename P3>
struct d2_shape_factory
{
  factories::color<P0> color;
  factories::texture<P1> texture_wall;
  factories::texture<P2> texture_container;
  factories::wireframe<P3> wireframe;

  explicit d2_shape_factory(factories::color<P0> &&cf, factories::texture<P1> &&tf0,
      factories::texture<P2> &&tf1, factories::wireframe<P3> &&wf)
    : color(MOVE(cf))
    , texture_wall(MOVE(tf0))
    , texture_container(MOVE(tf1))
    , wireframe(MOVE(wf))
  {}

  MOVE_CONSTRUCTIBLE_ONLY(d2_shape_factory);
};

template<typename P0, typename P1, typename P2, typename P3, typename P4>
struct d3_shape_factory
{
  factories::color<P0> color;
  factories::texture<P1> texture_cube;
  factories::texture<P2> house;
  factories::texture<P3> skybox;
  factories::wireframe<P4> wireframe;

  explicit d3_shape_factory(factories::color<P0> &&cf, factories::texture<P1> &&tf,
      factories::texture<P2> &&house, factories::texture<P3> &&sky,
      factories::wireframe<P4> &&wf)
    : color(MOVE(cf))
    , texture_cube(MOVE(tf))
    , house(MOVE(house))
    , skybox(MOVE(sky))
    , wireframe(MOVE(wf))
  {}

  MOVE_CONSTRUCTIBLE_ONLY(d3_shape_factory);
};

template<typename SF2D, typename SF3D>
struct shape_factories
{
  SF2D d2;
  SF3D d3;
};

template<typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6,
  typename P7, typename P8>
auto make_shape_factories(P0 &p0, P1 &p1, P2 &p2, P3 &p3, P4 &p4, P5 &p5, P6 &p6, P7 &p7, P8 &p8)
{
  auto d2p = d2_shape_factory<P0, P1, P2, P3>{
    factories::color<P0>{p0},
    factories::texture<P1>{p1},
    factories::texture<P2>{p2},
    factories::wireframe<P3>{p3}
  };

  auto d3p = d3_shape_factory<P4, P5, P6, P7, P8>{
    factories::color<P4>{p4},
    factories::texture<P5>{p5},
    factories::texture<P6>{p6},
    factories::texture<P7>{p7},
    factories::wireframe<P8>{p8}
  };

  using SF2D = decltype(d2p);
  using SF3D = decltype(d3p);
  return shape_factories<SF2D, SF3D>{MOVE(d2p), MOVE(d3p)};
}

} // ns opengl

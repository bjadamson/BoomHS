#pragma once
#include <gfx/types.hpp>
#include <gfx/shape.hpp>
#include <array>

namespace gfx
{

template <typename V>
struct cube : public shape {
  static auto constexpr NUM_VERTICES = 8;
  std::array<V, 8> vertices;

private:
  friend class cube_factory;
  explicit constexpr cube(enum draw_mode const dm, struct model const& m, std::array<V, 8> &&v)
      : shape(dm, m)
      , vertices(std::move(v))
  {
  }
};

struct cube_properties
{
  draw_mode const draw_mode;
  model const &model;
  width_height_length const dimensions;
};

class cube_factory
{
  cube_factory() = delete;

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
  static constexpr auto make(cube_properties const& cube_props, color_t, std::array<float, 3> const &c)
  {
    // TODO: this may be an advanced color function, IDK...
    auto const ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};

    std::array<color_properties::c, 8> const colors{
        std::array<float, 4>{1.0f, 0.0f, 0.0f, ALPHA}, color,
        std::array<float, 4>{0.0f, 1.0f, 0.0f, ALPHA}, color,
        std::array<float, 4>{0.2f, 0.5f, 0.2f, ALPHA}, color,
        std::array<float, 4>{0.6f, 0.4f, 0.8f, ALPHA}};
    color_properties const p{colors};
    return construct(cube_props, p);
  }

  static constexpr auto make(cube_properties const& cube_props, uv_t)
  {
    uv_properties const uv;
    return construct(cube_props, uv);
  }

  static constexpr auto make(cube_properties const& cube_props, wireframe_t)
  {
    wireframe_properties const wf;
    return construct(cube_props, wf);
  }
};

} // ns gfx

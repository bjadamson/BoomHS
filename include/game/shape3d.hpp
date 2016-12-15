#pragma once
#include <engine/gfx/types.hpp>
#include <game/shape.hpp>
#include <game/data_types.hpp>
#include <array>

namespace game
{

template <typename V>
struct cube : public shape {
  static auto constexpr NUM_VERTICES = 8;
  std::array<V, 8> vertices;

private:
  friend class cube_factory;
  explicit constexpr cube(drawmode const dm, struct model const& m, std::array<V, 8> &&v)
      : shape(dm, m)
      , vertices(std::move(v))
  {
  }
};

class cube_factory
{
  cube_factory() = delete;

  using height_width_length = ::engine::gfx::height_width_length;

  struct color_properties {
    using c = std::array<float, 4>;

    height_width_length const dimensions;
    std::array<c, 8> const colors;
  };

  struct uv_properties {
    height_width_length const dimensions;
  };

  struct wireframe_properties {
    height_width_length const dimensions;

    float const alpha = 1.0f;
    float const width = 0.25f;
  };

  static constexpr auto
  calculate_vertices(height_width_length const &hw)
  {
    auto const h = hw.height;
    auto const w = hw.width;
    auto const l = hw.length;

    // clang-format off
    return std::array<vertex, 8> {
      vertex{-w, -h, l, 1.0f}, // front bottom-left
      vertex{ w, -h, l, 1.0f}, // front bottom-right
      vertex{ w,  h, l, 1.0f}, // front top-right
      vertex{-w,  h, l, 1.0f}, // front top-left

      vertex{-w, -h, -l, 1.0f}, // back bottom-left
      vertex{ w, -h, -l, 1.0f}, // back bottom-right
      vertex{ w, h,  -l, 1.0f}, // back top-right
      vertex{-w, h,  -l, 1.0f}  // back top-left
    };
    // clang-format on
  }

  static constexpr auto construct(drawmode const dm, model const &m, color_properties const &props)
  {
    auto const vertices = calculate_vertices(props.dimensions);

    // clang-format off
    vertex_color_attributes const f_bottom_left  {vertices[0], color{props.colors[0]}};
    vertex_color_attributes const f_bottom_right {vertices[1], color{props.colors[1]}};
    vertex_color_attributes const f_top_right    {vertices[2], color{props.colors[2]}};
    vertex_color_attributes const f_top_left     {vertices[3], color{props.colors[3]}};

    vertex_color_attributes const b_bottom_left  {vertices[4], color{props.colors[4]}};
    vertex_color_attributes const b_bottom_right {vertices[5], color{props.colors[5]}};
    vertex_color_attributes const b_top_right    {vertices[6], color{props.colors[6]}};
    vertex_color_attributes const b_top_left     {vertices[7], color{props.colors[7]}};

    auto arr = stlw::make_array<vertex_color_attributes>(
        f_bottom_left, f_bottom_right, f_top_right, f_top_left,
        b_bottom_left, b_bottom_right, b_top_right, b_top_left);
    return cube<vertex_color_attributes>{dm, m, std::move(arr)};
    // clang-format on
  }

  static constexpr auto construct(drawmode const dm, model const &m, uv_properties const &props)
  {
    auto const vertices = calculate_vertices(props.dimensions);

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
    return cube<vertex_attributes_only>{dm, m, std::move(arr)};
    // clang-format on
  }

  static constexpr auto construct(drawmode const dm, model const &m, wireframe_properties const &props)
  {
    auto const vertices = calculate_vertices(props.dimensions);

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
    return cube<vertex_attributes_only>{dm, m, std::move(arr)};
    // clang-format on
  }

public:
  static constexpr auto make_spotted(drawmode const dm, model const &m, std::array<float, 3> const &c,
      height_width_length const& hwl)
  {
    auto const ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};

    std::array<color_properties::c, 8> const colors{
        std::array<float, 4>{1.0f, 0.0f, 0.0f, ALPHA}, color,
        std::array<float, 4>{0.0f, 1.0f, 0.0f, ALPHA}, color,
        std::array<float, 4>{0.2f, 0.5f, 0.2f, ALPHA}, color,
        std::array<float, 4>{0.6f, 0.4f, 0.8f, ALPHA}};
    color_properties const p{hwl, colors};
    return construct(dm, m, p);
  }

  static constexpr auto make_textured(drawmode const dm, model const &m, height_width_length const& hwl)
  {
    uv_properties const p{hwl};
    return construct(dm, m, p);
  }

  static constexpr auto make_wireframe(drawmode const dm, model const& m, height_width_length const& hwl)
  {
    wireframe_properties const p{hwl};
    return construct(dm, m, p);
  }
};

} // ns game

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
  model const& model;

private:
  friend class cube_factory;
  explicit constexpr cube(world_coordinate const &w, class model const& m, std::array<V, 8> &&v)
      : shape(w)
      , model(m)
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
    model const& model;
    std::array<c, 8> const colors;
  };

  struct uv_properties {
    height_width_length const dimensions;
    model const& model;
  };

  struct wireframe_properties {
    height_width_length const dimensions;
    model const& model;

    float const alpha = 1.0f;
    float const width = 0.25f;
  };

  static constexpr auto
  calculate_vertices(world_coordinate const &wc, height_width_length const &hw)
  {
    auto const h = hw.height;
    auto const w = hw.width;
    auto const l = hw.length;

    auto const x = wc.x();
    auto const y = wc.y();
    auto const z = wc.z();
    // clang-format off
    return std::array<vertex, 8> {
      vertex{x - w, y - h, z + l, wc.w()}, // front bottom-left
      vertex{x + w, y - h, z + l, wc.w()}, // front bottom-right
      vertex{x + w, y + h, z + l, wc.w()}, // front top-right
      vertex{x - w, y + h, z + l, wc.w()}, // front top-left

      vertex{x - w, y - h, z - l, wc.w()}, // back bottom-left
      vertex{x + w, y - h, z - l, wc.w()}, // back bottom-right
      vertex{x + w, y + h, z - l, wc.w()}, // back top-right
      vertex{x - w, y + h, z - l, wc.w()}  // back top-left
    };
    // clang-format on
  }

  static constexpr auto construct(world_coordinate const &wc, color_properties const &props)
  {
    auto const vertices = calculate_vertices(wc, props.dimensions);

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
    return cube<vertex_color_attributes>{wc, props.model, std::move(arr)};
    // clang-format on
  }

  static constexpr auto construct(world_coordinate const &wc, uv_properties const &props)
  {
    auto const vertices = calculate_vertices(wc, props.dimensions);

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
    return cube<vertex_attributes_only>{wc, props.model, std::move(arr)};
    // clang-format on
  }

  static constexpr auto construct(world_coordinate const &wc, wireframe_properties const &props)
  {
    auto const vertices = calculate_vertices(wc, props.dimensions);

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
    return cube<vertex_attributes_only>{wc, props.model, std::move(arr)};
    // clang-format on
  }

public:
  static constexpr auto make_spotted(world_coordinate const &wc, model const& model,
      std::array<float, 3> const &c, height_width_length const& hwl)
  {
    auto const ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};

    std::array<color_properties::c, 8> const colors{
        std::array<float, 4>{1.0f, 0.0f, 0.0f, ALPHA}, color,
        std::array<float, 4>{0.0f, 1.0f, 0.0f, ALPHA}, color,
        std::array<float, 4>{0.2f, 0.5f, 0.2f, ALPHA}, color,
        std::array<float, 4>{0.6f, 0.4f, 0.8f, ALPHA}};
    color_properties const p{hwl, model, colors};
    return construct(wc, p);
  }

  static constexpr auto make_textured(world_coordinate const &wc, model const& model,
      height_width_length const& hwl)
  {
    uv_properties const p{hwl, model};
    return construct(wc, p);
  }

  static constexpr auto make_wireframe(world_coordinate const &wc, model const& model,
      height_width_length const& hwl)
  {
    wireframe_properties const p{hwl, model};
    return construct(wc, p);
  }
};

} // ns game

#pragma once
#include <engine/gfx/colors.hpp>
#include <game/data_types.hpp>

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

struct vertex_attributes_only {
  vertex vertex;
  vertex_attributes_only() = default;

  explicit constexpr vertex_attributes_only(class vertex const &v)
      : vertex(v)
  {
  }
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

} // ns game

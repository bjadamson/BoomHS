#pragma once
#include <engine/gfx/types.hpp>
#include <engine/gfx/colors.hpp>
#include <engine/gfx/mode.hpp>

namespace engine::gfx
{
struct shape {
  draw_mode draw_mode_;
  model model_;

protected:
  explicit constexpr shape(draw_mode const dm, model const &m)
      : draw_mode_(dm)
      , model_(m)
  {
  }

public:
  auto constexpr const &draw_mode() const { return this->draw_mode_; }
  auto constexpr const &model() const { return this->model_; }
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

} // ns engine::gfx

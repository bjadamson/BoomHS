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

using vertex_color_attributes = std::tuple<vertex_t, color_t>;
using vertex_uv_attributes    = std::tuple<vertex_t, uv_t>;
using vertex_attributes_only  = std::tuple<vertex_t>;

template<typename T>
auto constexpr vertex(T const& t) { return std::get<vertex_t>(t); }

template<typename T>
auto constexpr color(T const& t) { return std::get<color_t>(t); }

template<typename T>
auto constexpr uv(T const& t) { return std::get<uv_t>(t); }

} // ns engine::gfx

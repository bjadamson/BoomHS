#pragma once
#include <opengl/types.hpp>
#include <opengl/colors.hpp>
#include <opengl/mode.hpp>

namespace opengl
{
struct shape {
  draw_mode draw_mode_;
  model const& model_;

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

using vertex_color_attributes = std::tuple<vertex_d, color_d>;
using vertex_uv_attributes    = std::tuple<vertex_d, uv_d>;
using vertex_attributes_only  = std::tuple<vertex_d>;

template<typename T>
auto constexpr vertex(T const& t) { return std::get<vertex_d>(t); }

template<typename T>
auto constexpr color(T const& t) { return std::get<color_d>(t); }

template<typename T>
auto constexpr uv(T const& t) { return std::get<uv_d>(t); }

} // ns opengl

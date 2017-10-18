#pragma once
#include <opengl/types.hpp>
#include <opengl/colors.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{
class shape {
  GLenum draw_mode_;
  model const& model_;
  bool in_gpu_memory_ = false;

protected:
  explicit constexpr shape(GLenum const dm, model const &m)
      : draw_mode_(dm)
      , model_(m)
  {
  }

public:

  auto constexpr draw_mode() const { return this->draw_mode_; }
  auto constexpr const &model() const { return this->model_; }

  bool is_in_gpu_memory() const { return this->in_gpu_memory_; }
  void set_is_in_gpu_memory(bool const v) { this->in_gpu_memory_ = v; }
};

using vertex_color_attributes     = std::tuple<vertex_d, color_d>;
using vertex_uv_attributes        = std::tuple<vertex_d, uv_d>;
using vertex_normal_uv_attributes = std::tuple<vertex_t, normal_t, uv_t>;
using vertex_attributes_only      = std::tuple<vertex_d>;

template<typename T>
auto constexpr vertex(T const& t) { return std::get<vertex_d>(t); }

template<typename T>
auto constexpr color(T const& t) { return std::get<color_d>(t); }

template<typename T>
auto constexpr normal(T const& t) { return std::get<normal_d>(t); }

template<typename T>
auto constexpr uv(T const& t) { return std::get<uv_d>(t); }

} // ns opengl

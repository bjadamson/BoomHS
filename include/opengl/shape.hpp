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

struct vertex_attributes_only {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX;
};

struct vertex_color_attributes {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX + color_t::NUM_FLOATS_PER_VERTEX;
};

struct vertex_uv_attributes {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX + uv_t::NUM_FLOATS_PER_VERTEX;
};

struct vertex_normal_uv_attributes {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX
    + normal_t::NUM_FLOATS_PER_VERTEX
    + uv_t::NUM_FLOATS_PER_VERTEX;
};

} // ns opengl

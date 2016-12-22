#pragma once
#include <gfx/shape2d.hpp>
#include <gfx/shape3d.hpp>

namespace gfx
{

template<typename D>
struct shape_properties
{
  draw_mode mode;
  D dimensions;
};

class shape_factory
{
  shape_factory() = delete;

public:
  template<typename M, typename ...Args>
  static auto make_triangle(draw_mode const dm, M const& model, Args &&... args)
  {
    return triangle_factory::make(dm, model, std::forward<Args>(args)...);
  }

  template<typename M, typename ...Args>
  static auto make_rectangle(draw_mode const dm, M const& model, Args &&... args)
  {
    return rectangle_factory::make(dm, model, std::forward<Args>(args)...);
  }

  template<typename M, typename ...Args>
  static auto make_polygon(draw_mode const dm, M const& model, Args &&... args)
  {
    return polygon_factory::make(dm, model, std::forward<Args>(args)...);
  }

  template<typename M, typename A>
  static auto make_spotted_cube(draw_mode const dm, M const& model, A const& a,
      height_width_length const& hwl)
  {
    return cube_factory::make_spotted(dm, model, a, hwl);
  }

  template<typename M>
  static auto make_textured_cube(draw_mode const dm, M const& model, height_width_length const& hwl)
  {
    return cube_factory::make_textured(dm, model, hwl);
  }

  template<typename M>
  static auto make_wireframe_cube(draw_mode const dm, M const& model, height_width_length const& hwl)
  {
    return cube_factory::make_wireframe(dm, model, hwl);
  }
};

} // ns gfx

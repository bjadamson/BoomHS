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
  template<typename ...Args>
  static auto make_triangle(triangle_properties const& properties, Args &&... args)
  {
    return triangle_factory::make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  static auto make_rectangle(rectangle_properties const& properties, Args &&... args)
  {
    return rectangle_factory::make(properties, std::forward<Args>(args)...);
  }

  template<typename M, typename ...Args>
  static auto make_polygon(draw_mode const dm, M const& model, Args &&... args)
  {
    return polygon_factory::make(dm, model, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  static auto make_spotted_cube(cube_properties const& properties, Args &&... args)
  {
    return cube_factory::make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  static auto make_textured_cube(cube_properties const& properties, Args &&... args)
  {
    return cube_factory::make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  static auto make_wireframe_cube(cube_properties const& properties, Args &&... args)
  {
    return cube_factory::make(properties, std::forward<Args>(args)...);
  }
};

} // ns gfx

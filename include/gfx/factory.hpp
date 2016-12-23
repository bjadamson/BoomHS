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
public:
  template<typename ...Args>
  auto make_triangle(triangle_properties const& properties, Args &&... args) const
  {
    return triangle_factory::make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_rectangle(rectangle_properties const& properties, Args &&... args) const
  {
    return rectangle_factory::make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_polygon(polygon_properties const& properties, Args &&... args) const
  {
    return polygon_factory::make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_spotted_cube(cube_properties const& properties, Args &&... args) const
  {
    return cube_factory::make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_textured_cube(cube_properties const& properties, Args &&... args) const
  {
    return cube_factory::make(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_wireframe_cube(cube_properties const& properties, Args &&... args) const
  {
    return cube_factory::make(properties, std::forward<Args>(args)...);
  }
};

} // ns gfx

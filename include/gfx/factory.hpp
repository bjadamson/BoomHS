#pragma once
#include <gfx/shape2d.hpp>
#include <gfx/shape3d.hpp>

namespace gfx
{

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
    return cube_factory::make_spotted(properties, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  auto make_color_cube(cube_properties const& properties, Args &&... args) const
  {
    return cube_factory::make_solid(properties, std::forward<Args>(args)...);
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

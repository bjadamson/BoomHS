#pragma once
#include <stlw/type_macros.hpp>
#include <engine/gfx/mode.hpp>
#include <engine/gfx/types.hpp>

// TODO: move out of game into engine
#include <game/shape2d.hpp>
#include <game/shape3d.hpp>

namespace engine::gfx
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
    return game::triangle_factory::make(dm, model, std::forward<Args>(args)...);
  }

  template<typename M, typename ...Args>
  static auto make_rectangle(draw_mode const dm, M const& model, Args &&... args)
  {
    return game::rectangle_factory::make(dm, model, std::forward<Args>(args)...);
  }

  template<typename M, typename ...Args>
  static auto make_polygon(draw_mode const dm, M const& model, Args &&... args)
  {
    return game::polygon_factory::make(dm, model, std::forward<Args>(args)...);
  }

  template<typename M, typename A>
  static auto make_spotted_cube(draw_mode const dm, M const& model, A const& a,
      height_width_length const& hwl)
  {
    return game::cube_factory::make_spotted(dm, model, a, hwl);
  }

  template<typename M>
  static auto make_textured_cube(draw_mode const dm, M const& model, height_width_length const& hwl)
  {
    return game::cube_factory::make_textured(dm, model, hwl);
  }

  template<typename M>
  static auto make_wireframe_cube(draw_mode const dm, M const& model, height_width_length const& hwl)
  {
    return game::cube_factory::make_wireframe(dm, model, hwl);
  }
};

} // ns engine::gfx

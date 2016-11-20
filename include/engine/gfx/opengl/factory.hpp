#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/polygon_renderer.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <engine/gfx/opengl/vertex_attrib.hpp>

namespace engine::gfx::opengl
{
namespace impl {

struct vertex_shader_filename {
  char const *filename;
  vertex_shader_filename(char const *f)
      : filename(f)
  {
  }
};
struct fragment_shader_filename {
  char const *filename;
  fragment_shader_filename(char const *f)
      : filename(f)
  {
  }
};

namespace gl = engine::gfx::opengl;

stlw::result<gl::program, std::string>
load_program(vertex_shader_filename const v, fragment_shader_filename const f)
{
  auto expected_program_id = gl::program_loader::load(v.filename, f.filename);
  if (!expected_program_id) {
    return stlw::make_error(expected_program_id.error());
  }
  return expected_program_id;
}

} // ns impl

struct factory
{
  NO_COPY_AND_NO_MOVE(factory);

  template<typename L>
  static stlw::result<polygon_renderer, std::string>
  make_polygon_renderer(L &logger)
  {
    DO_MONAD(auto phandle, impl::load_program("shader.vert", "shader.frag"));
    polygon_renderer polygon_renderer{std::move(phandle)};
    auto vertex_attribute_config = make_vertex_attribute_config(polygon_renderer);
    global::set_vertex_attributes(logger, vertex_attribute_config);
    return polygon_renderer;
  }
};

} // ns engine::gfx::opengl

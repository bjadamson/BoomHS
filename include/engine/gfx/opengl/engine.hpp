#pragma once
#include <engine/gfx/lib.hpp>
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/gfx_to_opengl.hpp>
#include <stlw/type_macros.hpp>

namespace engine::gfx::opengl
{

struct engine
{
  // data
  opengl_context rc0_, rc1_;

  NO_COPY(engine);
  NO_MOVE_ASSIGN(engine);
  MOVE_CONSTRUCTIBLE(engine);

  engine(opengl_context &&r0, opengl_context &&r1)
    : rc0_(std::move(r0))
    , rc1_(std::move(r1))
  {
    // background color
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  }

  void begin()
  {
    // Render
    glClear(GL_COLOR_BUFFER_BIT);
  }
  void end()
  {
  }

  template <typename Args, typename ...S>
  void draw0(Args const& args, S const&... shapes)
  {
    auto const gl_mapped_shapes = shape_mapper::map_to_floats(shapes...);
    renderer::draw_scene(args.logger, this->rc0_, args.view, args.projection, gl_mapped_shapes);
  }

  template <typename Args, typename ...S>
  void draw1(Args const& args, S const&... shapes)
  {
    auto const gl_mapped_shapes = shape_mapper::map_to_floats(shapes...);

    renderer::draw_scene(args.logger, this->rc1_, args.view, args.projection, gl_mapped_shapes);
  }
};

struct factory {
  factory() = delete;
  ~factory() = delete;

  template <typename L>
  static stlw::result<engine, std::string> make_opengl_engine(L &logger)
  {
    auto const make_ctx = [&logger](auto &&phandle, char const* asset_path) {
      auto context = context_factory::make_opengl_context(std::move(phandle), asset_path);
      auto vertex_attribute_config = make_vertex_attribute_config(context);
      global::set_vertex_attributes(logger, vertex_attribute_config);
      return context;
    };

    DO_MONAD(auto phandle0, program_loader::load("shader.vert", "shader.frag"));
    auto rc0 = make_ctx(std::move(phandle0), "assets/wall.jpg");

    DO_MONAD(auto phandle1, program_loader::load("shader.vert", "shader.frag"));
    auto rc1 = make_ctx(std::move(phandle1), "assets/container.jpg");

    // TODO: move this
    //glEnable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);
    return engine{std::move(rc0), std::move(rc1)};
  }
};

} // ns engine::gfx::opengl

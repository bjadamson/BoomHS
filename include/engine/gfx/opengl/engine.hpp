#pragma once
#include <engine/gfx/lib.hpp>
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/gfx_to_opengl.hpp>
#include <engine/gfx/resources.hpp>
#include <stlw/type_macros.hpp>

namespace engine::gfx::opengl
{

struct engine {
  // data
  opengl_context rc0_;
  opengl_texture_context rc1_;

  NO_COPY(engine);
  NO_MOVE_ASSIGN(engine);
  MOVE_CONSTRUCTIBLE(engine);

  engine(opengl_context &&r0, opengl_texture_context &&r1)
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
  void end() {}

  template <typename Args, typename... S>
  void draw0(Args const &args, S const &... shapes)
  {
    auto const gl_mapped_shapes = shape_mapper::map_to_floats(shapes...);
    renderer::draw_scene(args.logger, this->rc0_, args.view, args.projection, gl_mapped_shapes);
  }

  template <typename Args, typename... S>
  void draw1(Args const &args, S const &... shapes)
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
    auto const make_ctx = [&logger](auto &&phandle, char const *asset_path, auto const fn) {
      auto vertex_array = fn();
      auto context = context_factory::make_opengl_context(std::move(phandle), asset_path,
          std::move(vertex_array));
      return context;
    };

    auto constexpr RESOURCES = resources::make_resource_table();
    auto const get_r = [&](auto const i) { return RESOURCES[i]; };

    DO_TRY(auto phandle0, program_loader::from_files("color.vert", "color.frag"));
    auto va0 = global::make_vertex_color_vertex_attribute();
    auto c0 = context_factory::make_opengl_context(std::move(phandle0), std::move(va0));

    DO_TRY(auto phandle1, program_loader::from_files("image.vert", "image.frag"));
    auto va1 = global::make_vertex_uv_vertex_attribute();
    auto c1 = context_factory::make_texture_opengl_context(std::move(phandle1), get_r(IMAGES::WALL),
        std::move(va1));

    // TODO: move this
    // glEnable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);
    return engine{std::move(c0), std::move(c1)};
  }
};

} // ns engine::gfx::opengl

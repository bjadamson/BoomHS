#pragma once
#include <engine/gfx/lib.hpp>
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/gfx_to_opengl.hpp>
#include <engine/gfx/opengl/render2d.hpp>
#include <engine/gfx/opengl/render3d.hpp>
#include <engine/gfx/resources.hpp>
#include <stlw/type_macros.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace engine::gfx::opengl
{

struct engine {
  // data
  color2d_context color2d_;
  texture2d_context texture2d_wall_;
  texture2d_context texture2d_container_;

  color3d_context color3d_;
  texture3d_context texture3d_;

  opengl_wireframe_context wireframe_;

  NO_COPY(engine);
  NO_MOVE_ASSIGN(engine);
  MOVE_CONSTRUCTIBLE(engine);

  engine(color2d_context &&c2d, texture2d_context &&t2d0, texture2d_context &&t2d1,
      color3d_context &&c3d, texture3d_context &&t3d, opengl_wireframe_context &&w)
      : color2d_(std::move(c2d))
      , texture2d_wall_(std::move(t2d0))
      , texture2d_container_(std::move(t2d1))
      , color3d_(std::move(c3d))
      , texture3d_(std::move(t3d))
      , wireframe_(std::move(w))
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

  template <typename FN, typename... S>
  void draw_shape(FN const& fn, S const &... shapes)
  {
    auto const gl_mapped_shapes = shape_mapper::map_to_floats(shapes...);
    fn(gl_mapped_shapes);
  }

  template <typename Args, typename... S>
  void draw_2dshapes_with_colors(Args const &args, S const &... shapes)
  {
    auto const fn = [&](auto const& gl_mapped_shapes) {
      render2d::draw_scene(args.logger, this->color2d_, args.projection, gl_mapped_shapes);
    };
    this->draw_shape(fn, shapes...);
  }

  template <typename Args, typename... S>
  void draw_2dshapes_with_wall_texture(Args const &args, S const &... shapes)
  {
    auto const fn = [&](auto const& gl_mapped_shapes) {
      render2d::draw_scene(args.logger, this->texture2d_wall_, args.projection, gl_mapped_shapes);
    };
    this->draw_shape(fn, shapes...);
  }

  template <typename Args, typename... S>
  void draw_2dshapes_with_container_texture(Args const &args, S const &... shapes)
  {
    auto const fn = [&](auto const& gl_mapped_shapes) {
      render2d::draw_scene(args.logger, this->texture2d_container_, args.projection, gl_mapped_shapes);
    };
    this->draw_shape(fn, shapes...);
  }

  template <typename Args, typename... S>
  void draw_3dcolor_shapes(Args const &args, S const &... shapes)
  {
    auto const fn = [&](auto const& gl_mapped_shapes) {
      render3d::draw_scene(args.logger, this->color3d_, args.view, args.projection, gl_mapped_shapes);
    };
    this->draw_shape(fn, shapes...);
  }

  template <typename Args, typename... S>
  void draw_3dtextured_shapes(Args const &args, S const &... shapes)
  {
    auto const fn = [&](auto const& gl_mapped_shapes) {
      render3d::draw_scene(args.logger, this->texture3d_, args.view, args.projection, gl_mapped_shapes);
    };
    this->draw_shape(fn, shapes...);
  }

  template <typename Args, typename... S>
  void draw_shapes_with_wireframes(Args const &args, S const &... shapes)
  {
    //this->draw_shape(this->wf_, args, shapes...);
  }
};

struct factory {
  factory() = delete;
  ~factory() = delete;

  template <typename L>
  static stlw::result<engine, std::string> make_opengl_engine(L &logger)
  {
    auto constexpr RESOURCES = resources::make_resource_table();
    auto const get_r = [&](auto const i) { return RESOURCES[i]; };

    DO_TRY(auto phandle0, program_loader::from_files("2dcolor.vert", "2dcolor.frag"));
    auto va0 = global::make_vertex_color_vertex_attribute(logger);
    auto c0 = context_factory::make_color2d(logger, std::move(phandle0), std::move(va0));

    DO_TRY(auto phandle1, program_loader::from_files("2dtexture.vert", "2dtexture.frag"));
    auto va1 = global::make_vertex_uv2d_vertex_attribute(logger);
    auto c1 = context_factory::make_texture2d(logger, std::move(phandle1),
                                                           get_r(IMAGES::WALL), std::move(va1));

    DO_TRY(auto phandle2, program_loader::from_files("2dtexture.vert", "2dtexture.frag"));
    auto va2 = global::make_vertex_uv2d_vertex_attribute(logger);
    auto c2 = context_factory::make_texture2d(logger, std::move(phandle2),
                                                           get_r(IMAGES::CONTAINER), std::move(va2));

    DO_TRY(auto phandle3, program_loader::from_files("3dcolor.vert", "3dcolor.frag"));
    auto va3 = global::make_vertex_color_vertex_attribute(logger);
    auto c3 = context_factory::make_color3d(logger, std::move(phandle3), std::move(va3));

    DO_TRY(auto phandle4, program_loader::from_files("3dtexture.vert", "3dtexture.frag"));
    auto va4 = global::make_3dvertex_only_vertex_attribute(logger);
    auto c4 = context_factory::make_texture3d(
        logger, std::move(phandle4), std::move(va4),
        get_r(IMAGES::CUBE_FRONT),
        get_r(IMAGES::CUBE_RIGHT),
        get_r(IMAGES::CUBE_BACK),
        get_r(IMAGES::CUBE_LEFT),
        get_r(IMAGES::CUBE_TOP),
        get_r(IMAGES::CUBE_BOTTOM)
        );

    DO_TRY(auto phandle5, program_loader::from_files("wire.vert", "wire.frag"));
    // TODO: now 2d wireframes will be broken here, need a "c5" and separate shaders until we
    // figure out how to unify them.
    auto va5 = global::make_3dvertex_only_vertex_attribute(logger);
    auto const color = LIST_OF_COLORS::PINK;
    auto c5 = context_factory::make_wireframe(logger, std::move(phandle5), std::move(va5), color);

    return engine{std::move(c0), std::move(c1), std::move(c2), std::move(c3), std::move(c4),
      std::move(c5)};
  }
};

} // ns engine::gfx::opengl

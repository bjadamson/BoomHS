#pragma once
#include <gfx/opengl/context.hpp>
#include <gfx/opengl/pipeline.hpp>
#include <gfx/opengl/gfx_to_opengl.hpp>
#include <gfx/opengl/render2d.hpp>
#include <gfx/opengl/render3d.hpp>
#include <gfx/resources.hpp>
#include <stlw/type_macros.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gfx::opengl
{

namespace impl
{

template <typename FN, typename B>
void draw_shape(FN const& fn, B const& burrito)
{
  auto const gl_mapped_shapes = shape_mapper::map_to_opengl(burrito);
  fn(gl_mapped_shapes);
}

template<typename Args, typename C, typename B>
void draw2d(Args const& args, C &ctx, B const& burrito)
{
  auto const fn = [&](auto const& gl_mapped_shapes) {
    render2d::draw_scene(args.logger, ctx, gl_mapped_shapes);
  };
  draw_shape(fn, burrito);
}

template <typename Args, typename C, typename B>
void draw3d(Args const &args, C &ctx, B const& burrito)
{
  auto const fn = [&](auto const& gl_mapped_shapes) {
    render3d::draw_scene(args.logger, ctx, args.camera, args.projection, gl_mapped_shapes);
  };
  draw_shape(fn, burrito);
}

} // ns impl

struct context2d_args
{
  color2d_context color;
  texture2d_context texture_wall;
  texture2d_context texture_container;
  wireframe2d_context wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(context2d_args);
};
struct context3d_args
{
  color3d_context color;
  texture3d_context texture;
  skybox_context skybox;
  wireframe3d_context wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(context3d_args);
};

struct engine {
  // data
  context2d_args d2;
  context3d_args d3;

  MOVE_CONSTRUCTIBLE_ONLY(engine);

  void enable_depth_tests() {
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
  }

  void disable_depth_tests() {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
  }

  engine(glm::vec4 const& bg, context2d_args &&context_2d, context3d_args &&context_3d)
    : d2(std::move(context_2d))
    , d3(std::move(context_3d))
  {
    // background color
    glClearColor(bg.x, bg.y, bg.z, bg.w);

    // Initially assume we are drawing 3d
    enable_depth_tests();
  }

  void begin()
  {
    // Render
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
  void end() {}

  template <typename Args, typename C, typename B>
  void draw(Args const& args, C &ctx, B const& burrito)
  {
    if constexpr (C::IS_2D) {
      disable_depth_tests();
      impl::draw2d(args, ctx, burrito);
      enable_depth_tests();
    } else {
      auto const draw3d = [&]() {
        impl::draw3d(args, ctx, burrito);
      };
      if constexpr (C::IS_SKYBOX) {
        disable_depth_tests();
        draw3d();
        enable_depth_tests();
      } else {
        draw3d();
      }
    }
  }
};

struct engine_factory {
  engine_factory() = delete;
  ~engine_factory() = delete;

  template <typename L>
  static stlw::result<engine, std::string> make(L &logger)
  {
    auto constexpr RESOURCES = resources::make_resource_table();
    auto const get_r = [&](auto const i) { return RESOURCES[i]; };

    pipeline_factory pf;
    DO_TRY(auto phandle0, pf.make("2dcolor.vert", "2dcolor.frag"));
    auto va0 = global::make_vertex_color_vertex_attribute(logger);
    auto c0 = context_factory::make_color2d(logger, std::move(phandle0), std::move(va0));

    DO_TRY(auto phandle1, pf.make("2dtexture.vert", "2dtexture.frag"));
    auto va1 = global::make_vertex_uv2d_vertex_attribute(logger);
    auto c1 = context_factory::make_texture2d(logger, std::move(phandle1),
                                                           get_r(IMAGES::WALL), std::move(va1));

    DO_TRY(auto phandle2, pf.make("2dtexture.vert", "2dtexture.frag"));
    auto va2 = global::make_vertex_uv2d_vertex_attribute(logger);
    auto c2 = context_factory::make_texture2d(logger, std::move(phandle2),
                                                           get_r(IMAGES::CONTAINER), std::move(va2));

    DO_TRY(auto phandle3, pf.make("wire.vert", "wire.frag"));
    auto va3 = global::make_2dvertex_only_vertex_attribute(logger);
    auto const color = LIST_OF_COLORS::PINK;
    auto c3 = context_factory::make_wireframe2d(logger, std::move(phandle3), std::move(va3), color);

    DO_TRY(auto phandle4, pf.make("3dcolor.vert", "3dcolor.frag"));
    auto va4 = global::make_vertex_color_vertex_attribute(logger);
    auto c4 = context_factory::make_color3d(logger, std::move(phandle4), std::move(va4));

    DO_TRY(auto phandle5, pf.make("3dtexture.vert", "3dtexture.frag"));
    auto va5 = global::make_3dvertex_only_vertex_attribute(logger);
    auto c5 = context_factory::make_texture3d(
        logger, std::move(phandle5), std::move(va5),
        get_r(IMAGES::CUBE_FRONT),
        get_r(IMAGES::CUBE_RIGHT),
        get_r(IMAGES::CUBE_BACK),
        get_r(IMAGES::CUBE_LEFT),
        get_r(IMAGES::CUBE_TOP),
        get_r(IMAGES::CUBE_BOTTOM)
        );

    DO_TRY(auto phandle6, pf.make("3dtexture.vert", "3dtexture.frag"));
    auto va6 = global::make_3dvertex_only_vertex_attribute(logger);
    auto c6 = context_factory::make_skybox(
        logger, std::move(phandle6), std::move(va6),
        get_r(IMAGES::SB_FRONT),
        get_r(IMAGES::SB_RIGHT),
        get_r(IMAGES::SB_BACK),
        get_r(IMAGES::SB_LEFT),
        get_r(IMAGES::SB_TOP),
        get_r(IMAGES::SB_BOTTOM)
        );

    DO_TRY(auto phandle7, pf.make("3dwire.vert", "wire.frag"));
    auto va7 = global::make_3dvertex_only_vertex_attribute(logger);
    auto const color2 = LIST_OF_COLORS::PURPLE;
    auto c7 = context_factory::make_wireframe3d(logger, std::move(phandle7), std::move(va7), color2);

    context2d_args d2{std::move(c0), std::move(c1), std::move(c2), std::move(c3)};
    context3d_args d3{std::move(c4), std::move(c5), std::move(c6), std::move(c7)};

    auto const c = LIST_OF_COLORS::WHITE;
    auto const background_color = glm::vec4{c[0], c[1], c[2], 1.0f};
    return engine{background_color, std::move(d2), std::move(d3)};
  }
};

} // ns gfx::opengl

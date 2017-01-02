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

template<typename Args, typename P, typename B>
void draw2d(Args const& args, P &pipeline, B const& burrito)
{
  auto const fn = [&](auto const& gl_mapped_shapes) {
    render2d::draw_scene(args.logger, pipeline, gl_mapped_shapes);
  };
  draw_shape(fn, burrito);
}

template <typename Args, typename P, typename B>
void draw3d(Args const &args, P &pipeline, B const& burrito)
{
  auto const fn = [&](auto const& gl_mapped_shapes) {
    render3d::draw_scene(args.logger, pipeline, args.camera, args.projection, gl_mapped_shapes);
  };
  draw_shape(fn, burrito);
}

} // ns impl

struct program2d
{
  pipeline<color2d_context> color;
  pipeline<texture2d_context> texture_wall;
  pipeline<texture2d_context> texture_container;
  pipeline<wireframe2d_context> wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(program2d);
};

struct program3d
{
  pipeline<color3d_context> color;
  pipeline<texture3d_context> texture;
  pipeline<skybox_context> skybox;
  pipeline<wireframe3d_context> wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(program3d);
};

struct opengl_engine {
  program2d d2;
  program3d d3;

  MOVE_CONSTRUCTIBLE_ONLY(opengl_engine);

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

  opengl_engine(glm::vec4 const& bg, program2d &&p2d, program3d &&p3d)
    : d2(MOVE(p2d))
    , d3(MOVE(p3d))
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

  template <typename Args, typename P, typename B>
  void draw(Args const& args, P &pipeline, B const& burrito)
  {
    using C = typename P::CTX;
    if constexpr (C::IS_2D) {
      disable_depth_tests();
      impl::draw2d(args, pipeline, burrito);
      enable_depth_tests();
    } else {
      auto const draw3d = [&]() {
        impl::draw3d(args, pipeline, burrito);
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

class opengl_engine_factory {
  opengl_engine_factory() = delete;
  ~opengl_engine_factory() = delete;

public:
  template <typename L>
  static stlw::result<opengl_engine, std::string> make(L &logger)
  {
    auto constexpr RESOURCES = resources::make_resource_table();
    auto const get_r = [&](auto const i) { return RESOURCES[i]; };

    pipeline_factory pf;
    va_factory vf;
    auto c0 = context_factory::make_color2d(logger);
    auto va0 = vf.make_vertex_color(logger);
    DO_TRY(auto p0, pf.make("2dcolor.vert", "2dcolor.frag", MOVE(c0), MOVE(va0)));

    auto c1 = context_factory::make_texture2d(logger, get_r(IMAGES::WALL));
    auto va1 = vf.make_vertex_uv2d(logger);
    DO_TRY(auto p1, pf.make("2dtexture.vert", "2dtexture.frag", MOVE(c1), MOVE(va1)));

    auto c2 = context_factory::make_texture2d(logger, get_r(IMAGES::CONTAINER));
    auto va2 = vf.make_vertex_uv2d(logger);
    DO_TRY(auto p2, pf.make("2dtexture.vert", "2dtexture.frag", MOVE(c2), MOVE(va2)));

    auto const color = LIST_OF_COLORS::PINK;
    auto c3 = context_factory::make_wireframe2d(logger, color);
    auto va3 = vf.make_vertex_only(logger);
    DO_TRY(auto p3, pf.make("wire.vert", "wire.frag", MOVE(c3), MOVE(va3)));

    auto c4 = context_factory::make_color3d(logger);
    auto va4 = vf.make_vertex_color(logger);
    DO_TRY(auto p4, pf.make("3dcolor.vert", "3dcolor.frag", MOVE(c4), MOVE(va4)));

    auto c5 = context_factory::make_texture3d(logger,
        get_r(IMAGES::CUBE_FRONT),
        get_r(IMAGES::CUBE_RIGHT),
        get_r(IMAGES::CUBE_BACK),
        get_r(IMAGES::CUBE_LEFT),
        get_r(IMAGES::CUBE_TOP),
        get_r(IMAGES::CUBE_BOTTOM)
        );
    auto va5 = vf.make_vertex_only(logger);
    DO_TRY(auto p5, pf.make("3dtexture.vert", "3dtexture.frag", MOVE(c5), MOVE(va5)));

    auto c6 = context_factory::make_skybox(logger,
        get_r(IMAGES::SB_FRONT),
        get_r(IMAGES::SB_RIGHT),
        get_r(IMAGES::SB_BACK),
        get_r(IMAGES::SB_LEFT),
        get_r(IMAGES::SB_TOP),
        get_r(IMAGES::SB_BOTTOM)
        );
    auto va6 = vf.make_vertex_only(logger);
    DO_TRY(auto p6, pf.make("3dtexture.vert", "3dtexture.frag", MOVE(c6), MOVE(va6)));

    auto const color2 = LIST_OF_COLORS::PURPLE;
    auto c7 = context_factory::make_wireframe3d(logger, color2);
    auto va7 = vf.make_vertex_only(logger);
    DO_TRY(auto p7, pf.make("3dwire.vert", "wire.frag", MOVE(c7), MOVE(va7)));

    program2d d2{MOVE(p0), MOVE(p1), MOVE(p2), MOVE(p3)};
    program3d d3{MOVE(p4), MOVE(p5), MOVE(p6), MOVE(p7)};

    auto const c = LIST_OF_COLORS::WHITE;
    auto const background_color = glm::vec4{c[0], c[1], c[2], 1.0f};
    return opengl_engine{background_color, MOVE(d2), MOVE(d3)};
  }
};

} // ns gfx::opengl

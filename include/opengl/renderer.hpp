#pragma once
#include <opengl/context.hpp>
#include <opengl/render_shape2d.hpp>
#include <opengl/render_shape3d.hpp>
#include <stlw/type_macros.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace opengl
{

struct opengl_renderer
{
  explicit opengl_renderer(glm::vec4 const& bg)
  {
    // background color
    glClearColor(bg.x, bg.y, bg.z, bg.w);

    // Initially assume we are drawing 3d
    enable_depth_tests();
  }

  void enable_depth_tests()
  {
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
  }

  void disable_depth_tests()
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
  }

  void begin()
  {
    // Render
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
  void end() {}

  template <typename Args, typename P, typename SHAPE>
  void draw(Args const& args, P &pipeline, SHAPE const& shape)
  {
    // TODO: move this draw fn() up one left (out of opengl).
    using C = typename P::CTX;
    if constexpr (C::IS_2D) {
      disable_depth_tests();
      render::draw2d(args, pipeline, shape);
      enable_depth_tests();
    } else {
      auto const draw3d = [&]() {
        render::draw3d(args, pipeline, shape);
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

struct opengl_renderer_factory
{
  opengl_renderer_factory() = delete;

  template<typename L>
  static auto
  make(L &logger)
  {
    auto const c = LIST_OF_COLORS::WHITE;
    auto const background_color = glm::vec4{c[0], c[1], c[2], 1.0f};
    return opengl_renderer{background_color};
  }
};

} // ns opengl

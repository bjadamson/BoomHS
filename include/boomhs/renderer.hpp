#pragma once
#include <opengl/global.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/pipelines.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/tilemap.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs {

struct RenderArgs {
  stlw::Logger &logger;
  Camera const& camera;
};

} // ns boomhs

namespace boomhs::render {

inline void
enable_depth_tests()
{
  //glCullFace(GL_BACK);
  //glFrontFace(GL_CW);
  //glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
}

inline void
disable_depth_tests()
{
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
}

inline void
init(window::Dimensions const& dimensions)
{
  // Initialize opengl
  glViewport(0, 0, dimensions.w, dimensions.h);

  glDisable(GL_CULL_FACE);
  enable_depth_tests();
}

inline void
clear_screen(opengl::Color const& color)
{
  glClearColor(color.r, color.g, color.b, color.a);

  // Render
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

namespace detail {

template <typename PIPE, typename FN>
void
bind_stuff_and_draw(stlw::Logger &logger, PIPE &pipeline, opengl::DrawInfo const &dinfo, FN const& fn)
{
  using namespace opengl;

  if constexpr (PIPE::HAS_COLOR_UNIFORM) {
    pipeline.set_uniform_array_4fv(logger, "u_color", pipeline.color().to_array());
  }

  if constexpr (PIPE::HAS_TEXTURE) {
    opengl::global::texture_bind(pipeline.texture());
    ON_SCOPE_EXIT([&pipeline]() { opengl::global::texture_unbind(pipeline.texture()); });
    fn(dinfo);
  } else {
    fn(dinfo);
  }
}

template <typename PIPE>
void render_element_buffer(stlw::Logger &logger, PIPE &pipeline, opengl::DrawInfo const& dinfo)
{
  auto const draw_mode = dinfo.draw_mode();
  auto const num_indices = dinfo.num_indices();
  auto constexpr OFFSET = nullptr;

  if constexpr (PIPE::IS_INSTANCED) {
    auto const instance_count = pipeline.instance_count();

    glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, instance_count);
  } else {
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
  }
}

template <typename PIPE>
void draw_3dshape(RenderArgs const &args, boomhs::Transform const& transform, PIPE &pipeline, opengl::DrawInfo const& dinfo)
{
  auto &logger = args.logger;
  glm::mat4 const camera_matrix = args.camera.camera_matrix();

  auto const draw_3d_shape_fn = [&](auto const &dinfo) {
    auto const model_matrix = transform.model_matrix();

    // Model View Projection matrix
    auto const mvp_matrix = camera_matrix * model_matrix;
    pipeline.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);

    if constexpr (PIPE::IS_SKYBOX) {
      disable_depth_tests();
      render_element_buffer(logger, pipeline, dinfo);
      enable_depth_tests();
    } else {
      render_element_buffer(logger, pipeline, dinfo);
    }
  };

  // Use the pipeline's PROGRAM and bind the pipeline's VAO.
  pipeline.use_program(logger);
  opengl::global::vao_bind(pipeline.vao());

  bind_stuff_and_draw(logger, pipeline, dinfo, draw_3d_shape_fn);
}

template <typename PIPE>
void
draw_2dshape(RenderArgs const &args, boomhs::Transform const& transform, PIPE &pipeline, opengl::DrawInfo const &dinfo)
{
  using namespace opengl;

  auto &logger = args.logger;
  auto const draw_2d_shape_fn = [&](auto const& dinfo) {
    auto const model_matrix = transform.model_matrix();
    pipeline.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
    render_element_buffer(logger, pipeline, dinfo);
  };

  // Use the pipeline's PROGRAM and bind the pipeline's VAO.
  pipeline.use_program(logger);
  opengl::global::vao_bind(pipeline.vao());
  bind_stuff_and_draw(logger, pipeline, dinfo, draw_2d_shape_fn);
}

} // ns detail

template <typename PIPE>
void
draw(RenderArgs const& args, Transform const& transform, PIPE &pipeline, opengl::DrawInfo const& dinfo)
{
  auto &logger = args.logger;

  if constexpr (PIPE::IS_2D) {
    disable_depth_tests();
    detail::draw_2dshape(args, transform, pipeline, dinfo);
    enable_depth_tests();
  } else {
    detail::draw_3dshape(args, transform, pipeline, dinfo);
  }
}

struct DrawTilemapArgs
{
  opengl::DrawInfo const& hashtag_dinfo;
  opengl::PipelineHashtag3D &hashtag_pipeline;

  opengl::DrawInfo const& plus_dinfo;
  opengl::PipelinePlus3D &plus_pipeline;
};

template <typename TILEMAP>
void draw_tilemap(RenderArgs const& args, Transform const& transform, DrawTilemapArgs &&dt_args, TILEMAP const& tilemap)
{
  auto &logger = args.logger;
  glm::mat4 const camera_matrix = args.camera.camera_matrix();;
  auto const model_matrix = transform.model_matrix();
  auto const mvp_matrix = camera_matrix * model_matrix;

  auto const draw_tile = [&logger, &mvp_matrix](auto &pipeline, auto const& dinfo, auto const& arr) {
    pipeline.use_program(logger);
    opengl::global::vao_bind(pipeline.vao());
    pipeline.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);
    pipeline.set_uniform_array_3fv(logger, "u_offset", arr);

    detail::render_element_buffer(logger, pipeline, dinfo);
  };

  auto const [w, h, l] = tilemap.dimensions();
  FOR(a, w) {
    FOR(b, h) {
      FOR(c, l) {
        // don't draw non-wall tiles for now
        auto const cast = [](auto const v) { return static_cast<float>(v); };
        auto const arr = stlw::make_array<float>(cast(a), cast(b), cast(c));
        if(tilemap.data(a, b, c).is_wall) {
          draw_tile(dt_args.hashtag_pipeline, dt_args.hashtag_dinfo, arr);
        } else {
          draw_tile(dt_args.plus_pipeline, dt_args.plus_dinfo, arr);
        }
      }
    }
  }
}

} // ns boomhs::render

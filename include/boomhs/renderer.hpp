#pragma once
#include <opengl/global.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/pipelines.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <boomhs/state.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

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
void
draw_3dshape(RenderArgs const &args, glm::mat4 const& model_matrix, PIPE &pipeline, opengl::DrawInfo const& dinfo)
{
  auto &logger = args.logger;
  glm::mat4 const view_matrix = args.camera.camera_matrix();

  auto const draw_3d_shape_fn = [&](auto const &dinfo) {

    // various matrices
    pipeline.set_uniform_matrix_4fv(logger, "u_mvpmatrix", view_matrix * model_matrix);

    if constexpr (PIPE::RECEIVES_LIGHT) {
      pipeline.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
      pipeline.set_uniform_color_3fv(logger, "u_lightcolor", args.world.light_color);
      pipeline.set_uniform_vec3(logger, "u_lightpos", args.entities[GameState::LIGHT_INDEX]->translation);
      pipeline.set_uniform_vec3(logger, "u_viewpos", args.camera.world_position());

      pipeline.set_uniform_vec3(logger, "u_material.ambient",  glm::vec3{0.5f, 1.0f, 0.31f});
      pipeline.set_uniform_vec3(logger, "u_material.diffuse",  glm::vec3{0.5f, 1.0f, 0.31f});
      pipeline.set_uniform_vec3(logger, "u_material.specular", glm::vec3{1.0f, 1.0f, 0.5f});
      pipeline.set_uniform_float1(logger, "u_material.shininess", 32.0f);
    }

    if constexpr (PIPE::IS_SKYBOX) {
      disable_depth_tests();
      render_element_buffer(logger, pipeline, dinfo);
      enable_depth_tests();
    } else {
      render_element_buffer(logger, pipeline, dinfo);
    }
  };

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

  bind_stuff_and_draw(logger, pipeline, dinfo, draw_2d_shape_fn);
}

} // ns detail

template <typename PIPE>
void
draw(RenderArgs const& args, Transform const& transform, PIPE &pipeline, opengl::DrawInfo const& dinfo)
{
  auto &logger = args.logger;

  // Use the pipeline's PROGRAM and bind the pipeline's VAO.
  pipeline.use_program(logger);
  opengl::global::vao_bind(pipeline.vao());

  if constexpr (PIPE::IS_2D) {
    disable_depth_tests();
    detail::draw_2dshape(args, transform, pipeline, dinfo);
    enable_depth_tests();
  } else {
    auto const model_matrix = transform.model_matrix();
    detail::draw_3dshape(args, model_matrix, pipeline, dinfo);
  }
}

struct DrawTilemapArgs
{
  opengl::DrawInfo const& hashtag_dinfo;
  opengl::PipelineHashtag3D &hashtag_pipeline;

  opengl::DrawInfo const& plus_dinfo;
  opengl::PipelinePlus3D &plus_pipeline;
};

template<typename TILEMAP>
void
draw_tilemap(RenderArgs const& args, Transform const& transform, DrawTilemapArgs &&dt_args,
    TILEMAP const& tilemap, bool const reveal_map)
{
  auto const& draw_tile_helper = [&](auto &pipeline, auto const& dinfo, glm::vec3 const& tile_pos) {
    auto &logger = args.logger;
    pipeline.use_program(logger);
    opengl::global::vao_bind(pipeline.vao());

    glm::mat4 const model_matrix = transform.model_matrix();
    glm::mat4 const translated = glm::translate(model_matrix, tile_pos);
    detail::draw_3dshape(args, translated, pipeline, dinfo);
  };

  auto const draw_all_tiles = [&](auto const& pos) {
    auto const& tile = tilemap.data(pos.x, pos.y, pos.z);
    if (!reveal_map && !tile.is_visible) {
      return;
    }
    if(tile.is_wall) {
      draw_tile_helper(dt_args.hashtag_pipeline, dt_args.hashtag_dinfo, pos);
    } else {
      draw_tile_helper(dt_args.plus_pipeline, dt_args.plus_dinfo, pos);
    }
  };
  tilemap.visit_each(draw_all_tiles);
}

template<typename PIPE>
struct Drawable
{
  PIPE &pipeline;
  opengl::DrawInfo const& dinfo;
};

inline void
draw_tilegrid(RenderArgs const& args, Transform const& transform,
    Drawable<opengl::PipelinePositionColor3D> &&drawable)
{
  glm::mat4 const view_matrix = args.camera.camera_matrix();
  auto const model_matrix = transform.model_matrix();
  auto const mvp_matrix = view_matrix * model_matrix;

  auto &logger = args.logger;
  auto &pipeline = drawable.pipeline;
  pipeline.use_program(logger);
  opengl::global::vao_bind(pipeline.vao());

  detail::render_element_buffer(logger, pipeline, drawable.dinfo);
}

} // ns boomhs::render

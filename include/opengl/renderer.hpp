#pragma once
#include <opengl/global.hpp>
#include <opengl/camera.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/vertex_attribute.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace opengl {

struct RenderArgs {
  stlw::Logger &logger;
  opengl::camera const& camera;
  glm::mat4 const& projection;
};

void enable_depth_tests();
void disable_depth_tests();

namespace detail {

template <typename PIPE, typename FN>
void
draw_scene(stlw::Logger &logger, PIPE &pipeline, DrawInfo const &dinfo, FN const& fn)
{
  using namespace opengl;

  // Use the pipeline's PROGRAM.
  auto &program = pipeline.program_ref();
  program.use(logger);

  if constexpr (PIPE::HAS_COLOR_UNIFORM) {
    pipeline.set_uniform_array_4fv(logger, "u_color", pipeline.color());
  }

  // Enable the pipeline's VAO.
  global::vao_bind(pipeline.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  LOG_ANY_GL_ERRORS(logger, "before drawing scene");
  if constexpr (PIPE::HAS_TEXTURE) {
    global::texture_bind(pipeline.texture());
    ON_SCOPE_EXIT([&pipeline]() { global::texture_unbind(pipeline.texture()); });
    fn(dinfo);
  } else {
    fn(dinfo);
  }
  LOG_ANY_GL_ERRORS(logger, "after drawing scene");
}

template <typename PIPE>
void render_shape(stlw::Logger &logger, PIPE &pipeline, DrawInfo const& dinfo)
{
  auto const draw_mode = dinfo.draw_mode();
  auto const num_indices = dinfo.num_indices();
  auto constexpr OFFSET = nullptr;

  if constexpr (PIPE::IS_INSTANCED) {
    auto const instance_count = pipeline.instance_count();

    glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, instance_count);
    LOG_ANY_GL_ERRORS(logger, "glDrawElementsInstanced 1");
  } else {
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
  }
}

template <typename PIPE>
void draw_3dshape(RenderArgs const &args, opengl::Model const& model, PIPE &pipeline, DrawInfo const& dinfo)
{
  auto const view = compute_view(args.camera);
  auto const& projection = args.projection;

  auto &logger = args.logger;
  auto &program = pipeline.program_ref();

  auto const draw_3d_shape_fn = [&](auto const &dinfo) {
    auto const tmatrix = glm::translate(glm::mat4{}, model.translation);
    auto const rmatrix = glm::toMat4(model.rotation);
    auto const smatrix = glm::scale(glm::mat4{}, model.scale);
    auto const mmatrix = tmatrix * rmatrix * smatrix;
    auto const mvmatrix = projection * view * mmatrix;
    pipeline.set_uniform_matrix_4fv(logger, "u_mvmatrix", mvmatrix);

    if constexpr (PIPE::IS_SKYBOX) {
      opengl::disable_depth_tests();
      render_shape(logger, pipeline, dinfo);
      opengl::enable_depth_tests();
    } else {
      render_shape(logger, pipeline, dinfo);
    }
  };

  draw_scene(logger, pipeline, dinfo, draw_3d_shape_fn);
}

template <typename PIPE>
void
draw_2dshape(RenderArgs const &args, opengl::Model const& model, PIPE &pipeline, DrawInfo const &dinfo)
{
  using namespace opengl;

  auto &program = pipeline.program_ref();
  auto &logger = args.logger;
  auto const draw_2d_shape_fn = [&](auto const& dinfo) {
    auto const& t = model.translation;
    auto const tmatrix = glm::translate(glm::mat4{}, glm::vec3{t.x, t.y, 0.0});
    auto const rmatrix = glm::toMat4(model.rotation);
    auto const smatrix = glm::scale(glm::mat4{}, model.scale);
    auto const mvmatrix = tmatrix * rmatrix * smatrix;
    pipeline.set_uniform_matrix_4fv(logger, "u_mvmatrix", mvmatrix);
    render_shape(logger, pipeline, dinfo);
  };

  draw_scene(logger, pipeline, dinfo, draw_2d_shape_fn);
}

} // ns detail

template <typename PIPE>
void draw(RenderArgs const& args, Model const& model, PIPE &pipeline, DrawInfo const& dinfo)
{
  auto &logger = args.logger;

  if constexpr (PIPE::IS_2D) {
    opengl::disable_depth_tests();
    detail::draw_2dshape(args, model, pipeline, dinfo);
    opengl::enable_depth_tests();
  } else {
    detail::draw_3dshape(args, model, pipeline, dinfo);
  }
}

template <typename TILEMAP>
void draw_tilemap(RenderArgs const& args, Model const& model, PipelineHashtag3D &pipeline,
    DrawInfo const& dinfo, TILEMAP const& tilemap)
{
  auto &logger = args.logger;
  auto &program = pipeline.program_ref();

  auto const view = compute_view(args.camera);
  auto const& projection = args.projection;

  auto const draw_3d_walls_fn = [&](auto const &dinfo) {
    auto const tmatrix = glm::translate(glm::mat4{}, model.translation);
    auto const rmatrix = glm::toMat4(model.rotation);
    auto const smatrix = glm::scale(glm::mat4{}, model.scale);
    auto const mmatrix = tmatrix * rmatrix * smatrix;
    auto const mvmatrix = projection * view * mmatrix;
    pipeline.set_uniform_matrix_4fv(logger, "u_mvmatrix", mvmatrix);

    auto const instance_count = pipeline.instance_count();
    auto const draw_mode = dinfo.draw_mode();
    auto const num_indices = dinfo.num_indices();

    std::size_t offset = 0u;
    auto const draw_tile = [&](auto const& tile) {
      pipeline.set_uniform_array_3fv(logger, "u_offset", tile.pos);

      GLvoid const* p_offset = reinterpret_cast<GLvoid const*>(offset);
      glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, p_offset, instance_count);
      LOG_ANY_GL_ERRORS(logger, "glDrawElementsInstanced");
      offset += sizeof(GLuint) * num_indices;
    };

    for(auto const& tile : tilemap) {
      draw_tile(tile);
    }
  };

  detail::draw_scene(logger, pipeline, dinfo, draw_3d_walls_fn);
}

void enable_depth_tests()
{
  //glCullFace(GL_BACK);
  //glFrontFace(GL_CW);
  //glEnable(GL_CULL_FACE);
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

void init()
{
  glDisable(GL_CULL_FACE);
  enable_depth_tests();
}

void clear_screen(glm::vec4 const& color)
{
  glClearColor(color.x, color.y, color.z, color.w);
}

void begin()
{
  // Render
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void end() {}

} // ns opengl

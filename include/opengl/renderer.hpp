#pragma once
#include <opengl/global.hpp>
#include <opengl/camera.hpp>
#include <opengl/vertex_attribute.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace opengl {

void enable_depth_tests();
void disable_depth_tests();

} // ns opengl

namespace detail {

template <typename L, typename P, typename SHAPE, typename FN>
void
draw_scene(L &logger, P &pipeline, SHAPE const &shape, FN const& fn)
{
  using namespace opengl;
  using C = typename P::CTX;

  auto &program = pipeline.program_ref();
  program.use();
  program.check_errors(logger);

  if constexpr (C::HAS_COLOR_UNIFORM) {
    auto const& ctx = pipeline.ctx();
    program.set_uniform_array_4fv(logger, "u_color", ctx.color());
    program.check_errors(logger);
  }

  // Buffers need to be bound before we call global::set_vertex_attributes(...).
  global::vao_bind(pipeline.ctx().vao());
  //ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, shape.vbo());
  //ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ebo());
  //ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  // Instruct the vertex-processor to enable the vertex attributes for this pipeline.
  va::set_vertex_attributes(logger, pipeline.va());

  LOG_TRACE("before drawing shape ...");

  auto const& ctx = pipeline.ctx();
  if constexpr (C::HAS_TEXTURE) {
    global::texture_bind(ctx.texture());
    ON_SCOPE_EXIT([&ctx]() { global::texture_unbind(ctx.texture()); });
    fn(shape);
  } else {
    fn(shape);
  }
  LOG_TRACE("after drawing shape ...");
  program.check_errors(logger);
}

template <typename S>
constexpr auto
indices_size_in_bytes(S const& s)
{
  return s.indices().size() * sizeof(GLuint);
}

template <typename S>
constexpr std::size_t
vertices_size_in_bytes(S const& s)
{
  return s.vertices().size() * sizeof(GLfloat);
}

template <typename L, typename P, typename SHAPE>
void render_shape(L &logger, P &pipeline, SHAPE const& shape)
{
  using C = typename P::CTX;

  auto const draw_mode = shape.draw_mode();
  auto const num_indices = shape.num_indices();
  auto constexpr OFFSET = nullptr;
  //detail::log_shape_bytes(logger, shape);

  //LOG_TRACE(fmt::sprintf("%-15s %-15s %-15s\n", "num bytes", "num floats", "num vertices"));
  //LOG_TRACE(fmt::sprintf("%-15d %-15d %-15d\n", vertices_size_in_bytes(shape),
                            //shape.vertices().size(), shape.num_vertices()));
  //auto const fmt = fmt::sprintf("glDrawElements() render_mode '%d', num_indices '%d'",
                                //shape.draw_mode(), num_indices);
  //LOG_TRACE(fmt);

  if constexpr (C::IS_INSTANCED) {
    auto const& ctx = pipeline.ctx();
    auto const instance_count = ctx.instance_count();

    glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, instance_count);
    LOG_ANY_GL_ERRORS(logger, "glDrawElementsInstanced 1");
  } else {
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
  }
}

template <typename Args, typename P, typename SHAPE>
void draw_3dshape(Args const &args, opengl::Model const& model, P &pipeline, SHAPE const& shape)
{
  using C = typename P::CTX;

  auto const view = compute_view(args.camera);
  auto const& projection = args.projection;

  auto &logger = args.logger;
  auto &program = pipeline.program_ref();

  auto const draw_3d_shape_fn = [&](auto const &shape) {
    auto const tmatrix = glm::translate(glm::mat4{}, model.translation);
    auto const rmatrix = glm::toMat4(model.rotation);
    auto const smatrix = glm::scale(glm::mat4{}, model.scale);
    auto const mmatrix = tmatrix * rmatrix * smatrix;
    auto const mvmatrix = projection * view * mmatrix;
    program.set_uniform_matrix_4fv(logger, "u_mvmatrix", mvmatrix);

    if constexpr (C::IS_SKYBOX) {
      opengl::disable_depth_tests();
      render_shape(logger, pipeline, shape);
      opengl::enable_depth_tests();
    } else {
      render_shape(logger, pipeline, shape);
    }
  };

  draw_scene(logger, pipeline, shape, draw_3d_shape_fn);
}

template <typename Args, typename P, typename SHAPE, typename TILEMAP>
void draw_3dwalls(Args const &args, opengl::Model const& model, P &pipeline, SHAPE const& shape,
    TILEMAP const& tilemap)
{
  using C = typename P::CTX;

  auto const view = compute_view(args.camera);
  auto const& projection = args.projection;

  auto &logger = args.logger;
  auto &program = pipeline.program_ref();

  auto const draw_3d_walls_fn = [&](auto const &shape) {
    auto const tmatrix = glm::translate(glm::mat4{}, model.translation);
    auto const rmatrix = glm::toMat4(model.rotation);
    auto const smatrix = glm::scale(glm::mat4{}, model.scale);
    auto const mmatrix = tmatrix * rmatrix * smatrix;
    auto const mvmatrix = projection * view * mmatrix;
    program.set_uniform_matrix_4fv(logger, "u_mvmatrix", mvmatrix);

    auto const instance_count = pipeline.ctx().instance_count();
    auto const draw_mode = shape.draw_mode();
    auto const num_indices = shape.num_indices();

    std::size_t offset = 0u;
    auto const draw_one = [&](auto const& tile) {
      program.program_set_uniform_array_3fv(logger, "u_offset", tile.pos);

      GLvoid const* p_offset = reinterpret_cast<GLvoid const*>(offset);
      glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, p_offset, instance_count);
      LOG_ANY_GL_ERRORS(logger, "glDrawElementsInstanced");
      offset += sizeof(GLuint) * num_indices;
    };

    for(auto const& tile : tilemap) {
      draw_one(tile);
    }
  };

  draw_scene(logger, pipeline, shape, draw_3d_walls_fn);
}

template <typename Args, typename P, typename SHAPE>
void
draw_2dshape(Args const &args, opengl::Model const& model, P &pipeline, SHAPE const &shape)
{
  using namespace opengl;

  auto &program = pipeline.program_ref();
  auto &logger = args.logger;
  auto const draw_2d_shape_fn = [&](auto const& shape) {
    auto const& t = model.translation;
    auto const tmatrix = glm::translate(glm::mat4{}, glm::vec3{t.x, t.y, 0.0});
    auto const rmatrix = glm::toMat4(model.rotation);
    auto const smatrix = glm::scale(glm::mat4{}, model.scale);
    auto const mvmatrix = tmatrix * rmatrix * smatrix;
    program.set_uniform_matrix_4fv(logger, "u_mvmatrix", mvmatrix);
    render_shape(logger, pipeline, shape);
  };

  draw_scene(logger, pipeline, shape, draw_2d_shape_fn);
}

} // ns detail

namespace opengl {

template <typename Args, typename PIPELINE_SHAPE>
void draw(Args const& args, Model const& model, PIPELINE_SHAPE const& pipeline_shape)
{
  using PIPE = typename PIPELINE_SHAPE::PIPE;
  using C = typename PIPE::CTX;

  auto const& shape = pipeline_shape.shape;
  auto &pipeline = pipeline_shape.pipeline;
  auto &logger = args.logger;

  if constexpr (C::IS_2D) {
    opengl::disable_depth_tests();
    ::detail::draw_2dshape(args, model, pipeline, shape);
    opengl::enable_depth_tests();
  } else {
    ::detail::draw_3dshape(args, model, pipeline, shape);
  }
}

template <typename Args, typename PIPELINE_SHAPE, typename TILEMAP>
void draw_tilemap(Args const& args, opengl::Model const& model, PIPELINE_SHAPE const& tilemap_shape,
    TILEMAP const& tilemap)
{
  using PIPE = typename PIPELINE_SHAPE::PIPE;
  using C = typename PIPE::CTX;

  auto const& shape = tilemap_shape.shape;
  auto &pipeline = tilemap_shape.pipeline;
  auto &logger = args.logger;

  auto const draw_fn = [&](auto const& shape) {
    ::detail::draw_3dwalls(args, model, pipeline, shape, tilemap);
  };

  ::detail::draw_scene(logger, pipeline, shape, draw_fn);
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

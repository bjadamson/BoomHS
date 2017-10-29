#pragma once
#include <opengl/context.hpp>

#include <opengl/global.hpp>
#include <opengl/camera.hpp>

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

  // Instruct the vertex-processor to enable the vertex attributes for this context.
  global::set_vertex_attributes(logger, pipeline.va());

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

template <typename L, typename S>
void
log_shape_bytes(L &logger, S const &shape)
{
  assert(0 < shape.vertices().size());

  auto const print_bytes = [](auto &ostream, auto const length, auto const *data) {
    auto i = 0u;
    ostream << "[";
    ostream << std::to_string(data[i++]);

    for (; i < length; ++i) {
      ostream << ", " << std::to_string(data[i]);
    }
    ostream << "]";
    ostream << "\n";
  };

  std::stringstream ostream;
  ostream << fmt::sprintf("vertices: %-15s %-15s %-15s\n", "num bytes", "num floats", "num vertices");
  ostream << fmt::sprintf("          %-15d %-15d %-15d\n", vertices_size_in_bytes(shape),
                            shape.vertices().size(), shape.num_vertices());

  ostream << fmt::sprintf("indices count '%d', indices_size_in_bytes %d\n", shape.indices().size(),
                          indices_size_in_bytes(shape));
  ostream << "indices(bytes):\n";

  print_bytes(ostream, shape.vertices().size(), shape.vertices().data());
  print_bytes(ostream, shape.indices().size(), shape.indices().data());

  //std::cerr << ostream.str();
  //LOG_TRACE(ostream.str());
}

/*
template <typename L, typename S>
void
render_shape(L &logger, S const& shape)
{
  LOG_TRACE(fmt::sprintf("%-15s %-15s %-15s\n", "num bytes", "num floats", "num vertices"));
  LOG_TRACE(fmt::sprintf("%-15d %-15d %-15d\n", vertices_size_in_bytes(shape),
                            shape.vertices().size(), shape.num_vertices()));
  auto const fmt = fmt::sprintf("glDrawElements() render_mode '%d', indices_count '%d'",
                                shape.draw_mode(), shape.indices().size());

  //LOG_TRACE(fmt);
  //detail::log_shape_bytes(logger, shape);

  auto constexpr OFFSET = nullptr;
  glDrawElements(shape.draw_mode(), shape.indices().size(), GL_UNSIGNED_INT, OFFSET);
}

template <typename L, typename S>
void
render_shape_with_instancing(L &logger, S const& shape, GLsizei const prim_count)
{
  auto constexpr OFFSET = nullptr;
  glDrawElementsInstanced(shape.draw_mode(), shape.indices().size(), GL_UNSIGNED_INT, OFFSET,
      prim_count);
}
*/

template <typename L, typename P, typename SHAPE>
void render_shape(L &logger, P &pipeline, SHAPE const& shape)
{
  using C = typename P::CTX;

  auto const draw_mode = shape.draw_mode();
  auto const indices_size = shape.indices().size();
  auto constexpr OFFSET = nullptr;
  //detail::log_shape_bytes(logger, shape);

  if constexpr (C::IS_INSTANCED) {
    auto const& ctx = pipeline.ctx();
    auto const instance_count = ctx.instance_count();
    glDrawElementsInstanced(draw_mode, indices_size, GL_UNSIGNED_INT, OFFSET, instance_count);
  } else {
    LOG_TRACE(fmt::sprintf("%-15s %-15s %-15s\n", "num bytes", "num floats", "num vertices"));
    LOG_TRACE(fmt::sprintf("%-15d %-15d %-15d\n", vertices_size_in_bytes(shape),
                              shape.vertices().size(), shape.num_vertices()));
    auto const fmt = fmt::sprintf("glDrawElements() render_mode '%d', indices_count '%d'",
                                  shape.draw_mode(), shape.indices().size());

    //LOG_TRACE(fmt);
    glDrawElements(draw_mode, indices_size, GL_UNSIGNED_INT, OFFSET);
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
    detail::draw_2dshape(args, model, pipeline, shape);
    opengl::enable_depth_tests();
  } else {
    detail::draw_3dshape(args, model, pipeline, shape);
  }
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

template<typename L, typename PIPELINE_SHAPE>
void
copy_to_gpu(L &logger, PIPELINE_SHAPE &pipeline_shape)
{
  auto &shape = pipeline_shape.shape;
  detail::log_shape_bytes(logger, shape);

  auto &pipeline = pipeline_shape.pipeline;

  opengl::global::vao_bind(pipeline.ctx().vao());
  ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, shape.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  // copy the vertices
  auto const vertices_size = detail::vertices_size_in_bytes(shape);
  auto const& vertices_data = shape.vertices().data();
  glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices_data, GL_STATIC_DRAW);

  // copy the vertice rendering order
  auto const indices_size = detail::indices_size_in_bytes(shape);
  auto const& indices_data = shape.indices().data();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_data, GL_STATIC_DRAW);

  shape.set_is_in_gpu_memory(true);
}

} // ns opengl

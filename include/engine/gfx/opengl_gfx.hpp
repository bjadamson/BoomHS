#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/polygon_renderer.hpp>
#include <engine/gfx/opengl/shape_map.hpp>
#include <engine/gfx/opengl_glew.hpp>
#include <engine/gfx/opengl/vertex_attrib.hpp>
#include <engine/gfx/shapes.hpp>
#include <engine/window/sdl_window.hpp>
#include <game/data_types.hpp>
#include <stlw/type_ctors.hpp>
#include <glm/glm.hpp>

namespace engine::gfx::opengl
{

namespace impl {

namespace gl = engine::gfx::opengl;

stlw::result<gl::program, std::string>
load_program(vertex_shader_filename const v, fragment_shader_filename const f)
{
  auto expected_program_id = gl::program_loader::load(v.filename, f.filename);
  if (!expected_program_id) {
    return stlw::make_error(expected_program_id.error());
  }
  return expected_program_id;
}

} // ns impl

auto log_error = [](auto const line) {
  GLenum err = GL_NO_ERROR;
  while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "GL error! '" << std::hex << err << "' line #" << std::to_string(line)
              << std::endl;
    return;
  }
  std::string error = SDL_GetError();
  if (error != "") {
    std::cout << "SLD Error : " << error << std::endl;
    SDL_ClearError();
  }
};

template<typename L>
struct render_args
{
  L &logger;
  glm::mat4 const& view;
  glm::mat4 const& projection;

  render_args(L &l, glm::mat4 const& v, glm::mat4 const& p)
    : logger(l)
    , view(v)
    , projection(p)
  {
  }
};

class gfx_engine
{
  using W = ::engine::window::sdl_window;

  W window_;
  polygon_renderer pr0_, pr1_;
  render_context rc0_, rc1_;

  gfx_engine(W &&w, polygon_renderer &&p0, render_context &&r0, polygon_renderer &&p1,
      render_context &&r1)
    : window_(std::move(w))
    , pr0_(std::move(p0))
    , pr1_(std::move(p1))
    , rc0_(std::move(r0))
    , rc1_(std::move(r1))
  {
  }

  NO_COPY(gfx_engine);
  gfx_engine &operator=(gfx_engine &&) = delete;

  friend struct opengl_library;

public:
  // move-assignment OK.
  gfx_engine(gfx_engine &&other)
      : window_(std::move(other.window_))
      , pr0_(std::move(other.pr0_))
      , pr1_(std::move(other.pr1_))
      , rc0_(std::move(other.rc0_))
      , rc1_(std::move(other.rc1_))
  {
    // background color
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  }

  void begin()
  {
    // Render
    glClear(GL_COLOR_BUFFER_BIT);
  }
  void end()
  {
    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
  }

  template <typename L, typename ...S>
  void draw0(render_args<L> const& args, S const&... shapes)
  {
    auto const gl_mapped_shapes = map_to_gl(shapes...);
    this->pr0_.draw(args.logger, this->rc0_, args.view, args.projection, gl_mapped_shapes);
  }

  template <typename L, typename ...S>
  void draw1(render_args<L> const& args, S const&... shapes)
  {
    auto const gl_mapped_shapes = map_to_gl(shapes...);

    this->pr1_.draw(args.logger, this->rc1_, args.view, args.projection, gl_mapped_shapes);
  }
};

struct opengl_library {
  opengl_library() = delete;

  static void init()
  {
    // for now, to simplify rendering
    glEnable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);
  }

  static inline void destroy() {}

  template <typename L, typename W>
  static inline stlw::result<gfx_engine, std::string> make_gfx_engine(L &logger, W &&window)
  {
    auto const make_polygon_renderer = [](auto &logger, auto const& rc)
    {
      polygon_renderer polyr;
      auto vertex_attribute_config = make_vertex_attribute_config(rc);
      global::set_vertex_attributes(logger, vertex_attribute_config);
      return polyr;
    };

    // TODO: can they share the same program???
    DO_MONAD(auto phandle0, impl::load_program("shader.vert", "shader.frag"));
    auto rc0 = context_factory::make_render_context(std::move(phandle0), "assets/wall.jpg");

    DO_MONAD(auto phandle1, impl::load_program("shader.vert", "shader.frag"));
    auto rc1 = context_factory::make_render_context(std::move(phandle1), "assets/container.jpg");

    auto pr0 = make_polygon_renderer(logger, rc0);
    auto pr1 = make_polygon_renderer(logger, rc1);
    return gfx_engine{std::move(window), std::move(pr0), std::move(rc0), std::move(pr1), std::move(rc1)};
  }
};

} // ns engine::gfx::opengl

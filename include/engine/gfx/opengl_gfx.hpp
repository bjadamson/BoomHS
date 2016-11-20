#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/factory.hpp>
#include <engine/gfx/opengl/program.hpp>
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

  polygon_renderer poly_renderer_;
  W window_;
  render_context rc_;

  gfx_engine(W &&w, polygon_renderer &&poly_r, render_context &&rc)
      : window_(std::move(w))
      , poly_renderer_(std::move(poly_r))
      , rc_(std::move(rc))
  {
  }

  NO_COPY(gfx_engine);
  gfx_engine &operator=(gfx_engine &&) = delete;

  friend struct opengl_library;

public:
  // move-assignment OK.
  gfx_engine(gfx_engine &&other)
      : poly_renderer_(std::move(other.poly_renderer_))
      , window_(std::move(other.window_))
      , rc_(std::move(other.rc_))
  {
  }

  template <typename L, typename ...S>
  void draw(render_args<L> const& args, S const&... shapes)
  {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    auto const gl_mapped_shapes = map_to_gl(shapes...);

    // Render
    glClear(GL_COLOR_BUFFER_BIT);
    this->poly_renderer_.draw(args.logger, this->rc_, args.view, args.projection, gl_mapped_shapes);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
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

    DO_MONAD(auto phandle, impl::load_program("shader.vert", "shader.frag"));
    render_context rc{std::move(phandle)};
    auto poly_renderer = make_polygon_renderer(logger, rc);
    return gfx_engine{std::move(window), std::move(poly_renderer), std::move(rc)};
  }
};

} // ns engine::gfx::opengl

#pragma once
#include <engine/gfx/opengl/factory.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/shape_map.hpp>
#include <engine/gfx/opengl_glew.hpp>
#include <engine/gfx/shapes.hpp>
#include <engine/window/sdl_window.hpp>
#include <game/data_types.hpp>
#include <stlw/type_ctors.hpp>

#include <glm/glm.hpp>

namespace {

constexpr auto
wc_to_gfx_triangle(game::world_coordinate const& wc)
{
  using namespace engine::gfx;
  constexpr float radius = 0.5;

  // clang-format off
 std::array<float, 12> v0 =
  {
    wc.x() - radius, wc.y() - radius, wc.z(), wc.w(), // bottom left
    wc.x() + radius, wc.y() - radius, wc.z(), wc.w(), // bottom right
    wc.x()         , wc.y() + radius, wc.z(), wc.w()  // top middle
  };
  constexpr std::array<float, 12> c0 =
  {
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
  };
  return ::engine::gfx::make_triangle(v0, c0);
  // clang-format on
}

constexpr auto
wc_to_gfx_rectangle(game::world_coordinate const& wc)
{
  using namespace engine::gfx;
  constexpr float radius = 0.5;

  // clang-format off
 std::array<float, 16> v0 =
  {
    wc.x(), wc.y(), wc.z(), wc.w(),
    wc.x(), wc.y(), wc.z(), wc.w(),
    wc.x(), wc.y(), wc.z(), wc.w()
  };
  constexpr std::array<float, 16> c0 =
  {
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.2f, 1.0f,
  };
  return ::engine::gfx::make_rectangle(v0, c0);
  // clang-format on
}

} // nsa non


namespace engine
{
namespace gfx
{
namespace opengl
{

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

auto const bind_vao = [](auto const vao) { glBindVertexArray(vao); };
auto const unbind_vao = [](auto const vao) { glBindVertexArray(0); };

template<typename L>
struct render_args
{
  L &logger;
  glm::mat4 const& view;
  glm::mat4 const& projection;

  game::world_coordinate const& wc0;
  game::world_coordinate const& wc1;

  render_args(L &l, glm::mat4 const& v, glm::mat4 const& p, game::world_coordinate const& w,
      game::world_coordinate const& ww)
    : logger(l)
    , view(v)
    , projection(p)
    , wc0(w)
    , wc1(ww)
  {
  }
};

class gfx_engine
{
  using W = ::engine::window::sdl_window;

  renderer red_;
  W window_;

  gfx_engine(W &&w, renderer &&red)
      : window_(std::move(w))
      , red_(std::move(red))
  {
  }

  NO_COPY(gfx_engine);
  gfx_engine &operator=(gfx_engine &&) = delete;

  friend struct opengl_library;

public:
  // move-assignment OK.
  gfx_engine(gfx_engine &&other)
      : red_(std::move(other.red_))
      , window_(std::move(other.window_))
  {
  }

  template <typename L>
  void draw(render_args<L> const& args)
  {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    // IS THERE SOME WAY TO ABSTRAC
    auto const wc0 = wc_to_gfx_triangle(args.wc0);
    auto const wc1 = wc_to_gfx_triangle(args.wc1);

    //auto const wc0 = wc_to_gfx_rectangle(args.wc0);
    //auto const wc1 = wc_to_gfx_rectangle(args.wc1);

    // Render
    glClear(GL_COLOR_BUFFER_BIT);
    this->red_.draw(args.logger, args.view, args.projection, map_to_gl(wc0), map_to_gl(wc1));

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
    DO_MONAD(auto red, factory::make_renderer(logger));
    return gfx_engine{std::move(window), std::move(red)};
  }
};

} // ns opengl
} // ns gfx
} // ns engine

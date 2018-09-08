#include <boomhs/frame_time.hpp>
#include <common/log.hpp>
#include <common/timer.hpp>

#include <gl_sdl/sdl_window.hpp>
#include <opengl/factory.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>

#include <cstdlib>

using namespace boomhs;
using namespace common;
using namespace gl_sdl;
using namespace opengl;

namespace OR = opengl::render;

void
draw_rectangle(common::Logger& logger, Color const& color)
{
  auto const v = OF::rectangle_vertices_default();
  OF::RectInfo const ri{1.0f, 1.0f, color, std::nullopt, std::nullopt};
  RectBuffer  buffer = OF::make_rectangle(ri);


  std::vector<opengl::AttributePointerInfo> apis;

  {
    auto const attr_type    = attribute_type_from_string("position");
    apis.emplace_back(opengl::AttributePointerInfo{0, GL_FLOAT, attr_type, 3});
  }
  {
    auto const attr_type    = attribute_type_from_string("color");
    apis.emplace_back(opengl::AttributePointerInfo{1, GL_FLOAT, attr_type, 4});
  }

  auto va = make_vertex_attribute(apis);
  auto sp = make_shader_program(logger, "2dcolor.vert", "2dcolor.frag", MOVE(va))
    .expect_moveout("Error loading 2dcolor shader program");
  DrawInfo dinfo = gpu::copy_rectangle(logger, sp.va(), buffer);

  Transform  transform;
  auto const model_matrix = transform.model_matrix();

  DrawState ds{false};
  ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();
  sp.while_bound(logger, [&]() {
      OR::set_modelmatrix(logger, model_matrix, sp);

      BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
      OR::draw_2delements(logger, GL_TRIANGLES, sp, dinfo.num_indices());
  });
}

bool
process_event(SDL_Event& event)
{
  bool const event_type_keydown = event.type == SDL_KEYDOWN;
  auto const key_pressed        = event.key.keysym.sym;

  if (event_type_keydown) {
    switch (key_pressed) {
      case SDLK_F10:
      case SDLK_ESCAPE:
        return true;
        break;
      default:
        break;
    }
  }
  return event.type == SDL_QUIT;
}

int
main(int argc, char **argv)
{
  auto logger = common::LogFactory::make_stderr();
  LOG_ERROR("TEST HELLO WORLD");

  bool constexpr FULLSCREEN = false;
  auto const on_error = [&logger](auto const& error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  TRY_OR_ELSE_RETURN(auto sdl_gl, SDLGlobalContext::create(logger), on_error);
  TRY_OR_ELSE_RETURN(auto window, sdl_gl->make_window(logger, FULLSCREEN, 1024, 768), on_error);

  LOG_ERROR("A");

  OR::init(logger);
  LOG_ERROR("B");
  OR::set_viewport(ScreenDimensions{0, 0, 800, 600});
  LOG_ERROR("C");

  Timer timer;
  FrameCounter fcounter;

  SDL_Event event;
  bool quit = false;
  while (!quit) {
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      auto const clear_color = LOC::BLACK;
      OR::clear_screen(clear_color);
      draw_rectangle(logger, LOC::RED);

      quit = process_event(event);

      // Update window with OpenGL rendering
      SDL_GL_SwapWindow(window.raw());
    }
  }
  LOG_ERROR("E");

  return EXIT_SUCCESS;
}

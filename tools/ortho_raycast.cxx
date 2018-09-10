#include <boomhs/frame_time.hpp>
#include <common/log.hpp>
#include <common/timer.hpp>
#include <common/type_macros.hpp>

#include <gl_sdl/sdl_window.hpp>

#include <opengl/bind.hpp>
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

auto
make_program_and_rectangle(common::Logger& logger, Color const& color,
                           Rectangle const& view_rect)
{
  auto constexpr TOP_LEFT = glm::vec2{10, 10};
  auto const TL_NDC       = math::space_conversions::screen_to_ndc(TOP_LEFT, view_rect);

  auto constexpr BOTTOM_RIGHT = glm::vec2{80, 80};
  auto const BR_NDC           = math::space_conversions::screen_to_ndc(BOTTOM_RIGHT, view_rect);

  Rectangle      const rect{TL_NDC.x, TL_NDC.y, BR_NDC.x, BR_NDC.y};
  OF::RectangleColors const rect_colors{color, std::nullopt};
  OF::RectInfo const ri{rect, rect_colors, std::nullopt};
  RectBuffer  buffer = OF::make_rectangle(ri);


  std::vector<opengl::AttributePointerInfo> apis;

  {
    auto const attr_type    = AttributeType::POSITION;
    apis.emplace_back(AttributePointerInfo{0, GL_FLOAT, attr_type, 3});
  }
  {
    auto const attr_type    = AttributeType::COLOR;
    apis.emplace_back(AttributePointerInfo{1, GL_FLOAT, attr_type, 4});
  }

  auto va = make_vertex_attribute(apis);
  auto sp = make_shader_program(logger, "2dcolor.vert", "2dcolor.frag", MOVE(va))
    .expect_moveout("Error loading 2dcolor shader program");
  return PAIR(MOVE(sp), gpu::copy_rectangle(logger, sp.va(), buffer));
}

void
draw_rectangle(common::Logger& logger, ShaderProgram& sp, DrawInfo& dinfo)
{
  Transform  transform;
  auto const model_matrix = transform.model_matrix();

  DrawState ds{false};
  ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();
  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  OR::set_modelmatrix(logger, model_matrix, sp);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  OR::draw_2delements(logger, GL_TRIANGLES, sp, dinfo.num_indices());
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
  bool constexpr FULLSCREEN = false;
  int constexpr  WIDTH = 200, HEIGHT = 150;

  auto logger = common::LogFactory::make_stderr();
  auto const on_error = [&logger](auto const& error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  TRY_OR_ELSE_RETURN(auto sdl_gl, SDLGlobalContext::create(logger), on_error);
  TRY_OR_ELSE_RETURN(auto window, sdl_gl->make_window(logger, FULLSCREEN, WIDTH, HEIGHT), on_error);

  ScreenDimensions constexpr SCREEN_DIM{0, 0, WIDTH, HEIGHT};
  OR::init(logger);
  OR::set_viewport(SCREEN_DIM);

  Timer timer;
  FrameCounter fcounter;

  SDL_Event event;
  bool quit = false;

  auto pair = make_program_and_rectangle(logger, LOC::RED, SCREEN_DIM.rect());
  while (!quit) {
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      OR::clear_screen(LOC::BLACK);
      draw_rectangle(logger, pair.first, pair.second);

      quit = process_event(event);

      // Update window with OpenGL rendering
      SDL_GL_SwapWindow(window.raw());
    }
  }
  return EXIT_SUCCESS;
}

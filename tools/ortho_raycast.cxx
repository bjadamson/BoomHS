#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
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
using namespace boomhs::math;
using namespace common;
using namespace gl_sdl;
using namespace opengl;

namespace OR = opengl::render;

auto
make_program_and_bbox(common::Logger& logger)
{
  std::vector<opengl::AttributePointerInfo> apis;
  {
    auto const attr_type    = AttributeType::POSITION;
    apis.emplace_back(AttributePointerInfo{0, GL_FLOAT, attr_type, 3});
  }

  auto va = make_vertex_attribute(apis);
  auto sp = make_shader_program(logger, "wireframe.vert", "wireframe.frag", MOVE(va))
    .expect_moveout("Error loading wireframe shader program");

  CubeRenderable const cr{glm::vec3{0}, glm::vec3{1}};
  auto const vertices = OF::cube_vertices(cr.min, cr.max);

  return PAIR(MOVE(sp), gpu::copy_cube_gpu(logger, vertices, sp.va()));
}

auto
make_program_and_rectangle(common::Logger& logger, Rectangle const& rect,
                           Rectangle const& view_rect)
{
  auto const TOP_LEFT = glm::vec2{rect.left(), rect.top()};
  //auto const TL_NDC   = math::space_conversions::screen_to_ndc(TOP_LEFT, view_rect);

  auto const BOTTOM_RIGHT = glm::vec2{rect.right(), rect.bottom()};
  //auto const BR_NDC       = math::space_conversions::screen_to_ndc(BOTTOM_RIGHT, view_rect);

  Rectangle      const ndc_rect{TOP_LEFT.x, TOP_LEFT.y, BOTTOM_RIGHT.x, BOTTOM_RIGHT.y};
  OF::RectInfo const ri{ndc_rect, std::nullopt, std::nullopt, std::nullopt};
  RectBuffer  buffer = OF::make_rectangle(ri);

  std::vector<opengl::AttributePointerInfo> apis;
  {
    auto const attr_type    = AttributeType::POSITION;
    apis.emplace_back(AttributePointerInfo{0, GL_FLOAT, attr_type, 3});
  }

  auto va = make_vertex_attribute(apis);
  auto sp = make_shader_program(logger, "2dcolor.vert", "2dcolor.frag", MOVE(va))
    .expect_moveout("Error loading 2dcolor shader program");
  return PAIR(MOVE(sp), gpu::copy_rectangle(logger, sp.va(), buffer));
}

auto
calculate_pm(ScreenDimensions const& sd)
{
  auto constexpr NEAR   = 1.0f;
  auto constexpr FAR    = -1.0f;
  Frustum const f{
    static_cast<float>(sd.left()),
    static_cast<float>(sd.right()),
    static_cast<float>(sd.bottom()),
    static_cast<float>(sd.top()),
    NEAR,
    FAR};
  return glm::ortho(f.left, f.right, f.bottom, f.top, f.near, f.far);
}

void
draw_bbox(common::Logger& logger, ScreenDimensions const& sd, ShaderProgram& sp, DrawInfo& dinfo,
          DrawState& ds)
{
  auto const pm = calculate_pm(sd);

  Color const wire_color = LOC::BLUE;

  Transform tr;
  auto const model_matrix = tr.model_matrix();


  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  sp.set_uniform_color(logger, "u_wirecolor", wire_color);

  auto const pos = glm::vec3{0, 5, 0};
  auto const forward = -constants::Y_UNIT_VECTOR;
  auto const up      = -constants::Z_UNIT_VECTOR;
  auto const vm = glm::lookAt(pos, pos + forward, up);
  //auto const vm = glm::mat4{};

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  auto const camera_matrix = pm * vm;
  render::set_mvpmatrix(logger, camera_matrix, model_matrix, sp);
  render::draw(logger, ds, GL_LINES, sp, dinfo);
}

void
draw_rectangle(common::Logger& logger, ScreenDimensions const& sd, ShaderProgram& sp,
                     DrawInfo& dinfo, Color const& color, DrawState& ds)
{
  auto const pm = calculate_pm(sd);

  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  sp.set_uniform_matrix_4fv(logger, "u_projmatrix", pm);
  sp.set_uniform_color(logger, "u_color", color);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  OR::draw_2delements(logger, GL_TRIANGLES, sp, dinfo.num_indices());
}

bool
process_event(common::Logger& logger, SDL_Event& event, Rectangle const& rect, Color* color)
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
  else if (event.type == SDL_MOUSEMOTION) {
    auto const& motion = event.motion;
    float const x = motion.x, y = motion.y;
    auto const p = glm::vec2{x, y};
    if (collision::point_rectangle_intersects(p, rect)) {
      *color = LOC::GREEN;
      //LOG_ERROR_SPRINTF("mouse pos: %f:%f", p.x, p.y);
    }
    else {
      *color = LOC::RED;
    }
  }
  return event.type == SDL_QUIT;
}

int
main(int argc, char **argv)
{
  bool constexpr FULLSCREEN = false;
  int constexpr  WIDTH =  1024, HEIGHT = 768;

  auto logger = common::LogFactory::make_stderr();
  auto const on_error = [&logger](auto const& error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  TRY_OR_ELSE_RETURN(auto sdl_gl, SDLGlobalContext::create(logger), on_error);
  TRY_OR_ELSE_RETURN(auto window,
      sdl_gl->make_window(logger, "Ortho Raycast Test", FULLSCREEN, WIDTH, HEIGHT), on_error);

  ScreenDimensions constexpr SCREEN_DIM{0, 0, WIDTH, HEIGHT};
  OR::init(logger);
  OR::set_viewport(SCREEN_DIM);

  auto const view_rect = SCREEN_DIM.rect();
  auto const color_rect = Rectangle{WIDTH / 4, HEIGHT / 4, WIDTH / 2, HEIGHT / 2};
  auto rect_pair = make_program_and_rectangle(logger, color_rect, view_rect);
  auto bbox_pair = make_program_and_bbox(logger);

  Timer timer;
  FrameCounter fcounter;

  auto color = LOC::RED;
  SDL_Event event;
  bool quit = false;
  while (!quit) {
    auto const ft = FrameTime::from_timer(timer);
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      quit = process_event(logger, event, color_rect, &color);
    }

    OR::clear_screen(LOC::WHITE);


    DrawState ds;
    draw_rectangle(logger, SCREEN_DIM, rect_pair.first, rect_pair.second, color, ds);
    draw_bbox(logger, SCREEN_DIM, bbox_pair.first, bbox_pair.second, ds);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());

    timer.update();
    fcounter.update();
  }
  return EXIT_SUCCESS;
}

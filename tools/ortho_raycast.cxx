#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/raycast.hpp>

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

// clang-format off
static auto constexpr NEAR = 0.001f;
static auto constexpr FAR  = 500.0f;
static auto constexpr FOV  = glm::radians(110.0f);
static auto constexpr AR   = AspectRatio{4.0f, 3.0f};

static int constexpr WIDTH  = 1024;
static int constexpr HEIGHT = 768;
ScreenDimensions constexpr SCREEN_DIM{0, 0, WIDTH, HEIGHT};
static Frustum constexpr FRUSTUM{
    SCREEN_DIM.float_left(),
    SCREEN_DIM.float_right(),
    SCREEN_DIM.float_bottom(),
    SCREEN_DIM.float_top(),
    NEAR,
    FAR};
// clang-format on

static glm::vec3  CAMERA_POS   = glm::vec3{0, 1, 0};
static auto const VIEW_FORWARD = -constants::Y_UNIT_VECTOR;
static auto const VIEW_UP      =  constants::Z_UNIT_VECTOR;

namespace OR = opengl::render;

auto
make_program_and_bbox(common::Logger& logger, Cube const& cr)
{
  std::vector<opengl::AttributePointerInfo> const apis{{
    AttributePointerInfo{0, GL_FLOAT, AttributeType::POSITION, 3}
  }};

  auto va = make_vertex_attribute(apis);
  auto sp = make_shader_program(logger, "wireframe.vert", "wireframe.frag", MOVE(va))
    .expect_moveout("Error loading wireframe shader program");

  auto const vertices = OF::cube_vertices(cr.min, cr.max);

  return PAIR(MOVE(sp), gpu::copy_cube_wireframe_gpu(logger, vertices, sp.va()));
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

  std::vector<opengl::AttributePointerInfo> const apis{{
    AttributePointerInfo{0, GL_FLOAT, AttributeType::POSITION, 3}
  }};

  auto va = make_vertex_attribute(apis);
  auto sp = make_shader_program(logger, "2dcolor.vert", "2dcolor.frag", MOVE(va))
    .expect_moveout("Error loading 2dcolor shader program");
  return PAIR(MOVE(sp), gpu::copy_rectangle(logger, sp.va(), buffer));
}

auto
calculate_pm(float const near, float const far)
{
  return glm::ortho(
      FRUSTUM.left,
      FRUSTUM.right,
      FRUSTUM.bottom,
      FRUSTUM.top,
      near,
      far);
}

auto
calculate_vm(common::Logger& logger)
{
  auto const& eye   = CAMERA_POS;
  auto const center = eye + VIEW_FORWARD;
  auto const& up    = VIEW_UP;

  auto r = glm::lookAtRH(eye, center, up);

  // Flip the "right" vector computed by lookAt() so the X-Axis points "right" onto the screen.
  //
  // See implementation of glm::lookAtRH() for more details.
  auto& sx = r[0][0];
  auto& sy = r[1][0];
  auto& sz = r[2][0];
  math::negate(sx, sy, sz);

  return r;
}

void
draw_bbox(common::Logger& logger, glm::mat4 const& pm, glm::mat4 const& vm, Transform const& tr,
          ShaderProgram& sp, DrawInfo& dinfo, DrawState& ds, Color const& color)
{
  auto const model_matrix = tr.model_matrix();

  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  sp.set_uniform_color(logger, "u_wirecolor", color);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  auto const camera_matrix = pm * vm;
  render::set_mvpmatrix(logger, camera_matrix, model_matrix, sp);
  render::draw(logger, ds, GL_LINES, sp, dinfo);
}

void
draw_rectangle_pm(common::Logger& logger, ShaderProgram& sp,
                     DrawInfo& dinfo, Color const& color, DrawState& ds)
{
  int constexpr NEAR = -1.0;
  int constexpr FAR  = 1.0f;
  auto const pm = calculate_pm(-1.0f, 1.0f);

  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  sp.set_uniform_matrix_4fv(logger, "u_projmatrix", pm);
  sp.set_uniform_color(logger, "u_color", color);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  OR::draw_2delements(logger, GL_TRIANGLES, sp, dinfo.num_indices());
}

bool
process_event(common::Logger& logger, SDL_Event& event, Rectangle const& view_rect,
              glm::mat4 const& pm, glm::mat4 const& vm, Transform& tr,
              Rectangle const& pm_rect, Color* pm_rect_color,
              Cube const& cube, Color* wire_color)
{
  bool const event_type_keydown = event.type == SDL_KEYDOWN;
  auto const key_pressed        = event.key.keysym.sym;


  if (event_type_keydown) {
    switch (key_pressed) {
      case SDLK_F10:
      case SDLK_ESCAPE:
        return true;
        break;
      case SDLK_a:
        CAMERA_POS += constants::X_UNIT_VECTOR;
        break;
      case SDLK_d:
        CAMERA_POS -= constants::X_UNIT_VECTOR;
        break;
      case SDLK_w:
        CAMERA_POS += constants::Z_UNIT_VECTOR;
        break;
      case SDLK_s:
        CAMERA_POS -= constants::Z_UNIT_VECTOR;
        break;
      case SDLK_e:
        CAMERA_POS -= constants::Y_UNIT_VECTOR;
        break;
      case SDLK_q:
        CAMERA_POS += constants::Y_UNIT_VECTOR;
        break;

      case SDLK_UP:
        tr.translation -= constants::Z_UNIT_VECTOR;
        break;
      case SDLK_DOWN:
        tr.translation += constants::Z_UNIT_VECTOR;
        break;
      case SDLK_LEFT:
        tr.translation -= constants::X_UNIT_VECTOR;
        break;
      case SDLK_RIGHT:
        tr.translation += constants::X_UNIT_VECTOR;
        break;
      default:
        break;
    }
  }
  else if (event.type == SDL_MOUSEMOTION) {
    auto const& motion = event.motion;
    float const x = motion.x, y = motion.y;
    auto const mouse_pos = glm::vec2{x, y};
    if (collision::point_rectangle_intersects(mouse_pos, pm_rect)) {
      *pm_rect_color = LOC::GREEN;
      //LOG_ERROR_SPRINTF("mouse pos: %f:%f", p.x, p.y);
    }
    else {
      *pm_rect_color = LOC::RED;
    }

    glm::vec3 const ray_dir   = Raycast::calculate_ray_into_screen(mouse_pos, pm,
                                                                   vm, view_rect);
    glm::vec3 const ray_start = CAMERA_POS;
    Ray const  ray{ray_start, ray_dir};

    float distance = 0.0f;
    if (collision::ray_cube_intersect(ray, tr, cube, distance)) {
      *wire_color = LOC::PURPLE;
    }
    else {
      *wire_color = LOC::BLUE;
    }
  }
  return event.type == SDL_QUIT;
}

int
main(int argc, char **argv)
{
  bool constexpr FULLSCREEN = false;

  auto logger = common::LogFactory::make_stderr();
  auto const on_error = [&logger](auto const& error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  TRY_OR_ELSE_RETURN(auto sdl_gl, SDLGlobalContext::create(logger), on_error);
  TRY_OR_ELSE_RETURN(auto window,
      sdl_gl->make_window(logger, "Ortho Raycast Test", FULLSCREEN, WIDTH, HEIGHT), on_error);

  OR::init(logger);
  OR::set_viewport(SCREEN_DIM);

  auto const view_rect = SCREEN_DIM.rect();
  auto const color_rect = Rectangle{WIDTH / 4, HEIGHT / 4, WIDTH / 2, HEIGHT / 2};
  auto rect_pair = make_program_and_rectangle(logger, color_rect, view_rect);

  Cube const cr{glm::vec3{0, 0, 0}, glm::vec3{100, 0, 100}};
  auto bbox_pair = make_program_and_bbox(logger, cr);

  Timer timer;
  FrameCounter fcounter;

  auto color = LOC::RED;
  Color wire_color = LOC::BLUE;

  SDL_Event event;
  bool quit = false;

  Transform tr;
  tr.translation = glm::vec3{5, 0, 5};

  glm::mat4 pm;
  glm::mat4 vm;
  while (!quit) {
    pm = calculate_pm(NEAR, FAR);
    vm = calculate_vm(logger);

    auto const ft = FrameTime::from_timer(timer);
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      quit = process_event(logger, event, view_rect, pm, vm, tr, color_rect, &color, cr, &wire_color);
    }
    LOG_ERROR_SPRINTF("cube tr: %s", glm::to_string(tr.translation));

    OR::clear_screen(LOC::WHITE);


    DrawState ds;
    draw_rectangle_pm(logger, rect_pair.first, rect_pair.second, color, ds);

    draw_bbox(logger, pm, vm, tr, bbox_pair.first, bbox_pair.second, ds, wire_color);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());

    timer.update();
    fcounter.update();
  }
  return EXIT_SUCCESS;
}

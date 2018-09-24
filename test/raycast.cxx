#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/raycast.hpp>
#include <boomhs/random.hpp>

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

static glm::vec3  ORTHO_CAMERA_POS = glm::vec3{0, 1, 0};
static glm::vec3  PERS_CAMERA_POS  = glm::vec3{0, 0, 1};
static bool MOUSE_ON_SCREEN = false;

auto&
active_camera_pos()
{
  return MOUSE_ON_SCREEN
    ? PERS_CAMERA_POS
    : ORTHO_CAMERA_POS;
}

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
calculate_vm_fps(common::Logger& logger)
{
  auto const VIEW_FORWARD = -constants::Z_UNIT_VECTOR;
  auto const VIEW_UP      = constants::Y_UNIT_VECTOR;
  auto const pos          = PERS_CAMERA_POS;
  return glm::lookAtRH(pos, pos + VIEW_FORWARD, VIEW_UP);
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
draw_bboxes(common::Logger& logger, glm::mat4 const& pm, glm::mat4 const& vm,
            std::vector<Transform> const& trs, ShaderProgram& sp, DrawInfo& dinfo, DrawState& ds,
            Color const& color)
{
  for (auto const& tr : trs) {
    draw_bbox(logger, pm, vm, tr, sp, dinfo, ds, color);
  }
}

void
draw_rectangle_pm(common::Logger& logger, CameraORTHO const& camera, ShaderProgram& sp,
                     DrawInfo& dinfo, Color const& color, DrawState& ds)
{
  int constexpr NEAR = -1.0;
  int constexpr FAR  = 1.0f;
  Frustum const f{
    FRUSTUM.left,
    FRUSTUM.right,
    FRUSTUM.bottom,
    FRUSTUM.top,
    NEAR,
    FAR
  };

  auto const pm = camera.calc_pm(AR, f);

  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  sp.set_uniform_matrix_4fv(logger, "u_projmatrix", pm);
  sp.set_uniform_color(logger, "u_color", color);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  OR::draw_2delements(logger, GL_TRIANGLES, sp, dinfo.num_indices());
}

struct ViewportDisplayInfo
{
  Rectangle const& view_rect;
  glm::mat4 const& perspective, view;

  Cube const& cube;
};


bool
process_event(common::Logger& logger, SDL_Event& event,
              ViewportDisplayInfo const& left_vdi, ViewportDisplayInfo const& right_vdi,
              std::vector<Transform>& trs,
              Rectangle const& pm_rect, Color* pm_rect_color,
              Color* wire_color)
{
  bool const event_type_keydown = event.type == SDL_KEYDOWN;
  auto const key_pressed        = event.key.keysym.sym;
  auto &camera_pos = active_camera_pos();

  auto const move_trs = [&trs](auto const& delta_v) {
    for (auto& tr : trs) {
      tr.translation += delta_v;
    }
  };

  if (event_type_keydown) {
    switch (key_pressed) {
      case SDLK_F10:
      case SDLK_ESCAPE:
        return true;
        break;
      case SDLK_a:
        camera_pos += constants::X_UNIT_VECTOR;
        break;
      case SDLK_d:
        camera_pos -= constants::X_UNIT_VECTOR;
        break;
      case SDLK_w:
        camera_pos += constants::Z_UNIT_VECTOR;
        break;
      case SDLK_s:
        camera_pos -= constants::Z_UNIT_VECTOR;
        break;
      case SDLK_e:
        camera_pos -= constants::Y_UNIT_VECTOR;
        break;
      case SDLK_q:
        camera_pos += constants::Y_UNIT_VECTOR;
        break;

      case SDLK_UP:
        move_trs(-constants::Z_UNIT_VECTOR);
        break;
      case SDLK_DOWN:
        move_trs(constants::Z_UNIT_VECTOR);
        break;
      case SDLK_LEFT:
        move_trs(-constants::X_UNIT_VECTOR);
        break;
      case SDLK_RIGHT:
        move_trs(constants::X_UNIT_VECTOR);
        break;
      case SDLK_PAGEUP:
        move_trs(constants::Y_UNIT_VECTOR);
        break;
      case SDLK_PAGEDOWN:
        move_trs(-constants::Y_UNIT_VECTOR);
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

    auto const middle_point = left_vdi.view_rect.right();
    MOUSE_ON_SCREEN = mouse_pos.x > middle_point;

    glm::vec2 mouse_start;
    float distance = 0.0f;
    glm::mat4 const* pm        = nullptr;
    glm::mat4 const* vm        = nullptr;
    Rectangle const* view_rect = nullptr;
    Cube      const* cube      = nullptr;

    if (MOUSE_ON_SCREEN) {
      // RHS
      cube        = &right_vdi.cube;
      view_rect   = &right_vdi.view_rect;
      mouse_start = mouse_pos - glm::vec2{middle_point, 0};

      pm = &right_vdi.perspective;
      vm = &right_vdi.view;
    }
    else {
      // LHS
      cube        = &left_vdi.cube;
      view_rect   = &left_vdi.view_rect;
      mouse_start = mouse_pos;

      pm = &left_vdi.perspective;
      vm = &left_vdi.view;
    }

    auto const& camera_pos  = active_camera_pos();
    glm::vec3 const ray_dir = Raycast::calculate_ray_into_screen(mouse_start, *pm,
                                                                  *vm, *view_rect);
    glm::vec3 const ray_start = camera_pos;
    Ray const  ray{ray_start, ray_dir};

    bool collision = false;
    for (auto const& tr : trs) {
      collision |= collision::ray_cube_intersect(ray, tr, *cube, distance);
      if (collision) {
        break;
      }
    }
    if (collision) {
      *wire_color = LOC::PURPLE;
    }
    else {
      *wire_color = LOC::BLUE;
    }
  }
  return event.type == SDL_QUIT;
}

auto
gen_trs(RNG &rng)
{
  auto const gen = [&rng](auto const& l, auto const& h) { return rng.gen_float_range(l, h); };
  auto const gen_low_x = [&gen]() { return gen(0, WIDTH); };
  auto const gen_low_z = [&gen]() { return gen(0, HEIGHT); };
  auto const gen_tr = [&]() { return glm::vec3{gen_low_x(), 0, gen_low_z()}; };

  std::vector<Transform> trs;
  FOR(i, 30) {
    trs.emplace_back(gen_tr());
  }
  return trs;
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
  glEnable(GL_SCISSOR_TEST);

  auto const MIDDLE_HORIZ = SCREEN_DIM.right() / 2;
  auto const LHS = ScreenDimensions{
    SCREEN_DIM.left(),
    SCREEN_DIM.top(),
    MIDDLE_HORIZ,
    SCREEN_DIM.bottom()
  };
  auto const RHS = ScreenDimensions{
    MIDDLE_HORIZ,
    SCREEN_DIM.top(),
    SCREEN_DIM.right(),
    SCREEN_DIM.bottom()
  };

  auto const PERS_FORWARD = -constants::Z_UNIT_VECTOR;
  auto constexpr PERS_UP  =  constants::Y_UNIT_VECTOR;
  WorldOrientation const PERS_WO{PERS_FORWARD, PERS_UP};

  auto const ORTHO_FORWARD = -constants::Y_UNIT_VECTOR;
  auto constexpr ORTHO_UP  =  constants::Z_UNIT_VECTOR;
  WorldOrientation const ORTHO_WO{ORTHO_FORWARD, ORTHO_UP};
  CameraORTHO cam_ortho{ORTHO_WO, glm::vec2{WIDTH, HEIGHT}};

  CameraTarget cam_target;

  EntityRegistry er;
  auto const eid = er.create();
  WorldObject  wo{eid, er, PERS_WO};
  cam_target.set(wo);
  CameraFPS cam_fps{cam_target, PERS_WO};

  auto const lhs_view_rect = LHS.rect();
  auto const rhs_view_rect = RHS.rect();

  auto const color_rect = Rectangle{
    200, 200, 400, 400};
  auto rect_pair = make_program_and_rectangle(logger, color_rect, SCREEN_DIM.rect());

  glm::vec3 const min{0, 0, 0};
  glm::vec3 const max{100, 100, 100};
  Cube const cr{min, max};
  Cube const cr_LHS{glm::vec3{min.x, 0, min.z}, glm::vec3{max.x, 0, max.z}};
  auto bbox_pair = make_program_and_bbox(logger, cr);

  auto const pers_pm = glm::perspective(FOV, AR.compute(), FRUSTUM.near, FRUSTUM.far);

  Timer timer;
  FrameCounter fcounter;

  auto color = LOC::RED;
  Color wire_color = LOC::BLUE;

  SDL_Event event;
  bool quit = false;

  RNG rng;
  std::vector<Transform> trs = gen_trs(rng);

  while (!quit) {
    glm::mat4 const ortho_pm = cam_ortho.calc_pm(AR, FRUSTUM);
    glm::mat4 const ortho_vm = cam_ortho.calc_vm();

    glm::mat4 const pers_vm  = calculate_vm_fps(logger);

    auto const ft = FrameTime::from_timer(timer);
    while ((!quit) && (0 != SDL_PollEvent(&event))) {

      ViewportDisplayInfo const left_vdi{lhs_view_rect, ortho_pm, ortho_vm, cr_LHS};
      ViewportDisplayInfo const right_vdi{rhs_view_rect, pers_pm,  pers_vm, cr};
      quit = process_event(logger, event,
          left_vdi, right_vdi,
          trs,
          color_rect, &color,
          &wire_color);
    }

    auto const& camera_pos = active_camera_pos();
    LOG_ERROR_SPRINTF("camera pos: %s",
        glm::to_string(camera_pos));

    OR::set_viewport(LHS);
    OR::set_scissor(LHS);
    OR::clear_screen(LOC::WHITE);

    DrawState ds;
    draw_bboxes(logger, ortho_pm, ortho_vm, trs, bbox_pair.first, bbox_pair.second, ds, wire_color);

    OR::set_viewport(RHS);
    OR::set_scissor(RHS);
    OR::clear_screen(LOC::BLACK);

    draw_bboxes(logger, pers_pm, pers_vm, trs, bbox_pair.first, bbox_pair.second, ds, wire_color);

    OR::set_scissor(SCREEN_DIM);
    OR::set_viewport(SCREEN_DIM);
    draw_rectangle_pm(logger, cam_ortho, rect_pair.first, rect_pair.second, color, ds);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());

    timer.update();
    fcounter.update();
  }
  return EXIT_SUCCESS;
}

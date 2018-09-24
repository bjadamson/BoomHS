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

static int constexpr NUM_CUBES                     = 300;
static float constexpr SCREENSIZE_VIEWPORT_RATIO_X = 2.0f;

// clang-format off
static auto constexpr NEAR = 0.001f;
static auto constexpr FAR  = 1000.0f;
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
static bool MOUSE_ON_RHS_SCREEN = false;
static bool MOUSE_BUTTON_PRESSED = false;

auto&
active_camera_pos()
{
  return MOUSE_ON_RHS_SCREEN
    ? PERS_CAMERA_POS
    : ORTHO_CAMERA_POS;
}

namespace OR = opengl::render;

auto
make_program(common::Logger& logger)
{
  std::vector<opengl::AttributePointerInfo> const apis{{
    AttributePointerInfo{0, GL_FLOAT, AttributeType::POSITION, 3}
  }};

  auto va = make_vertex_attribute(apis);
  return make_shader_program(logger, "wireframe.vert", "wireframe.frag", MOVE(va))
    .expect_moveout("Error loading wireframe shader program");
}

auto
make_bbox(common::Logger& logger, ShaderProgram const& sp, Cube const& cr)
{
  auto const vertices = OF::cube_vertices(cr.min, cr.max);

  return gpu::copy_cube_wireframe_gpu(logger, vertices, sp.va());
}

auto
make_program_and_rectangle(common::Logger& logger, Rectangle const& rect,
                           Rectangle const& view_rect)
{
  auto const TOP_LEFT = glm::vec2{rect.left(), rect.top()};
  auto const BOTTOM_RIGHT = glm::vec2{rect.right(), rect.bottom()};

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

class CubeEntity
{
  Cube cube_;
  Transform tr_;
  DrawInfo di_;
public:
  COPYMOVE_DEFAULT(CubeEntity);

  CubeEntity(Cube&& cube, Transform&& tr, DrawInfo&& di)
      : cube_(MOVE(cube))
      , tr_(MOVE(tr))
      , di_(MOVE(di))
  {
  }

  bool moused_over = false;

  auto const& cube() const { return cube_; }
  auto& cube() { return cube_; }

  auto const& transform() const { return tr_; }
  auto& transform() { return tr_; }

  auto const& draw_info() const { return di_; }
  auto& draw_info() { return di_; }
};
using CubeEntities = std::vector<CubeEntity>;

void
draw_bboxes(common::Logger& logger, glm::mat4 const& pm, glm::mat4 const& vm,
            CubeEntities& cube_ents, ShaderProgram& sp,
            DrawState& ds)
{
  for (auto &cube_tr : cube_ents) {
    auto const& tr    = cube_tr.transform();
    auto const& wire_color = cube_tr.moused_over ? LOC::BLUE : LOC::RED;
    auto &dinfo = cube_tr.draw_info();
    draw_bbox(logger, pm, vm, tr, sp, dinfo, ds, wire_color);
  }
}

void
draw_rectangle_pm(common::Logger& logger, Rectangle const& viewport, CameraORTHO const& camera,
                  ShaderProgram& sp, DrawInfo& dinfo, Color const& color, DrawState& ds)
{
  int constexpr NEAR = -1.0;
  int constexpr FAR  = 1.0f;
  Frustum const f{
    viewport.left(),
    viewport.right(),
    viewport.bottom(),
    viewport.top(),
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
};

bool
process_keydown(SDL_Keycode const keycode, glm::vec3& camera_pos, CubeEntities& cube_ents)
{
  auto const move_trs = [&](auto const& delta_v) {
    for (auto& cube_tr : cube_ents) {
      auto& tr = cube_tr.transform();
      tr.translation += delta_v;
    }
  };
  switch (keycode)
  {
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
  return false;
}

struct PmRect
{
  Rectangle const& rect;
  bool is_mousedover = false;

  explicit PmRect(Rectangle const& r)
      : rect(r)
  {}
};

void
on_mouse_cube_collisions(glm::vec2 const& mouse_pos, glm::vec2 const& mouse_start,
                    Rectangle const& view_rect, glm::mat4 const& pm, glm::mat4 const& vm,
                    PmRect &pm_rect,
                    CubeEntities& cube_ents)
{
  pm_rect.is_mousedover = collision::point_rectangle_intersects(mouse_pos, pm_rect.rect);

  auto const& camera_pos  = active_camera_pos();
  glm::vec3 const ray_dir = Raycast::calculate_ray_into_screen(mouse_start, pm,
                                                                vm, view_rect);
  Ray const ray{camera_pos, ray_dir};
  for (auto &cube_tr : cube_ents) {
    auto const& cube = cube_tr.cube();

    auto tr          = cube_tr.transform();
    Cube cr{cube.min, cube.max};
    if (!MOUSE_ON_RHS_SCREEN) {
      tr.translation.y = 0.0f;
      cr.min.y = 0;
      cr.max.y = 0;
    }
    float distance = 0.0f;
    cube_tr.moused_over = collision::ray_cube_intersect(ray, tr, cr, distance);
  }
}

void
process_mousemotion(SDL_MouseMotionEvent const& motion,
                    ViewportDisplayInfo const& left_vdi, ViewportDisplayInfo const& right_vdi,
                    PmRect& pm_rect, CubeEntities& cube_ents)
{
  float const x = motion.x, y = motion.y;
  auto const mouse_pos = glm::vec2{x, y};

  auto const middle_point = left_vdi.view_rect.right();
  MOUSE_ON_RHS_SCREEN = mouse_pos.x > middle_point;

  glm::vec2 mouse_start;
  float distance = 0.0f;
  glm::mat4 const* pm        = nullptr;
  glm::mat4 const* vm        = nullptr;
  Rectangle const* view_rect = nullptr;
  if (MOUSE_ON_RHS_SCREEN) {
    // RHS
    view_rect   = &right_vdi.view_rect;
    mouse_start = mouse_pos - glm::vec2{middle_point, 0};

    pm = &right_vdi.perspective;
    vm = &right_vdi.view;
  }
  else {
    // LHS
    view_rect   = &left_vdi.view_rect;
    mouse_start = mouse_pos;

    pm = &left_vdi.perspective;
    vm = &left_vdi.view;
  }
  on_mouse_cube_collisions(mouse_pos, mouse_start, *view_rect, *pm, *vm, pm_rect, cube_ents);
}

bool
process_event(common::Logger& logger, SDL_Event& event, CameraORTHO& cam_ortho,
              ViewportDisplayInfo const& left_vdi, ViewportDisplayInfo const& right_vdi,
              CubeEntities& cube_ents, PmRect& pm_rect)
{
  bool const event_type_keydown = event.type == SDL_KEYDOWN;
  auto &camera_pos = active_camera_pos();

  if (event_type_keydown) {
    SDL_Keycode const key_pressed = event.key.keysym.sym;
    if (process_keydown(key_pressed, camera_pos, cube_ents)) {
      return true;
    }
  }
  else {
    if (event.type == SDL_MOUSEMOTION) {
      process_mousemotion(event.motion, left_vdi, right_vdi, pm_rect, cube_ents);
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN) {
      MOUSE_BUTTON_PRESSED = true;
      cam_ortho.click_position.x = event.button.x;
      cam_ortho.click_position.y = event.button.y;
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
      MOUSE_BUTTON_PRESSED = false;
    }
  }
  return event.type == SDL_QUIT;
}

auto
make_cube(RNG& rng)
{
  float constexpr MIN = 0, MAX = 200;
  static_assert(MIN < MAX, "MIN must be atleast one less than MAX");

  auto const gen = [&rng]() { return rng.gen_float_range(MIN + 1, MAX); };
  glm::vec3 const min{MIN, MIN, MIN};
  glm::vec3 const max{gen(), gen(), gen()};
  return Cube{min, max};
}

auto
gen_cube_entities(common::Logger& logger, ShaderProgram const& sp, RNG &rng)
{
  auto const gen = [&rng](auto const& l, auto const& h) { return rng.gen_float_range(l, h); };
  auto const gen_low_x = [&gen]() { return gen(0, WIDTH); };
  auto const gen_low_z = [&gen]() { return gen(0, HEIGHT); };
  auto const gen_tr = [&]() { return glm::vec3{gen_low_x(), 0, gen_low_z()}; };

  CubeEntities cube_ents;
  FOR(i, NUM_CUBES) {
    auto cube = make_cube(rng);
    auto tr = gen_tr();
    auto di = make_bbox(logger, sp, cube);
    CubeEntity pair{MOVE(cube), MOVE(tr), MOVE(di)};
    cube_ents.emplace_back(MOVE(pair));
  }
  return cube_ents;
}

void
draw_cursor_under_mouse(common::Logger& logger, Rectangle const& viewport, ShaderProgram& sp,
                        CameraORTHO const& cam_ortho, DrawState& ds)
{
  auto const& cp = cam_ortho.click_position;
  int x, y;
  SDL_GetMouseState(&x, &y);
  float const minx = cp.x * SCREENSIZE_VIEWPORT_RATIO_X;
  float const miny = cp.y;
  float const maxx = x * SCREENSIZE_VIEWPORT_RATIO_X;
  float const maxy = y;

  Rectangle const rect{minx, miny, maxx, maxy};
  OF::RectangleColors const RECT_C{
    std::nullopt,
    std::nullopt
  };
  OF::RectInfo const ri{rect, RECT_C, std::nullopt};
  auto const rbuffer = OF::make_rectangle(ri);
  auto di            = OG::copy_rectangle(logger, sp.va(), rbuffer);
  draw_rectangle_pm(logger, viewport, cam_ortho, sp, di, LOC::BLACK, ds);
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

  float const MIDDLE_HORIZ = SCREEN_DIM.right() / SCREENSIZE_VIEWPORT_RATIO_X;
  auto const LHS = ScreenDimensions{
    SCREEN_DIM.left(),
    SCREEN_DIM.top(),
    static_cast<int>(MIDDLE_HORIZ),
    SCREEN_DIM.bottom()
  };
  auto const RHS = ScreenDimensions{
    (int)MIDDLE_HORIZ,
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

  auto const lhs_rect    = LHS.rect();
  auto const rhs_rect    = RHS.rect();
  auto const screen_rect = SCREEN_DIM.rect();

  auto const color_rect = Rectangle{
    200, 200, 400, 400};
  auto rect_pair = make_program_and_rectangle(logger, color_rect, screen_rect);

  auto const pers_pm = glm::perspective(FOV, AR.compute(), FRUSTUM.near, FRUSTUM.far);

  Timer timer;
  FrameCounter fcounter;

  SDL_Event event;
  bool quit = false;

  auto wire_sp   = make_program(logger);
  RNG rng;
  auto cube_ents = gen_cube_entities(logger, wire_sp, rng);

  auto const draw_lhs = [&](DrawState& ds, auto const& pm, auto const& vm) {
    OR::set_viewport(LHS);
    OR::set_scissor(LHS);
    OR::clear_screen(LOC::WHITE);
    draw_bboxes(logger, pm, vm, cube_ents, wire_sp, ds);
  };
  auto const draw_rhs = [&](DrawState& ds, auto const& pm, auto const& vm) {
    OR::set_viewport(RHS);
    OR::set_scissor(RHS);
    OR::clear_screen(LOC::BLACK);
    draw_bboxes(logger, pm, vm, cube_ents, wire_sp, ds);
  };
  auto const draw_pm = [&](DrawState& ds, Color const& color) {
    OR::set_scissor(SCREEN_DIM);
    OR::set_viewport(SCREEN_DIM);
    draw_rectangle_pm(logger, screen_rect, cam_ortho, rect_pair.first, rect_pair.second, color, ds);
  };

  PmRect pm_rect{color_rect};
  while (!quit) {
    glm::mat4 const ortho_pm = cam_ortho.calc_pm(AR, FRUSTUM);
    glm::mat4 const ortho_vm = cam_ortho.calc_vm();

    glm::mat4 const pers_vm  = calculate_vm_fps(logger);

    auto const ft = FrameTime::from_timer(timer);
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      ViewportDisplayInfo const left_vdi{lhs_rect, ortho_pm, ortho_vm};
      ViewportDisplayInfo const right_vdi{rhs_rect, pers_pm,  pers_vm};
      quit = process_event(logger, event, cam_ortho, left_vdi, right_vdi, cube_ents, pm_rect);
    }

    DrawState ds;
    draw_lhs(ds, ortho_pm, ortho_vm);
    if (!MOUSE_ON_RHS_SCREEN && MOUSE_BUTTON_PRESSED) {
      auto& sp = rect_pair.first;
      OR::set_scissor(LHS);
      OR::set_viewport(LHS);
      draw_cursor_under_mouse(logger, lhs_rect, sp, cam_ortho, ds);
    }
    draw_rhs(ds, pers_pm, pers_vm);

    {
      auto const color = pm_rect.is_mousedover ? LOC::ORANGE : LOC::GREEN;
      draw_pm(ds, color);
    }

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());

    timer.update();
    fcounter.update();
  }
  return EXIT_SUCCESS;
}

#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/raycast.hpp>
#include <boomhs/random.hpp>

#include <common/algorithm.hpp>
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

static int constexpr NUM_CUBES                     = 100;
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

struct ProgramAndGpuHandle
{
  MOVE_CONSTRUCTIBLE_ONLY(ProgramAndGpuHandle);

  ShaderProgram sp;
  DrawInfo di;
};

auto
make_program_and_rect_gpuhandle(common::Logger& logger, Rectangle const& rect,
                           Rectangle const& view_rect)
{
  auto const TOP_LEFT = glm::vec2{rect.left, rect.top};
  auto const BOTTOM_RIGHT = glm::vec2{rect.right, rect.bottom};

  Rectangle      const ndc_rect{TOP_LEFT.x, TOP_LEFT.y, BOTTOM_RIGHT.x, BOTTOM_RIGHT.y};
  OF::RectInfo const ri{ndc_rect, std::nullopt, std::nullopt, std::nullopt};
  RectBuffer  buffer = OF::make_rectangle(ri);

  std::vector<opengl::AttributePointerInfo> const apis{{
    AttributePointerInfo{0, GL_FLOAT, AttributeType::POSITION, 3}
  }};

  auto va = make_vertex_attribute(apis);
  auto sp = make_shader_program(logger, "2dcolor.vert", "2dcolor.frag", MOVE(va))
    .expect_moveout("Error loading 2dcolor shader program");
  return ProgramAndGpuHandle{MOVE(sp), gpu::copy_rectangle(logger, sp.va(), buffer)};
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

  bool selected = false;

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
    auto const& wire_color = cube_tr.selected ? LOC::BLUE : LOC::RED;
    auto &dinfo = cube_tr.draw_info();
    draw_bbox(logger, pm, vm, tr, sp, dinfo, ds, wire_color);
  }
}

void
draw_rectangle_pm(common::Logger& logger, Rectangle const& viewport, CameraORTHO const& camera,
                  ShaderProgram& sp, DrawInfo& dinfo, Color const& color, GLenum const draw_mode,
                  DrawState& ds)
{
  int constexpr NEAR = -1.0;
  int constexpr FAR  = 1.0f;
  Frustum const f{
    viewport.left,
    viewport.right,
    viewport.bottom,
    viewport.top,
    NEAR,
    FAR
  };

  auto const pm = camera.calc_pm(AR, f);

  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  sp.set_uniform_matrix_4fv(logger, "u_projmatrix", pm);
  sp.set_uniform_color(logger, "u_color", color);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  OR::draw_2delements(logger, draw_mode, sp, dinfo.num_indices());
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
  bool selected = false;

  explicit PmRect(Rectangle const& r)
      : rect(r)
  {}
};

Rectangle
make_mouse_rect(CameraORTHO const& camera, glm::vec2 const& mouse_pos)
{
  auto const& click_pos  = camera.click_position;

  auto const lx = lesser_of(click_pos.x, mouse_pos.x);
  auto const rx = other_of_two(lx, PAIR(click_pos.x, mouse_pos.x));

  auto const ty = lesser_of(click_pos.y, mouse_pos.y);
  auto const by = other_of_two(ty, PAIR(click_pos.y, mouse_pos.y));

  return Rectangle{VEC2{lx, ty}, VEC2{rx, by}};
}

void
on_rhs_mouse_cube_collisions(common::Logger& logger, glm::vec2 const& mouse_pos,
                             glm::mat4 const& pm, glm::mat4 const& vm, Rectangle const& viewport,
                             CubeEntities& cube_ents)
{
  auto &camera_pos = active_camera_pos();
  Ray const ray{camera_pos, Raycast::calculate_ray_into_screen(mouse_pos, pm, vm, viewport)};
  for (auto &cube_ent : cube_ents) {
    auto const& cube = cube_ent.cube();
    auto tr          = cube_ent.transform();
    Cube cr{cube.min, cube.max};
    if (!MOUSE_ON_RHS_SCREEN) {
      tr.translation.y = 0.0f;
      cr.min.y = 0;
      cr.max.y = 0;
    }

    float distance = 0.0f;
    cube_ent.selected = collision::ray_cube_intersect(ray, tr, cr, distance);
  }
}

void
on_lhs_mouse_cube_collisions(common::Logger& logger,
                         glm::vec2 const& mouse_pos, glm::vec2 const& mouse_start,
                         Rectangle const& mouse_rect,
                         CameraORTHO const& cam_ortho, Rectangle const& view_rect,
                         glm::mat4 const& pm, glm::mat4 const& vm,
                         CubeEntities& cube_ents)
{
  for (auto &cube_ent : cube_ents) {
    auto const& cube = cube_ent.cube();
    auto tr          = cube_ent.transform();
    Cube cr{cube.min, cube.max};
    if (!MOUSE_ON_RHS_SCREEN) {
      tr.translation.y = 0.0f;
      cr.min.y = 0;
      cr.max.y = 0;
    }

    auto xz = cr.xz_rect();
    xz.left   = xz.left;
    xz.right  = xz.left + (xz.width() / SCREENSIZE_VIEWPORT_RATIO_X);

    xz.top    = xz.top;
    xz.bottom = xz.bottom;

    xz.left  += tr.translation.x / SCREENSIZE_VIEWPORT_RATIO_X;
    xz.right += tr.translation.x / SCREENSIZE_VIEWPORT_RATIO_X;

    xz.top    += tr.translation.z;
    xz.bottom += tr.translation.z;

    cube_ent.selected = collision::rectangles_overlap(mouse_rect, xz);
  }
}

void
process_mousemotion(common::Logger& logger, SDL_MouseMotionEvent const& motion,
                    CameraORTHO const& cam_ortho,
                    ViewportDisplayInfo const& left_vdi, ViewportDisplayInfo const& right_vdi,
                    PmRect& pm_rect, CubeEntities& cube_ents)
{
  float const x = motion.x, y = motion.y;
  auto const mouse_pos = glm::vec2{x, y};

  auto const& middle_point = left_vdi.view_rect.right;
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

  if (MOUSE_ON_RHS_SCREEN) {
    // RHS
    on_rhs_mouse_cube_collisions(logger, mouse_start, *pm, *vm, *view_rect, cube_ents);
  }
  else {
    if (MOUSE_BUTTON_PRESSED) {
      // LHS
      auto const mouse_rect = make_mouse_rect(cam_ortho, mouse_start);
      on_lhs_mouse_cube_collisions(logger, mouse_pos, mouse_start, mouse_rect, cam_ortho,
                                   *view_rect, *pm, *vm, cube_ents);
    }
    pm_rect.selected = collision::point_rectangle_intersects(mouse_start, pm_rect.rect);
  }
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
  else if (event.type == SDL_MOUSEWHEEL) {
    auto& wheel = event.wheel;
    if (wheel.y > 0) {
      cam_ortho.grow_view(glm::vec2{1.0f});
    }
    else {
      cam_ortho.shink_view(glm::vec2{1.0f});
    }
  }
  else if (event.type == SDL_MOUSEMOTION) {
    process_mousemotion(logger, event.motion, cam_ortho, left_vdi, right_vdi, pm_rect, cube_ents);
  }
  else if (event.type == SDL_MOUSEBUTTONDOWN) {
    MOUSE_BUTTON_PRESSED = true;
    cam_ortho.click_position.x = event.button.x;
    cam_ortho.click_position.y = event.button.y;
  }
  else if (event.type == SDL_MOUSEBUTTONUP) {
    MOUSE_BUTTON_PRESSED = false;

    for (auto& cube_ent : cube_ents) {
      cube_ent.selected = false;
    }
    pm_rect.selected = false;
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
                        Rectangle const& rect, CameraORTHO const& cam_ortho, DrawState& ds)
{
  auto const rbuffer = OF::make_line_rectangle(rect);
  auto di            = OG::copy_rectangle(logger, sp.va(), rbuffer);
  draw_rectangle_pm(logger, viewport, cam_ortho, sp, di, LOC::LIME_GREEN, GL_LINE_LOOP, ds);
}

void
draw_scene(common::Logger& logger,
           glm::mat4 const& ortho_pm, glm::mat4 const& ortho_vm,
           glm::mat4 const& pers_pm, glm::mat4 const& pers_vm,
           ScreenDimensions const& LHS, ScreenDimensions const& RHS,
           ScreenDimensions const& screen_dim, CameraORTHO const& cam_ortho,
           ProgramAndGpuHandle& pair, ShaderProgram& wire_sp, PmRect const& pm_rect,
           CubeEntities& cube_ents)
{
  auto& rect_sp        = pair.sp;
  auto& rect_gpuhandle = pair.di;

  DrawState ds;
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
    draw_rectangle_pm(logger, screen_dim.rect(), cam_ortho, rect_sp, rect_gpuhandle, color,
                      GL_TRIANGLES, ds);
  };

  draw_lhs(ds, ortho_pm, ortho_vm);
  draw_rhs(ds, pers_pm, pers_vm);

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    auto const& click_pos = cam_ortho.click_position;
    float const minx = click_pos.x;
    float const miny = click_pos.y;
    float const maxx = mouse_x;
    float const maxy = mouse_y;

    Rectangle mouse_rect{minx, miny, maxx, maxy};
    ScreenDimensions const* pview_port = nullptr;
    if (!MOUSE_ON_RHS_SCREEN && MOUSE_BUTTON_PRESSED) {
      mouse_rect.left *= SCREENSIZE_VIEWPORT_RATIO_X;
      mouse_rect.right *= SCREENSIZE_VIEWPORT_RATIO_X;
      pview_port = &LHS;

      assert(pview_port);
      OR::set_scissor(*pview_port);
      OR::set_viewport(*pview_port);
      draw_cursor_under_mouse(logger, pview_port->rect(), rect_sp, mouse_rect, cam_ortho, ds);
    }
    //else if (MOUSE_ON_RHS_SCREEN) {
      //p//view_port = &RHS;
      //draw_cursor_under_mouse(logger, pview_port->rect(), rect_sp, mouse_rect, cam_ortho, ds);
    //}

  {
    auto const color = pm_rect.selected ? LOC::ORANGE : LOC::PURPLE;
    draw_pm(ds, color);
  }
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

  auto const color_rect = Rectangle{
    200, 200, 400, 400};
  auto rectprog = make_program_and_rect_gpuhandle(logger, color_rect, SCREEN_DIM.rect());

  Timer timer;
  FrameCounter fcounter;

  SDL_Event event;
  bool quit = false;

  auto wire_sp   = make_program(logger);
  RNG rng;
  auto cube_ents = gen_cube_entities(logger, wire_sp, rng);

  PmRect pm_rect{color_rect};
  auto const pers_pm = glm::perspective(FOV, AR.compute(), FRUSTUM.near, FRUSTUM.far);
  while (!quit) {
    glm::mat4 const ortho_pm = cam_ortho.calc_pm(AR, FRUSTUM);
    glm::mat4 const ortho_vm = cam_ortho.calc_vm();

    glm::mat4 const pers_vm  = calculate_vm_fps(logger);

    auto const ft = FrameTime::from_timer(timer);
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      ViewportDisplayInfo const left_vdi{LHS.rect(), ortho_pm, ortho_vm};
      ViewportDisplayInfo const right_vdi{RHS.rect(), pers_pm,  pers_vm};
      quit = process_event(logger, event, cam_ortho, left_vdi, right_vdi, cube_ents, pm_rect);
    }

    draw_scene(logger, ortho_pm, ortho_vm, pers_pm, pers_vm, LHS, RHS, SCREEN_DIM, cam_ortho,
               rectprog, wire_sp, pm_rect, cube_ents);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());

    timer.update();
    fcounter.update();
  }
  return EXIT_SUCCESS;
}

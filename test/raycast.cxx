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

static int constexpr NUM_CUBES                   = 100;

static int constexpr SCREENSIZE_VIEWPORT_RATIO_X = 2.0f;
static int constexpr SCREENSIZE_VIEWPORT_RATIO_Y = 1.0f;

// clang-format off
static int constexpr WIDTH  = 1024;
static int constexpr HEIGHT = 768;

static auto constexpr NEAR = 0.001f;
static auto constexpr FAR  = 1000.0f;
static auto constexpr FOV  = glm::radians(110.0f);
static auto constexpr AR   = AspectRatio{4.0f, 3.0f};
// clang-format on

static glm::vec3  ORTHO_CAMERA_POS = glm::vec3{0, 1, 0};
static glm::vec3  PERS_CAMERA_POS  = glm::vec3{0, 0, 1};
static bool MOUSE_ON_RHS_SCREEN         = false;
static bool MOUSE_BUTTON_PRESSED        = false;
static bool MIDDLE_MOUSE_BUTTON_PRESSED = false;

auto&
active_camera_pos()
{
  return MOUSE_ON_RHS_SCREEN
    ? PERS_CAMERA_POS
    : ORTHO_CAMERA_POS;
}

namespace OR = opengl::render;

auto
make_wireframe_program(common::Logger& logger)
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
make_perspective_rect_gpuhandle(common::Logger& logger, RectFloat const& rect,
                                VertexAttribute const& va)
{
  OF::RectInfo const ri{rect, std::nullopt, std::nullopt, std::nullopt};
  RectBuffer  buffer = OF::make_rectangle(ri);

  return gpu::copy_rectangle(logger, va, buffer);
}

auto
make_rectangle_program(common::Logger& logger)
{
  std::vector<opengl::AttributePointerInfo> const apis{{
    AttributePointerInfo{0, GL_FLOAT, AttributeType::POSITION, 3}
  }};

  auto va = make_vertex_attribute(apis);
  return make_shader_program(logger, "2dcolor.vert", "2dcolor.frag", MOVE(va))
    .expect_moveout("Error loading 2dcolor shader program");
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
draw_rectangle_pm(common::Logger& logger, RectFloat const& viewport, CameraORTHO const& camera,
                  ShaderProgram& sp, DrawInfo& dinfo, Color const& color, GLenum const draw_mode,
                  DrawState& ds)
{
  int constexpr NEAR_PM = -1.0;
  int constexpr FAR_PM  = 1.0f;
  Frustum const f{
    viewport.left,
    viewport.right,
    viewport.bottom,
    viewport.top,
    NEAR_PM,
    FAR_PM
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
  Viewport const viewport;
  glm::mat4 const pm, vm;

  auto mouse_offset() const { return viewport.rect().left_top(); }
};

struct Viewports
{
  ViewportDisplayInfo left, right;
  Viewport fullscreen;

  // clang-format off
  auto center()        const { return right.viewport.rect().center_left(); }

  auto center_left()   const { return left.viewport.rect().center_left(); }
  auto center_right()  const { return right.viewport.rect().center_right(); }

  auto center_top()    const { return right.viewport.rect().left_top(); }
  auto center_bottom() const { return right.viewport.rect().left_bottom(); }
  // clang-format on
};

void
move_cubes(glm::vec3 const& delta_v, CubeEntities& cube_ents)
{
  for (auto& cube_tr : cube_ents) {
    auto& tr = cube_tr.transform();
    tr.translation += delta_v;
  }
}

bool
process_keydown(SDL_Keycode const keycode, glm::vec3& camera_pos, CubeEntities& cube_ents)
{
  auto const move_trs = [&](auto const& delta_v) {
    move_cubes(delta_v, cube_ents);
  };
  auto const rotate_ents = [&](float const angle_degrees, glm::vec3 const& axis) {
    for (auto& cube_ent : cube_ents) {
      cube_ent.transform().rotate_degrees(angle_degrees, axis);
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

    case SDLK_KP_2:
        rotate_ents(1.0f, constants::Y_UNIT_VECTOR);
        break;
    case SDLK_KP_4:
        rotate_ents(1.0f, constants::X_UNIT_VECTOR);
        break;
    case SDLK_KP_6:
        rotate_ents(-1.0f, constants::Y_UNIT_VECTOR);
        break;
    case SDLK_KP_8:
        rotate_ents(-1.0f, constants::X_UNIT_VECTOR);
        break;
    case SDLK_KP_3:
        rotate_ents(1.0f, constants::Z_UNIT_VECTOR);
        break;
    case SDLK_KP_9:
        rotate_ents(-1.0f, constants::Z_UNIT_VECTOR);
        break;

    default:
      break;
  }
  return false;
}

struct PmRect
{
  RectFloat const rect;
  DrawInfo        di;

  bool selected = false;

  explicit PmRect(RectFloat const& r, DrawInfo &&d)
      : rect(r)
      , di(MOVE(d))
  {}
};

struct PmRects
{
  std::array<PmRect, 2> pms;

  INDEX_OPERATOR_FNS(pms);
  BEGIN_END_FORWARD_FNS(pms);
};

auto
make_mouse_rect(CameraORTHO const& camera, glm::ivec2 const& mouse_pos)
{
  auto const& click_pos = camera.mouse_click.left_right;

  auto const lx = lesser_of(click_pos.x, mouse_pos.x);
  auto const rx = other_of_two(lx, PAIR(click_pos.x, mouse_pos.x));

  auto const ty = lesser_of(click_pos.y, mouse_pos.y);
  auto const by = other_of_two(ty, PAIR(click_pos.y, mouse_pos.y));

  return RectFloat{VEC2{lx, ty}, VEC2{rx, by}};
}

void
on_rhs_mouse_cube_collisions(common::Logger& logger, glm::vec2 const& mouse_pos,
                             ViewportDisplayInfo const& vdi,
                             CubeEntities& cube_ents)
{
  auto const& pm        = vdi.pm;
  auto const& vm        = vdi.vm;
  auto const view_rect  = vdi.viewport.rect();

  auto &camera_pos = active_camera_pos();
  Ray const ray{camera_pos, Raycast::calculate_ray_into_screen(mouse_pos, pm, vm, view_rect)};

  for (auto &cube_ent : cube_ents) {
    auto const& cube = cube_ent.cube();
    auto const& tr   = cube_ent.transform();

    float distance = 0.0f;
    cube_ent.selected = collision::ray_intersects_cube(logger, ray, tr, cube, distance);
  }
}

void
on_lhs_mouse_cube_collisions(common::Logger& logger, CameraORTHO const& cam_ortho,
                         glm::ivec2 const& mouse_start, ViewportDisplayInfo const& vdi,
                         CubeEntities& cube_ents)
{
  auto const& pm        = vdi.pm;
  auto const& vm        = vdi.vm;
  auto const view_rect  = vdi.viewport.rect();
  auto const mouse_rect = make_mouse_rect(cam_ortho, mouse_start);

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
    xz.bottom = xz.top + (xz.height() / SCREENSIZE_VIEWPORT_RATIO_Y);

    xz.left  += tr.translation.x / SCREENSIZE_VIEWPORT_RATIO_X;
    xz.right += tr.translation.x / SCREENSIZE_VIEWPORT_RATIO_X;

    xz.top    += tr.translation.z / SCREENSIZE_VIEWPORT_RATIO_Y;
    xz.bottom += tr.translation.z / SCREENSIZE_VIEWPORT_RATIO_Y;

    cube_ent.selected = collision::rectangles_overlap(mouse_rect, xz);
  }
}

void
process_mousemotion(common::Logger& logger, SDL_MouseMotionEvent const& motion,
                    CameraORTHO const& cam_ortho,
                    Viewports const& viewports,
                    PmRects& pm_rects, CubeEntities& cube_ents)
{
  float const x = motion.x, y = motion.y;
  auto const mouse_pos = glm::ivec2{x, y};

  auto const& left  = viewports.left;
  auto const& right = viewports.right;

  auto const& middle_point = viewports.center().x;

  MOUSE_ON_RHS_SCREEN = mouse_pos.x > middle_point;

  ViewportDisplayInfo const* vdi = nullptr;
  if (MOUSE_ON_RHS_SCREEN) {
    vdi = &right;
  }
  else {
    vdi = &left;
  }
  auto const mouse_start = mouse_pos - vdi->mouse_offset();
  if (MOUSE_ON_RHS_SCREEN) {
    // RHS
    on_rhs_mouse_cube_collisions(logger, mouse_start, *vdi, cube_ents);
  }
  else {
    if (MOUSE_BUTTON_PRESSED) {
      // LHS
      on_lhs_mouse_cube_collisions(logger, cam_ortho, mouse_start, *vdi, cube_ents);
    }
  }

  for (auto& pm_rect : pm_rects) {
    pm_rect.selected = collision::point_rectangle_intersects(mouse_pos, pm_rect.rect);
  }
}

bool
process_event(common::Logger& logger, SDL_Event& event, CameraORTHO& cam_ortho,
              Viewports const& viewports, CubeEntities& cube_ents, PmRects& pm_rects)
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
    process_mousemotion(logger, event.motion, cam_ortho, viewports, pm_rects, cube_ents);
  }
  else if (event.type == SDL_MOUSEBUTTONDOWN) {
    auto const& mouse_button = event.button;
    if (mouse_button.button == SDL_BUTTON_MIDDLE) {
      MIDDLE_MOUSE_BUTTON_PRESSED = true;
      auto &middle_clickpos = cam_ortho.mouse_click.middle;
      middle_clickpos.x = mouse_button.x;
      middle_clickpos.y = mouse_button.y;
    }
    else {
      MOUSE_BUTTON_PRESSED = true;
      auto &leftright_clickpos = cam_ortho.mouse_click.left_right;
      leftright_clickpos.x = mouse_button.x;
      leftright_clickpos.y = mouse_button.y;
    }
  }
  else if (event.type == SDL_MOUSEBUTTONUP) {
    auto const& mouse_button = event.button;
    if (mouse_button.button == SDL_BUTTON_MIDDLE) {
      MIDDLE_MOUSE_BUTTON_PRESSED = false;
    }
    else {
      MOUSE_BUTTON_PRESSED = false;

      for (auto& cube_ent : cube_ents) {
        cube_ent.selected = false;
      }
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
draw_cursor_under_mouse(common::Logger& logger, RectFloat const& viewport, ShaderProgram& sp,
                        RectFloat const& rect, CameraORTHO const& cam_ortho, DrawState& ds)
{
  auto const rbuffer = OF::make_line_rectangle(rect);
  auto di            = OG::copy_rectangle(logger, sp.va(), rbuffer);
  draw_rectangle_pm(logger, viewport, cam_ortho, sp, di, LOC::LIME_GREEN, GL_LINE_LOOP, ds);
}

void
draw_mouserect(common::Logger& logger, CameraORTHO const& camera,
               glm::ivec2 const& mouse_pos, ShaderProgram& rect_sp,
               Viewport const& view_port, DrawState& ds)
{
  auto const& click_pos = camera.mouse_click.left_right;
  float const minx = click_pos.x;
  float const miny = click_pos.y;
  float const maxx = mouse_pos.x;
  float const maxy = mouse_pos.y;

  RectFloat mouse_rect{minx, miny, maxx, maxy};
  mouse_rect.left  *= SCREENSIZE_VIEWPORT_RATIO_X;
  mouse_rect.right *= SCREENSIZE_VIEWPORT_RATIO_X;

  mouse_rect.top    *= SCREENSIZE_VIEWPORT_RATIO_Y;
  mouse_rect.bottom *= SCREENSIZE_VIEWPORT_RATIO_Y;

  OR::set_viewport_and_scissor(view_port);
  draw_cursor_under_mouse(logger, view_port.rect_float(), rect_sp, mouse_rect, camera, ds);
}

struct PmDrawInfo
{
  PmRects&       rects;
  ShaderProgram& sp;
};

void
draw_scene(common::Logger& logger, Viewports const& viewports, PmDrawInfo& pm_info,
           CameraORTHO const& camera,
           ShaderProgram& wire_sp, glm::ivec2 const& mouse_pos,
           CubeEntities& cube_ents)
{
  auto const& LHS = viewports.left;
  auto const& RHS = viewports.right;
  auto& pm_sp = pm_info.sp;

  auto const draw_lhs = [&](DrawState& ds) {
    OR::set_viewport_and_scissor(LHS.viewport);
    OR::clear_screen(LOC::WHITE);
    draw_bboxes(logger, LHS.pm, LHS.vm, cube_ents, wire_sp, ds);

    if (MOUSE_BUTTON_PRESSED) {
      draw_mouserect(logger, camera, mouse_pos, pm_sp, LHS.viewport, ds);
    }
  };
  auto const draw_rhs = [&](DrawState& ds) {
    OR::set_viewport_and_scissor(RHS.viewport);
    OR::clear_screen(LOC::BLACK);
    draw_bboxes(logger, RHS.pm, RHS.vm, cube_ents, wire_sp, ds);
  };
  auto const draw_pm = [&](auto& sp, auto& di, DrawState& ds, Color const& color) {
    auto const& viewport = viewports.fullscreen;
    OR::set_viewport_and_scissor(viewport);
    draw_rectangle_pm(logger, viewport.rect_float(), camera, sp, di, color, GL_TRIANGLES, ds);
  };
  auto const draw_pms = [&](auto& ds) {
    for (auto& pm_rect : pm_info.rects) {
      auto const color = pm_rect.selected ? LOC::ORANGE : LOC::PURPLE;
      draw_pm(pm_sp, pm_rect.di, ds, color);
    }
  };

  DrawState ds;
  draw_lhs(ds);
  draw_rhs(ds);
  draw_pms(ds);
}

void
update(common::Logger& logger, CameraORTHO& camera, RectFloat const& left_viewport,
       RectFloat const& right_viewport, glm::ivec2 const& mouse_pos, CubeEntities& cube_ents,
       FrameTime const& ft)
{
  if (MIDDLE_MOUSE_BUTTON_PRESSED) {
    auto const& middle_clickpos = camera.mouse_click.middle;
    float const distance        = pythag_distance(middle_clickpos, mouse_pos);

    auto const& frustum = MOUSE_ON_RHS_SCREEN
        ? left_viewport
        : right_viewport;
    float const dx = (mouse_pos - middle_clickpos).x / frustum.width();
    float const dy = (mouse_pos - middle_clickpos).y / frustum.height();

    auto constexpr SCROLL_SPEED = 15.0f;
    auto const multiplier = SCROLL_SPEED * distance * ft.delta_millis();

    move_cubes(glm::vec3{dx, 0, dy} * multiplier, cube_ents);
  }
}

auto
get_mousepos()
{
  int mouse_x, mouse_y;
  SDL_GetMouseState(&mouse_x, &mouse_y);
  return glm::ivec2{mouse_x, mouse_y};
}

auto
get_viewports(common::Logger &logger, CameraORTHO const& camera, Frustum const& frustum,
              Viewport const& lhs, Viewport const& rhs, Viewport const& screen_viewport,
              glm::mat4 const& pers_pm)
{
  glm::mat4 const ortho_pm = camera.calc_pm(AR, frustum);
  glm::mat4 const ortho_vm = camera.calc_vm();

  glm::mat4 const pers_vm  = calculate_vm_fps(logger);

  ViewportDisplayInfo const left{lhs, ortho_pm, ortho_vm};
  ViewportDisplayInfo const right{rhs, pers_pm,  pers_vm};
  return Viewports{left, right, screen_viewport};
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
  ENABLE_SCISSOR_TEST_UNTIL_SCOPE_EXIT();

  Viewport constexpr SCREEN_VIEWPORT{glm::ivec2{0, 0}, WIDTH, HEIGHT};
  LOG_ERROR_SPRINTF("SV %s", SCREEN_VIEWPORT.rect().to_string());

  int const VIEWPORT_WIDTH  = SCREEN_VIEWPORT.width() / SCREENSIZE_VIEWPORT_RATIO_X;
  int const VIEWPORT_HEIGHT = SCREEN_VIEWPORT.height() / SCREENSIZE_VIEWPORT_RATIO_Y;

  auto const LHS = Viewport{
    PAIR(SCREEN_VIEWPORT.left(), SCREEN_VIEWPORT.top()),
    VIEWPORT_WIDTH,
    SCREEN_VIEWPORT.height()
  };
  auto const RHS = Viewport{
    PAIR(VIEWPORT_WIDTH, SCREEN_VIEWPORT.top()),
    VIEWPORT_WIDTH,
    SCREEN_VIEWPORT.height()
  };
  Frustum constexpr FRUSTUM{
    SCREEN_VIEWPORT.float_left(),
    SCREEN_VIEWPORT.float_right(),
    SCREEN_VIEWPORT.float_bottom(),
    SCREEN_VIEWPORT.float_top(),
    NEAR,
    FAR};

  auto const ORTHO_FORWARD = -constants::Y_UNIT_VECTOR;
  auto constexpr ORTHO_UP  =  constants::Z_UNIT_VECTOR;
  WorldOrientation const ORTHO_WO{ORTHO_FORWARD, ORTHO_UP};
  CameraORTHO cam_ortho{ORTHO_WO, glm::vec2{WIDTH, HEIGHT}};

  auto const cr0 = RectFloat{200, 200, 400, 400};
  auto const cr1 = RectFloat{600, 600, 800, 800};
  auto rect_sp = make_rectangle_program(logger);

  auto const& va = rect_sp.va();
  auto di0 = make_perspective_rect_gpuhandle(logger, cr0, va);
  auto di1 = make_perspective_rect_gpuhandle(logger, cr1, va);

  Timer timer;
  FrameCounter fcounter;

  SDL_Event event;
  bool quit = false;

  auto wire_sp   = make_wireframe_program(logger);
  RNG rng;
  auto cube_ents = gen_cube_entities(logger, wire_sp, rng);

  PmRects pm_rects{
    std::array<PmRect, 2>{{
      PmRect{cr0, MOVE(di0)},
      PmRect{cr1, MOVE(di1)}
    }}
  };

  auto const pers_pm = glm::perspective(FOV, AR.compute(), FRUSTUM.near, FRUSTUM.far);
  while (!quit) {
    auto const viewports = get_viewports(logger, cam_ortho, FRUSTUM, LHS, RHS, SCREEN_VIEWPORT, pers_pm);
    auto const ft = FrameTime::from_timer(timer);
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      quit = process_event(logger, event, cam_ortho, viewports, cube_ents, pm_rects);
    }

    auto const mouse_pos = get_mousepos();
    update(logger, cam_ortho, LHS.rect_float(), RHS.rect_float(), mouse_pos, cube_ents, ft);

    PmDrawInfo pm_info{pm_rects, rect_sp};
    draw_scene(logger, viewports, pm_info, cam_ortho, wire_sp, mouse_pos, cube_ents);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());

    timer.update();
    fcounter.update();
  }
  return EXIT_SUCCESS;
}

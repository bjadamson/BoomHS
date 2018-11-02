#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/raycast.hpp>
#include <boomhs/random.hpp>
#include <boomhs/vertex_factory.hpp>
#include <boomhs/viewport.hpp>

#include <common/algorithm.hpp>
#include <common/log.hpp>
#include <common/timer.hpp>
#include <common/type_macros.hpp>

#include <demo/demo.hpp>

#include <gl_sdl/common.hpp>

#include <opengl/bind.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>
#include <opengl/ui_renderer.hpp>

#include <cstdlib>

//
// A test application used to develop/maintain code regarding muliple viewports and mouse selection
// using different camera perspectives.
//
// This application was developed as a sandbox application for implementing the above features.
//
// Summary:
//          Splits the screen into multiple viewports, rendering the same scene of 3dimensional
//          cubes from multiple camera positions/types (orthographic/perspective).
//
//          + If the user clicks and holds the left mouse button down within a orthographic
//          viewports a hollow rectangle is drawn from the point where the user clicked the mouse
//          initially and where the cursor is currently. This is used to test mouse box selection.
//
//          + If the user moves the mouse over a cube within one of the perspective viewports the
//          cube all of the cubes the mouse is over change color. This is used to test mouse
//          raycasting.
//
//          + If the user uses the arrow keys the entities within the scene are translated
//          accordingly.
using namespace boomhs;
using namespace boomhs::math;
using namespace common;
using namespace demo;
using namespace gl_sdl;
using namespace opengl;

static int constexpr NUM_CUBES                   = 100;
static glm::ivec2 constexpr SCREENSIZE_VIEWPORT_SIZE{2.0f, 2.0};

// clang-format off
static auto constexpr NEAR = 0.001f;
static auto constexpr FAR  = 1000.0f;
static auto constexpr FOV  = glm::radians(110.0f);
static auto constexpr AR   = AspectRatio{4.0f, 3.0f};
static auto constexpr VS   = ViewSettings{AR, FOV};
// clang-format on

using CameraPosition = glm::vec3;
static auto CAMERA_POS_TOPDOWN   = CameraPosition{0,  1,  0};
static auto CAMERA_POS_BOTTOP    = CameraPosition{0, -1,  0};
static auto CAMERA_POS_INTOSCENE = CameraPosition{0,  0,  1};

struct ViewportInfo
{
  Viewport       viewport;
  ScreenSector   screen_sector;
  Camera         camera;
  CameraMatrices matrices = {};

  MOVE_DEFAULT_ONLY(ViewportInfo);
  auto mouse_offset() const { return viewport.rect().left_top(); }

  void update(ViewSettings const& vs, Frustum const& frustum, RectInt const& window_rect)
  {
    matrices = camera.calc_cm(vs, frustum, window_rect.size(), camera.position());
  }
};

// global state
static bool MOUSE_BUTTON_PRESSED        = false;
static bool MIDDLE_MOUSE_BUTTON_PRESSED = false;
static MouseCursorInfo MOUSE_INFO;

struct PmRect
{
  RectFloat       rect;
  DrawInfo        di;

  bool selected = false;

  explicit PmRect(RectFloat const& r, DrawInfo &&d)
      : rect(r)
      , di(MOVE(d))
  {}

  MOVE_DEFAULT_ONLY(PmRect);
};

struct ViewportPmRects
{
  std::vector<PmRect> rects;
  Viewport            viewport;

  MOVE_DEFAULT_ONLY(ViewportPmRects);
};

struct PmViewports
{
  std::vector<ViewportPmRects> viewports;

  MOVE_DEFAULT_ONLY(PmViewports);

  INDEX_OPERATOR_FNS(viewports);
  BEGIN_END_FORWARD_FNS(viewports);

  // TODO: DON'T HAVE TO WRITE MANUALLY.
  auto size() const { return viewports.size(); }
};

struct PmDrawInfo
{
  ViewportPmRects  vp_rects;
  ShaderProgram&   sp;
};

struct PmDrawInfos
{
  std::vector<PmDrawInfo> infos;
  DEFINE_VECTOR_LIKE_WRAPPER_FNS(infos);
};

struct ViewportGrid
{
  ScreenSize screen_size;
  glm::ivec2 viewport_size;

  std::array<ViewportInfo, 4> infos;

  ViewportGrid(ScreenSize const& ss, glm::ivec2 const& ratio, ViewportInfo&& lt, ViewportInfo&& rt,
               ViewportInfo&& lb, ViewportInfo&& rb)
      : screen_size(ss)
      , viewport_size(ratio)
      , infos(common::make_array<ViewportInfo>(MOVE(lt), MOVE(rt), MOVE(lb), MOVE(rb)))
  {
  }

  MOVE_DEFAULT_ONLY(ViewportGrid);
  DEFINE_ARRAY_LIKE_WRAPPER_FNS(infos);

  auto& left_top() { return infos[0]; }
  auto const& left_top() const { return infos[0]; }

  auto& right_top() { return infos[1]; }
  auto const& right_top() const { return infos[1]; }

  auto& left_bottom() { return infos[2]; }
  auto const& left_bottom() const { return infos[2]; }

  auto& right_bottom() { return infos[3]; }
  auto const& right_bottom() const { return infos[3]; }

#define SCREEN_SECTOR_TO_VI_IMPL                                                                   \
  auto const match      = [&ss](auto const& vi) { return vi.screen_sector == ss; };                \
  if (match(left_top())) {                                                                         \
    return left_top();                                                                             \
  }                                                                                                \
  else if (match(left_bottom())) {                                                                 \
    return left_bottom();                                                                          \
  }                                                                                                \
  else if (match(right_bottom())) {                                                                \
    return right_bottom();                                                                         \
  }                                                                                                \
  else if (match(right_top())) {                                                                   \
    return right_top();                                                                            \
  }                                                                                                \
                                                                                                   \
  /* If we get here, programming error.*/                                                          \
  std::abort();

  ViewportInfo const&
  screen_sector_to_vi(ScreenSector const ss) const
  {
    SCREEN_SECTOR_TO_VI_IMPL
  }

  ViewportInfo&
  screen_sector_to_vi(ScreenSector const ss)
  {
    SCREEN_SECTOR_TO_VI_IMPL
  }

#undef SCREEN_SECTOR_TO_VI_IMPL

  auto&
  active_camera()
  {
    return screen_sector_to_vi(MOUSE_INFO.sector).camera;
  }

  Viewport
  fullscreen_viewport() const
  {
    auto const lt = left_top().viewport.left_top();

    auto const w = screen_size.width;
    auto const h = screen_size.height;

    return Viewport{lt.x, lt.y, w, h};
  }

  auto
  get_random_ordered_infos(RNG& rng) const
  {
    auto constexpr NUM_VIEWPORTS = 4;
    auto const array_of_ints = rng.gen_int_array_range<NUM_VIEWPORTS>(0, NUM_VIEWPORTS - 1);

    auto const fn = [&](auto const i) { return &(*this)[array_of_ints[i]]; };
    return common::make_array_from_fn_forwarding_index<array_of_ints.size()>(fn);
  }

  // clang-format off
  auto center()        const { return left_top().viewport.rect().right_bottom(); }

  auto center_left()   const { return left_top().viewport.rect().left_bottom(); }
  auto center_right()  const { return right_top().viewport.rect().right_bottom(); }

  auto center_top()    const { return right_top().viewport.rect().left_top(); }
  auto center_bottom() const { return right_top().viewport.rect().left_bottom(); }
  // clang-format on

  void update(AspectRatio const& ar, Frustum const& frustum, RectInt const& window_rect)
  {
    for (auto& vi : infos) {
      vi.update(VS, frustum, window_rect);
    }
  }
};

ScreenSector
mouse_pos_to_screensector(ViewportGrid const& vp_grid, glm::ivec2 const& mouse_pos)
{
  auto const& middle_point   = vp_grid.center();
  bool const mouse_on_rhs    = mouse_pos.x > middle_point.x;
  bool const mouse_on_bottom = mouse_pos.y > middle_point.y;

  if (!mouse_on_rhs) {
    if (!mouse_on_bottom) {
      return ScreenSector::LEFT_TOP;
    }
    else if (mouse_on_bottom) {
      return ScreenSector::LEFT_BOTTOM;
    }
  }
  else {
    if (!mouse_on_bottom) {
      return ScreenSector::RIGHT_TOP;
    }
    else if (mouse_on_bottom) {
      return ScreenSector::RIGHT_BOTTOM;
    }
  }
  std::abort();
}

auto
screen_sector_to_float_rect(ScreenSector const ss, ViewportGrid const& vp_grid)
{
  auto const& vi = vp_grid.screen_sector_to_vi(ss);
  return vi.viewport.rect_float();
}

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
  auto const vertices = VertexFactory::build_cube(cr.min, cr.max);
  return OG::copy_cube_wireframe_gpu(logger, vertices, sp.va());
}

void
draw_bbox(common::Logger& logger, CameraMatrices const& cm, ShaderProgram& sp,
          Transform const& tr, DrawInfo& dinfo, Color const& color, DrawState& ds)
{
  auto const model_matrix = tr.model_matrix();

  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  shader::set_uniform(logger, sp, "u_wirecolor", color);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  auto const camera_matrix = cm.proj * cm.view;
  render::set_mvpmatrix(logger, camera_matrix, model_matrix, sp);
  render::draw(logger, ds, GL_LINES, sp, dinfo);
}

class CubeEntity
{
  Cube cube_;
  Transform tr_;
  DrawInfo di_;
public:
  MOVE_DEFAULT_ONLY(CubeEntity);

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
draw_bboxes(common::Logger& logger, CameraMatrices const& cm,
            CubeEntities& cube_ents, ShaderProgram& sp,
            DrawState& ds)
{
  for (auto &cube_tr : cube_ents) {
    auto const& tr      = cube_tr.transform();
    auto &dinfo         = cube_tr.draw_info();
    bool const selected = cube_tr.selected;

    auto const& wire_color = selected ? LOC4::BLUE : LOC4::RED;
    draw_bbox(logger, cm, sp, tr, dinfo, wire_color, ds);
  }
}

void
move_cubes(glm::vec3 const& delta_v, CubeEntities& cube_ents)
{
  for (auto& cube_tr : cube_ents) {
    auto& tr = cube_tr.transform();
    tr.translation += delta_v;
  }
}

bool
process_keydown(common::Logger& logger, SDL_Keycode const keycode, CameraPosition& camera_pos,
                CubeEntities& cube_ents)
{
  auto const move_trs = [&](auto const& delta_v) {
    move_cubes(delta_v, cube_ents);
  };
  auto const rotate_ents = [&](float const angle_degrees, glm::vec3 const& axis) {
    for (auto& cube_ent : cube_ents) {
      auto& tr = cube_ent.transform();
      tr.rotate_degrees(angle_degrees, axis);
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
    case SDLK_KP_8:
        rotate_ents(-1.0f, constants::Y_UNIT_VECTOR);
        break;

    case SDLK_KP_4:
        rotate_ents(1.0f, constants::X_UNIT_VECTOR);
        break;
    case SDLK_KP_6:
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

void
cast_rays_through_cubes_into_screen(common::Logger& logger, glm::vec2 const& mouse_pos,
                                    ViewportInfo const& vi, CubeEntities& cube_ents)
{
  auto const view_rect = vi.viewport.rect();
  auto const& cm       = vi.matrices;
  auto const& pm       = cm.proj;
  auto const& vm       = cm.view;

  auto const& camera_pos = vi.camera.position();
  auto const dir = Raycast::calculate_ray_into_screen(mouse_pos, pm, vm, view_rect);
  Ray const ray{camera_pos, dir};
  for (auto &cube_ent : cube_ents) {
    auto const& cube = cube_ent.cube();
    auto const& tr   = cube_ent.transform();

    float distance = 0.0f;
    cube_ent.selected = collision::intersects(logger, ray, tr, cube, distance);
  }
}

void
select_cubes_under_user_drawn_rect(common::Logger& logger, RectFloat const& mouse_rect,
                                   glm::ivec2 const& vpgrid_size, CubeEntities& cube_ents,
                                   ModelViewMatrix const& mv)
{
  // Determine whether a cube projected onto the XZ plane and another rectangle overlap.
  auto const cube_mouserect_overlap = [&](auto const& cube_entity) {
    auto const& cube = cube_entity.cube();
    auto tr          = cube_entity.transform();
    Cube cr{cube.min, cube.max};

    // Take the Cube in Object space, and create a rectangle from the x/z coordinates.
    auto xz = cr.xz_rect();
    xz = space_conversions::screen_to_viewport(xz, vpgrid_size);

    // Translate the rectangle from Object space to world space.
    auto const xm = tr.translation.x / vpgrid_size.x;
    auto const zm = tr.translation.z / vpgrid_size.y;
    xz.move(xm, zm);

    // Combine the transform and rectangle into a single structure.
    RectTransform const rect_tr{xz, tr};

    // Compute whether the rectangle (converted from the cube) and the rectangle from the user
    // clicking and dragging are overlapping.
    return collision::overlap(mouse_rect, rect_tr, mv);
  };

  for (auto &ce : cube_ents) {
    ce.selected = cube_mouserect_overlap(ce);
  }
}

void
process_mousemotion(common::Logger& logger, SDL_MouseMotionEvent const& motion,
                    ViewportGrid const& vp_grid, PmDrawInfos& pmdis,
                    CubeEntities& cube_ents)
{
  auto const mouse_pos   = glm::ivec2{motion.x, motion.y};
  MOUSE_INFO.sector      = mouse_pos_to_screensector(vp_grid, mouse_pos);
  auto const& vi         = vp_grid.screen_sector_to_vi(MOUSE_INFO.sector);
  auto const& camera     = vi.camera;
  auto const camera_mode = camera.mode();

  switch (camera_mode) {
    case CameraMode::ThirdPerson:
    case CameraMode::FPS:
    {
      auto const mouse_start = mouse_pos - vi.mouse_offset();
      cast_rays_through_cubes_into_screen(logger, mouse_start, vi, cube_ents);
    } break;

    case CameraMode::Ortho:
    {
      if (MOUSE_BUTTON_PRESSED) {
        auto const& click_pos_ss = MOUSE_INFO.click_positions.left_right;
        auto const mouse_rect    = MouseRectangleRenderer::make_mouse_rect(click_pos_ss, mouse_pos,
                                                               vi.mouse_offset());
        auto const& cm     = vi.matrices;
        auto const vp_size = vp_grid.viewport_size;
        select_cubes_under_user_drawn_rect(logger, mouse_rect, vp_size, cube_ents, cm.proj);
      }
    } break;

    case CameraMode::Fullscreen_2DUI:
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      std::abort();
  }

  for (auto& pm_vp : pmdis.infos) {
    for (auto& pm_rect : pm_vp.vp_rects.rects) {
      pm_rect.selected = collision::intersects(mouse_pos, pm_rect.rect);
    }
  }
}

bool
process_event(common::Logger& logger, SDL_Event& event, ViewportGrid& vp_grid,
              CubeEntities& cube_ents, PmDrawInfos& pmdis, FrameTime const& ft)
{
  bool const event_type_keydown = event.type == SDL_KEYDOWN;
  auto &camera = vp_grid.active_camera();

  if (event_type_keydown) {
    SDL_Keycode const key_pressed = event.key.keysym.sym;
    if (process_keydown(logger, key_pressed, camera.ortho.position, cube_ents)) {
      return true;
    }
  }
  else if (event.type == SDL_MOUSEWHEEL) {
    auto& wheel = event.wheel;

    auto const& fn = (wheel.y > 0)
      ? &Camera::zoom_out
      : &Camera::zoom_in;

    (camera.*fn)(1.0f, ft);
  }
  else if (event.type == SDL_MOUSEMOTION) {
    process_mousemotion(logger, event.motion, vp_grid, pmdis, cube_ents);
  }
  else if (event.type == SDL_MOUSEBUTTONDOWN) {
    auto const& mouse_button = event.button;
    if (mouse_button.button == SDL_BUTTON_MIDDLE) {
      MIDDLE_MOUSE_BUTTON_PRESSED = true;
      auto &middle_clickpos = MOUSE_INFO.click_positions.middle;
      middle_clickpos.x = mouse_button.x;
      middle_clickpos.y = mouse_button.y;
    }
    else {
      MOUSE_BUTTON_PRESSED = true;
      auto &leftright_clickpos = MOUSE_INFO.click_positions.left_right;
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
  float constexpr MIN = -100, MAX = 100;
  static_assert(MIN < MAX, "MIN must be atleast one less than MAX");

  auto const gen = [&rng]() { return rng.gen_float_range(MIN + 1, MAX); };
  glm::vec3 const min{MIN, MIN, MIN};
  glm::vec3 const max{gen(), gen(), gen()};

  return Cube{min, max};
}

auto
gen_cube_entities(common::Logger& logger, ScreenSize const& ss, ShaderProgram const& sp, RNG &rng)
{
  auto const gen = [&rng](auto const& l, auto const& h) { return rng.gen_float_range(l, h); };
  auto const gen_low_x = [&gen, &ss]() { return gen(0, ss.width); };
  auto const gen_low_z = [&gen, &ss]() { return gen(0, ss.height); };
  auto const gen_tr = [&]() { return glm::vec3{gen_low_x(), 0, gen_low_z()}; };

  CubeEntities cube_ents;
  FOR(i, NUM_CUBES) {
    auto cube = make_cube(rng);
    auto tr = gen_tr();
    auto di = make_bbox(logger, sp, cube);
    cube_ents.emplace_back(MOVE(cube), MOVE(tr), MOVE(di));
  }
  return cube_ents;
}

void
draw_scene(common::Logger& logger, ViewportGrid const& vp_grid, PmDrawInfos& pm_infos,
           Frustum const& frustum, ShaderProgram& wire_sp, ShaderProgram& pm_sp,
           glm::ivec2 const& mouse_pos, CubeEntities& cube_ents)
{
  auto const screen_size   = vp_grid.screen_size;
  auto const screen_height = screen_size.height;

  auto const draw_viewport = [&](auto& vi, DrawState& ds) {
    auto const& viewport = vi.viewport;
    OR::set_viewport_and_scissor(viewport, screen_height);
    OR::clear_screen(viewport.bg_color());
    auto const& cm = vi.matrices;
    draw_bboxes(logger, cm, cube_ents, wire_sp, ds);
  };
  auto const draw_pms = [&](auto& ds, auto& pm_infos) {
    for (auto& pm_info : pm_infos) {
      auto& vi             = pm_info.vp_rects;
      auto const& viewport = vi.viewport;
      OR::set_viewport_and_scissor(viewport, screen_height);
      for (auto& pm_rect : vi.rects) {
        auto const color = pm_rect.selected ? LOC4::ORANGE : LOC4::PURPLE;
        auto ui_renderer = UiRenderer::create(logger, viewport, AR);
        ui_renderer.draw_rect(logger, pm_rect.di, color, GL_TRIANGLES, ds);
      }
    }
  };

  auto const& vi_mousein = vp_grid.screen_sector_to_vi(MOUSE_INFO.sector);
  auto const figureout_draw_with_mouserect = [&](auto const& vi) {
    bool const is_ortho = CameraMode::Ortho == vi.camera.mode();
    bool const same_grid_as_mouse = (&vi_mousein == &vi);
    return is_ortho && same_grid_as_mouse && MOUSE_BUTTON_PRESSED;
  };

  DrawState ds;
  for (auto const& vi : vp_grid) {
    draw_viewport(vi, ds);

    if (figureout_draw_with_mouserect(vi)) {
      auto const& pos_init = MOUSE_INFO.click_positions.left_right;
      MouseRectangleRenderer::draw_mouseselect_rect(logger, pos_init, mouse_pos, LOC4::LIME_GREEN,
                                                    vi.viewport, AR, ds);
    }
  }

  // draw PMS
  draw_pms(ds, pm_infos);
}

void
update(common::Logger& logger, ViewportGrid& vp_grid, glm::ivec2 const& mouse_pos,
       CubeEntities& cube_ents, Frustum const& frustum, RectInt const& window_rect,
       FrameTime const& ft)
{
  auto const rect = screen_sector_to_float_rect(MOUSE_INFO.sector, vp_grid);

  if (MIDDLE_MOUSE_BUTTON_PRESSED) {
    auto const& middle_clickpos = MOUSE_INFO.click_positions.middle;
    float const distance        = pythag_distance(middle_clickpos, mouse_pos);

    float const dx = (mouse_pos - middle_clickpos).x / rect.width();
    float const dy = (mouse_pos - middle_clickpos).y / rect.height();

    auto constexpr SCROLL_SPEED = 25.0f;
    auto const multiplier = SCROLL_SPEED * distance * ft.delta_millis();

    move_cubes(glm::vec3{dx, 0, dy} * multiplier, cube_ents);
  }

  vp_grid.update(AR, frustum, window_rect);
}

auto
create_viewport_grid(common::Logger &logger, RectInt const& window_rect)
{
  int const viewport_width  = window_rect.width() / SCREENSIZE_VIEWPORT_SIZE.x;
  int const viewport_height = window_rect.height() / SCREENSIZE_VIEWPORT_SIZE.y;

  auto const lhs_top = Viewport{
    PAIR(window_rect.left, window_rect.top),
    viewport_width,
    viewport_height,
    LOC4::WHITE
  };
  auto const rhs_top = Viewport{
    PAIR(viewport_width, window_rect.top),
    viewport_width,
    viewport_height,
    LOC4::GREEN
  };

  auto const mid_height = window_rect.top + window_rect.half_height();
  auto const lhs_bottom = Viewport{
    PAIR(window_rect.left, mid_height),
    viewport_width,
    viewport_height,
    LOC4::LIGHT_SKY_BLUE
  };
  auto const rhs_bottom = Viewport{
    PAIR(viewport_width, mid_height),
    viewport_width,
    viewport_height,
    LOC4::LIGHT_GOLDENROD_YELLOW
  };

  // pers => perspective
  auto const     PERS_MINUSZ_FORWARD = -constants::Z_UNIT_VECTOR;
  auto constexpr PERS_MINUSZ_UP      =  constants::Y_UNIT_VECTOR;
  auto const wo_pers_minusz          = WorldOrientation{PERS_MINUSZ_FORWARD, PERS_MINUSZ_UP};

  auto const     PERS_PLUSZ_FORWARD = constants::Z_UNIT_VECTOR;
  auto constexpr PERS_PLUSZ_UP      = constants::Y_UNIT_VECTOR;
  auto const wo_pers_plusz          = WorldOrientation{PERS_MINUSZ_FORWARD, PERS_MINUSZ_UP};

  auto const     ORTHO_TOPDOWN_FORWARD = -constants::Y_UNIT_VECTOR;
  auto constexpr ORTHO_TOPDOWN_UP      =  constants::Z_UNIT_VECTOR;
  auto const wo_ortho_td               = WorldOrientation{ORTHO_TOPDOWN_FORWARD, ORTHO_TOPDOWN_UP};

  auto const ORTHO_BOTTOMTOP_FORWARD   =  constants::Y_UNIT_VECTOR;
  auto const ORTHO_BOTTOMTOP_UP        =  constants::Z_UNIT_VECTOR;
  auto const wo_ortho_bu               = WorldOrientation{ORTHO_BOTTOMTOP_FORWARD, ORTHO_BOTTOMTOP_UP};

  // TODO: Understand the following better.
  //
  // The top-down camera should flip the right unit vector.
  // The bottom-up camera does NOT flip the right unit vector.
  auto ortho_td = Camera::make_default(CameraMode::Ortho, wo_pers_minusz, wo_ortho_td);
  ortho_td.ortho.position = CAMERA_POS_TOPDOWN;
  ortho_td.ortho.flip_rightv = true;

  auto ortho_bu = Camera::make_default(CameraMode::Ortho, wo_pers_minusz, wo_ortho_bu);
  ortho_bu.ortho.position = CAMERA_POS_BOTTOP;

  auto const tps_fwd = Camera::make_default(CameraMode::ThirdPerson, wo_pers_minusz, wo_ortho_td);
  auto const tps_bkwd = Camera::make_default(CameraMode::ThirdPerson, wo_pers_plusz, wo_ortho_td);

  auto const fps_fwd = Camera::make_default(CameraMode::FPS, wo_pers_minusz, wo_ortho_td);
  auto const fps_bkwd = Camera::make_default(CameraMode::FPS, wo_pers_plusz, wo_ortho_td);

  auto const pick_camera = [&](RNG& rng) {
    int const val = rng.gen_int_range(0, 5);
    switch (val) {
      case 0:
        return ortho_td.clone();
      case 1:
        return ortho_bu.clone();
      case 2:
        return tps_fwd.clone();
      case 3:
        return tps_bkwd.clone();
      case 4:
        return fps_fwd.clone();
      case 5:
        return fps_bkwd.clone();
    }
    std::abort();
  };

  RNG rng;
  ViewportInfo left_top {lhs_top,    ScreenSector::LEFT_TOP,     pick_camera(rng)};
  ViewportInfo right_bot{rhs_bottom, ScreenSector::RIGHT_BOTTOM, pick_camera(rng)};

  ViewportInfo right_top{rhs_top,    ScreenSector::RIGHT_TOP,    pick_camera(rng)};
  ViewportInfo left_bot {lhs_bottom, ScreenSector::LEFT_BOTTOM,  pick_camera(rng)};

  auto const ss = window_rect.size();
  return ViewportGrid{ss, SCREENSIZE_VIEWPORT_SIZE, MOVE(left_top), MOVE(right_top), MOVE(left_bot), MOVE(right_bot)};
}

auto
make_pminfos(common::Logger& logger, ShaderProgram& sp, RNG &rng, ViewportGrid const& vp_grid)
{
  auto const& va = sp.va();
  auto const make_viewportpm_rects = [&](auto const& r, auto const& viewport) {
    std::vector<PmRect> vector_pms;

    auto di = demo::make_perspective_rect_gpuhandle(logger, r, va);
    vector_pms.emplace_back(r, MOVE(di));
    return ViewportPmRects{MOVE(vector_pms), viewport};
  };

  auto const vis_randomized = vp_grid.get_random_ordered_infos(rng);
  std::vector<ViewportPmRects> pms;
  for (auto const& vi : vis_randomized) {
    auto const& viewport = vi->viewport;
    auto const prect     = demo::make_perspective_rect(viewport, rng);

    auto pm0 = make_viewportpm_rects(prect, viewport);
    pms.emplace_back(MOVE(pm0));
  }
  PmViewports pm_vps{MOVE(pms)};

  std::vector<PmDrawInfo> pm_infos_vec;
  FOR(i, pm_vps.size()) {
    PmDrawInfo info{MOVE(pm_vps[i]), sp};
    pm_infos_vec.emplace_back(MOVE(info));
  }
  return PmDrawInfos{MOVE(pm_infos_vec)};
}

int
main(int argc, char **argv)
{
  char const* TITLE = "Multiple Viewport Raycast";
  bool constexpr FULLSCREEN = false;

  auto logger = common::LogFactory::make_stderr();
  auto const on_error = [&logger](auto const& error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  auto gl_sdl = TRY_OR(GlSdl::make_default(logger, TITLE, FULLSCREEN, 1024, 768), on_error);

  OR::init(logger);
  ENABLE_SCISSOR_TEST_UNTIL_SCOPE_EXIT();

  auto& window           = gl_sdl.window;
  auto const window_rect = window.view_rect();
  auto const frustum     = Frustum::from_rect_and_nearfar(window_rect, NEAR, FAR);

  EntityRegistry registry;
  auto const eid = registry.create();

  auto const     INTOSCENE_FORWARD = -constants::Z_UNIT_VECTOR;
  auto constexpr INTOSCENE_UP      =  constants::Y_UNIT_VECTOR;
  WorldOrientation const wo_intoscene{INTOSCENE_FORWARD,  INTOSCENE_UP};
  WorldObject EMPTY_WO{eid, registry, wo_intoscene};
  registry.get<Transform>(eid).translation = CAMERA_POS_INTOSCENE;

  auto vp_grid = create_viewport_grid(logger, window_rect);
  for (auto& vi : vp_grid) {
    vi.camera.set_target(EMPTY_WO);
  }

  RNG rng;
  auto color2d_program = static_shaders::BasicMvWithUniformColor::create(logger);
  auto& rect_sp = color2d_program.sp();

  auto pm_infos  = make_pminfos(logger, rect_sp, rng, vp_grid);
  auto wire_sp   = make_wireframe_program(logger);
  auto cube_ents = gen_cube_entities(logger, window_rect.size(), wire_sp, rng);

  FrameCounter fcounter;
  SDL_Event event;
  bool quit = false;

  Timer timer;
  while (!quit) {
    auto const ft = FrameTime::from_timer(timer);
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      quit = process_event(logger, event, vp_grid, cube_ents, pm_infos, ft);
    }

    auto const mouse_pos = gl_sdl::mouse_coords();
    update(logger, vp_grid, mouse_pos, cube_ents, frustum, window_rect, ft);
    draw_scene(logger, vp_grid, pm_infos, frustum, wire_sp, rect_sp, mouse_pos, cube_ents);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());

    timer.update();
    fcounter.update();
  }
  return EXIT_SUCCESS;
}

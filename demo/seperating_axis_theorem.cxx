#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/color.hpp>
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

#include <demo/common.hpp>

#include <gl_sdl/sdl_window.hpp>
#include <gl_sdl/common.hpp>

#include <opengl/bind.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>
#include <opengl/ui_renderer.hpp>

#include <cstdlib>

//
// A test application used to develop/check the math for collision detection between different
// polygon's using the seperating axis theorem.
//
// Summary:
//      Renders a number of rectangles onto the screen. Depending on whether the rectangle is
//      overlapping another rectangle, it's color will be different.
//
//      Overlapping rectangles are one color, rectangles which overlap with no other rectangle are
//      a different color.
//
//      + If the user clicks and holds the left mouse button down within a orthographic
//        viewports a hollow rectangle is drawn from the point where the user clicked the mouse
//        initially and where the cursor is currently. Any rectangles within this user-defined
//        rectangle have their color set to a random color, for the duration of the user holding
//        down the button.
//
// User Input:
//      The following table indicates how the user may interact with the demo.
//
//      No key-modier will result in the action being applied to a single rectangle.
//
//      Holding the left-shift modifier while pressing any of the keys in the following table
//      will cause the function to be applied to ALL entities in the scene.
//
//        + [w,a,s,d]           : translate
//        + [NUMPAD_2, NUMPAD_8]: rotate
//        + [e, q]              : scale
using namespace boomhs;
using namespace boomhs::math;
using namespace common;
using namespace demo;
using namespace gl_sdl;
using namespace opengl;

static int constexpr NUM_CUBES = 1;
static int constexpr NUM_RECTS = 1;

static int constexpr WIDTH     = 1024;
static int constexpr HEIGHT    = 768;

static auto constexpr VIEWPORT          = Viewport{0, 0, WIDTH, HEIGHT};
static auto const     VPGRID_DIMENSIONS = glm::ivec2{1, 1};

static int constexpr NEAR      = -1.0;
static int constexpr FAR       = 1.0f;

// global state
static bool MOUSE_BUTTON_PRESSED        = false;
static bool MIDDLE_MOUSE_BUTTON_PRESSED = false;
static MouseCursorInfo MOUSE_INFO;

static Color CLICK_COLOR;

struct PmRect
{
  RectFloat rect;
  DrawInfo  di;
  Transform transform;
  Color     color;

  bool mouse_selected = false;

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
  ShaderProgram*      sp;

  MOVE_DEFAULT_ONLY(ViewportPmRects);
};

bool
process_keydown(common::Logger& logger, SDL_Keycode const keycode, ViewportPmRects& vp_rects,
                Transform& controlled_tr, CubeEntities& cube_ents)
{
  uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
  assert(keystate);
  bool const shift_down = keystate[SDL_SCANCODE_LSHIFT];

  auto const iter_rects = [&](auto const& fn) {
    if (shift_down) {
      for (auto& rect : vp_rects.rects) {
        fn(rect.transform);
      }
      for (auto& cube : cube_ents) {
        fn(cube.transform());
      }
    }
    else {
      fn(controlled_tr);
    }
  };

  auto const rotate_ents = [&](float const angle_degrees, glm::vec3 const& axis) {
    iter_rects([&](auto& tr) { tr.rotate_degrees(angle_degrees, axis); });
  };
  auto const transform_ents = [&](glm::vec3 const& delta) {
    iter_rects([&](auto& tr) { tr.translation += 4 * delta; });
  };
  auto const scale_ents = [&](glm::vec3 const& delta) {
    iter_rects([&](auto& tr) { tr.scale += delta; });
  };
  switch (keycode)
  {
    case SDLK_F10:
    case SDLK_ESCAPE:
      return true;
      break;

    // translation
    case SDLK_d:
      transform_ents(+constants::X_UNIT_VECTOR);
      break;
    case SDLK_a:
      transform_ents(-constants::X_UNIT_VECTOR);
      break;
    case SDLK_w:
      transform_ents(-constants::Y_UNIT_VECTOR);
      break;
    case SDLK_s:
      transform_ents(+constants::Y_UNIT_VECTOR);
      break;

    // scaling
    case SDLK_e:
      scale_ents(+VEC3(0.2));
      break;
    case SDLK_q:
      scale_ents(-VEC3(0.2));
      break;

    // rotation
    case SDLK_KP_2:
      rotate_ents(-1.0f, constants::Z_UNIT_VECTOR);
      break;
    case SDLK_KP_8:
      rotate_ents(+1.0f, constants::Z_UNIT_VECTOR);
      break;
    default:
      break;
  }
  return false;
}

void
update(common::Logger& logger, ViewportPmRects& vp_rects, CubeEntities& cube_ents,
       ProjMatrix const& proj, ViewMatrix const& view)
{
  assert(!vp_rects.rects.empty());
  auto& a = vp_rects.rects.front();

  for (auto& a : vp_rects.rects) {
    bool overlap = false;
    for (auto& b : vp_rects.rects) {
      if (overlap || &a == &b) continue;

      RectTransform const ra{a.rect, a.transform};
      RectTransform const rb{b.rect, b.transform};

      overlap |= collision::overlap(ra, rb, proj, view);
    }

    a.color = a.mouse_selected ? CLICK_COLOR
      : (overlap ? LOC4::RED : LOC4::LIGHT_SEAGREEN);
  }
}

void
select_pmrects_under_user_drawn_rect(common::Logger& logger, RectFloat const& mouse_rect,
                                     std::vector<PmRect>& rects, ProjMatrix const& proj,
                                     ViewMatrix const& view)
{
  for (auto& rect : rects) {
    RectTransform const rect_tr{rect.rect, rect.transform};
    rect.mouse_selected = collision::overlap(mouse_rect, rect_tr, proj, view);
  }
}

void
process_mousemotion(common::Logger& logger, SDL_MouseMotionEvent const& motion,
    std::vector<PmRect>& rects, CubeEntities& cube_ents, ProjMatrix const& proj,
    ViewMatrix const& view)
{
  if (MOUSE_BUTTON_PRESSED) {
    auto const mouse_pos   = glm::ivec2{motion.x, motion.y};
    auto const& click_pos_ss = MOUSE_INFO.click_positions.left_right;

    auto const mouse_rect = MouseRectangleRenderer::make_mouse_rect(click_pos_ss, mouse_pos,
                                                                    VIEWPORT.left_top());

    select_pmrects_under_user_drawn_rect(logger, mouse_rect, rects, proj, view);

    demo::select_cubes_under_user_drawn_rect(logger, mouse_rect, VPGRID_DIMENSIONS, cube_ents, proj,
                                             view, VIEWPORT);
  }
}

bool
process_event(common::Logger& logger, SDL_Event& event, ViewportPmRects& vp_rects,
              CubeEntities& cube_ents, ProjMatrix const& proj, ViewMatrix const& view,
              Transform& controlled_tr, FrameTime const& ft)
{
  bool const event_type_keydown = event.type == SDL_KEYDOWN;

  if (event_type_keydown) {
    SDL_Keycode const key_pressed = event.key.keysym.sym;
    if (process_keydown(logger, key_pressed, vp_rects, controlled_tr, cube_ents)) {
      return true;
    }
  }
  else if (event.type == SDL_MOUSEMOTION) {
    process_mousemotion(logger, event.motion, vp_rects.rects, cube_ents, proj, view);
  }
  else if (event.type == SDL_MOUSEBUTTONDOWN) {
    auto const& mouse_button = event.button;
    CLICK_COLOR = color::random();

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

      for (auto& rect : vp_rects.rects) {
        rect.mouse_selected = false;
      }
    }
  }
  return event.type == SDL_QUIT;
}

auto
make_rects(common::Logger& logger, int const num_rects, Viewport const& viewport, ShaderProgram& sp,
    RNG &rng)
{
  auto const& va = sp.va();
  auto const make_viewportpm_rects = [&](auto const& r, auto const& viewport) {
    std::vector<PmRect> vector_pms;

    FORI(i, num_rects) {
      auto di = demo::make_perspective_rect_gpuhandle(logger, r, va);
      vector_pms.emplace_back(PmRect{r, MOVE(di)});
    }

    return ViewportPmRects{MOVE(vector_pms), viewport, &sp};
  };

  auto constexpr prect = RectFloat{-50, -50, 50, 50};
  auto vppm_rects  = make_viewportpm_rects(prect, viewport);

  {
    auto const& MIN = constants::ZERO;
    auto const max = VEC3{WIDTH, HEIGHT, 0};
    for (auto& rect : vppm_rects.rects) {
      rect.transform.translation = glm::vec3{100, 100, 0};//rng.gen_3dposition(MIN, max);
      rect.transform.rotate_degrees(5.0f, constants::Z_UNIT_VECTOR);
    }
  }

  return vppm_rects;
}

void
draw_scene(common::Logger& logger, UiRenderer& ui_renderer, ViewportPmRects& vp_rects,
           CubeEntities& cube_ents, ShaderProgram& wire_sp, CameraMatrices const& cm, DrawState& ds)
{
  auto const screen_height = VIEWPORT.height();

  auto const& viewport = vp_rects.viewport;
  OR::set_viewport_and_scissor(viewport, screen_height);

  for (auto& pm_rect : vp_rects.rects) {
    auto& di         = pm_rect.di;
    auto const model = pm_rect.transform.model_matrix();
    ui_renderer.draw_rect(logger, model, di, pm_rect.color, GL_TRIANGLES, ds);
  }

  if (MOUSE_BUTTON_PRESSED) {
    auto const mouse_pos = gl_sdl::mouse_coords();
    auto const& pos_init = MOUSE_INFO.click_positions.left_right;
    MouseRectangleRenderer::draw_mouseselect_rect(logger, pos_init, mouse_pos, LOC4::PURPLE,
                                                  viewport, ds);
  }

  demo::draw_bboxes(logger, cm, cube_ents, wire_sp, ds);
};

int
main(int argc, char **argv)
{
  char const* TITLE = "Seperating Axis Theorem";
  bool constexpr FULLSCREEN = false;

  auto logger = common::LogFactory::make_stderr();
  auto const on_error = [&logger](auto const& error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  auto gl_sdl = TRY_OR(GlSdl::make_default(logger, TITLE, FULLSCREEN, WIDTH, HEIGHT), on_error);
  OR::init(logger);
  auto& window           = gl_sdl.window;
  auto const window_rect = window.view_rect();
  auto const frustum     = Frustum::from_rect_and_nearfar(window_rect, NEAR, FAR);

  auto color2d_program = static_shaders::BasicMvWithUniformColor::create(logger);
  auto& rect_sp = color2d_program.sp();

  RNG rng;
  auto vp_rects = make_rects(logger, NUM_RECTS, VIEWPORT, rect_sp, rng);
  assert(!vp_rects.rects.empty());

  bool constexpr IS_2D = true;
  auto cube_ents = demo::gen_cube_entities(logger, NUM_CUBES, window_rect.size(), rect_sp, rng,
                                           IS_2D);
  auto wire_sp   = demo::make_wireframe_program(logger);

  ProjMatrix const proj = CameraORTHO::compute_pm(frustum, VIEWPORT.size(), constants::ZERO,
                                                  glm::ivec2{0});
  ViewMatrix const view{};

  auto ui_renderer = UiRenderer::create(logger, VIEWPORT);

  FrameCounter fcounter;
  SDL_Event event;
  bool quit = false;

  Timer timer;
  while (!quit) {
    auto const ft = FrameTime::from_timer(timer);
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      auto& controlled_tr = vp_rects.rects.back().transform;
      quit = process_event(logger, event, vp_rects, cube_ents, proj, view, controlled_tr, ft);
    }

    update(logger, vp_rects, cube_ents, proj, view);

    DrawState ds;
    OR::clear_screen(LOC4::DEEP_SKY_BLUE);

    CameraMatrices const cm{proj, view};
    draw_scene(logger, ui_renderer, vp_rects, cube_ents, wire_sp, cm, ds);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());

    timer.update();
    fcounter.update();
  }
  return EXIT_SUCCESS;
}

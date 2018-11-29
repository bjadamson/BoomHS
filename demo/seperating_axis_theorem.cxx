#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/color.hpp>
#include <boomhs/components.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/math.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/raycast.hpp>
#include <boomhs/rectangle.hpp>
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

static int constexpr NUM_CUBES = 2;
static int constexpr NUM_RECTS = 1;

static int constexpr WIDTH     = 1024;
static int constexpr HEIGHT    = 768;

static auto constexpr VIEWPORT = Viewport{0, 0, WIDTH, HEIGHT};

static int constexpr NEAR      = -500;
static int constexpr FAR       = +500;

// global state
static bool MOUSE_BUTTON_PRESSED        = false;
static bool MIDDLE_MOUSE_BUTTON_PRESSED = false;
static MouseCursorInfo MOUSE_INFO;

static Color CLICK_COLOR;

// (camera related)
namespace WO           = opengl::world_orientation;
static auto CAMERA_POS = constants::ZERO;

static auto CAMERA_VIEW_FWD = WO::FORWARDZ_FORWARD;
static auto CAMERA_VIEW_UP  = WO::FORWARDZ_UP;

struct PmRect
{
  RectFloat   rect;
  DrawInfo    di;
  Transform2D transform;
  Color       color;

  bool mouse_selected = false;

  explicit PmRect(RectFloat const& r, DrawInfo &&d)
      : rect(r)
      , di(MOVE(d))
  {}

  MOVE_DEFAULT_ONLY(PmRect);
};

struct PmRects
{
  std::vector<PmRect> rects;
  ShaderProgram*      sp;

  MOVE_DEFAULT_ONLY(PmRects);
};

template <typename T, typename F>
void
something_3d2d(T& transform, F const& fn, glm::vec3 const& delta)
{
  auto& tr = transform;

  bool constexpr IS_3D     = std::is_same_v<T, Transform>;
  bool constexpr IS_2D     = std::is_same_v<T, Transform2D>;
  static_assert(IS_3D != IS_2D, "Cannot be both 2D AND 3D.");

  if constexpr (IS_2D) {
    // When transforming a 2D entity by a 3d vector, drop the Z component.
    auto const d2 = glm::vec2{delta.x, delta.y};
    fn(tr, d2);
  }
  else {
    fn(tr, delta);
  }
}

bool
process_keydown(common::Logger& logger, SDL_Keycode const keycode, PmRects& vp_rects,
                PmRect& controlled_rect, CubeEntities& cube_ents, CubeEntity& controlled_cube)
{
  auto constexpr SCALE_AMOUNT = 0.2;

  uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
  assert(keystate);
  bool const shift_down = keystate[SDL_SCANCODE_LSHIFT];

  auto const iter_rects = [&](auto const& fn, auto&&... args)
  {
    if (shift_down) {
      for (auto& rect : vp_rects.rects) {
        fn(rect.transform, FORWARD(args));
      }
      for (auto& cube : cube_ents) {
        fn(cube.transform(), FORWARD(args));
      }
    }
    else {
      fn(controlled_rect.transform, FORWARD(args));
      fn(controlled_cube.transform(), FORWARD(args));
    }
  };

  auto const rotate_ents = [&](float const angle_degrees, glm::vec3 const& axis) {
    iter_rects([&](auto& tr) { tr.rotate_degrees(angle_degrees, axis); });
  };


  auto const invoke_on_entities = [&iter_rects](auto const& fn, auto&&... args) {
    auto const lifted = [&](auto& tr) { something_3d2d(tr, fn, FORWARD(args)); };
    iter_rects(lifted);
  };

  auto const transform_ents = [&](glm::vec3 const& delta) {
    auto const add_fn = [](auto& tr, auto const& d) { tr.translation += 4 * d; };
    invoke_on_entities(add_fn, delta);
  };

  auto const scale_ents = [&](glm::vec3 const& delta) {
    auto const scale_fn = [](auto& tr, auto const& d) { tr.scale += d; };
    invoke_on_entities(scale_fn, delta);
  };
  auto const on_nonquit_fkey_press = [&](char const* name, auto const& pos, auto const& wo) {
    CAMERA_POS = constants::ZERO;

    CAMERA_VIEW_FWD = wo.forward;
    CAMERA_VIEW_UP  = wo.up;

    auto const pos_s = glm::to_string(CAMERA_POS);
    auto const fwd_s = glm::to_string(CAMERA_VIEW_FWD);
    auto const up_s  = glm::to_string(CAMERA_VIEW_UP);
    LOG_INFO_SPRINTF("Camera %s: %s %s %s", name, pos_s, fwd_s, up_s);
  };
  switch (keycode)
  {
    case SDLK_F10:
    case SDLK_ESCAPE:
      return true;
      break;

    case SDLK_F1:
      on_nonquit_fkey_press("FORWARDZ", constants::ZERO, WO::FORWARDZ);
      break;
    case SDLK_F2:
      on_nonquit_fkey_press("REVERSEZ", constants::ZERO, WO::REVERSEZ);
      break;
    case SDLK_F3:
      on_nonquit_fkey_press("FORWARDX", constants::ZERO, WO::FORWARDX);
      break;
    case SDLK_F4:
      on_nonquit_fkey_press("REVERSEX", constants::ZERO, WO::REVERSEX);
      break;

    case SDLK_F5:
      on_nonquit_fkey_press("TOPDOWN", VEC3{0, 0, 0}, WO::TOPDOWN);
      break;
    case SDLK_F6:
      on_nonquit_fkey_press("BOTTOMUP", VEC3{0, 0, 0}, WO::BOTTOMUP);
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

    case SDLK_e:
      transform_ents(+constants::Z_UNIT_VECTOR);
      break;
    case SDLK_q:
      transform_ents(-constants::Z_UNIT_VECTOR);
      break;

    // UNIFORM scaling
    case SDLK_1:
      scale_ents(+VEC3(SCALE_AMOUNT));
      break;
    case SDLK_2:
      scale_ents(-VEC3(SCALE_AMOUNT));
      break;

    // NON-uniform scaling
    case SDLK_3:
      scale_ents(+VEC3(SCALE_AMOUNT, 0, 0));
      break;
    case SDLK_4:
      scale_ents(-VEC3(SCALE_AMOUNT, 0, 0));
      break;
    case SDLK_5:
      scale_ents(+VEC3(0, SCALE_AMOUNT, 0));
      break;
    case SDLK_6:
      scale_ents(-VEC3(0, SCALE_AMOUNT, 0));
      break;
    case SDLK_7:
      scale_ents(+VEC3(0, 0, SCALE_AMOUNT));
      break;
    case SDLK_8:
      scale_ents(-VEC3(0, 0, SCALE_AMOUNT));
      break;

    // rotation
    case SDLK_KP_2:
      rotate_ents(-1.0f, constants::Z_UNIT_VECTOR);
      break;
    case SDLK_KP_8:
      rotate_ents(+1.0f, constants::Z_UNIT_VECTOR);
      break;

    case SDLK_KP_4:
      rotate_ents(-1.0f, constants::Y_UNIT_VECTOR);
      break;
    case SDLK_KP_6:
      rotate_ents(+1.0f, constants::Y_UNIT_VECTOR);
      break;

    case SDLK_KP_1:
      rotate_ents(-1.0f, constants::X_UNIT_VECTOR);
      break;
    case SDLK_KP_3:
      rotate_ents(+1.0f, constants::X_UNIT_VECTOR);
      break;

    default:
      break;
  }
  return false;
}

void
update(common::Logger& logger, PmRects& vp_rects, CubeEntities& cube_ents)
{
  assert(!vp_rects.rects.empty());
  auto& a = vp_rects.rects.front();

  for (auto& a : vp_rects.rects) {
    bool overlap = false;
    for (auto& b : vp_rects.rects) {
      if (overlap || &a == &b) continue;

      RectTransform const ra{a.rect, a.transform};
      RectTransform const rb{b.rect, b.transform};

      overlap |= collision::overlap(ra, rb);
    }

    a.color = a.mouse_selected ? CLICK_COLOR
      : (overlap ? LOC4::RED : LOC4::LIGHT_SEAGREEN);
  }

  auto const ce_to_obb = [](CubeEntity const& ce) {
    auto const& c = ce.cube();
    auto const& t = ce.transform();
    return OBB::from_cube_transform(c, t);
  };

  for (auto& b : cube_ents)
  {
    bool overlap = false;
    for (auto& a : cube_ents) {
      if (overlap || &a == &b) continue;

      auto const obb0 = ce_to_obb(a);
      auto const obb1 = ce_to_obb(b);

      bool const obb_overlap = collision::overlap(obb0, obb1);
      if (obb_overlap) {
        a.overlap_color = LOC4::PINK;
        b.overlap_color = LOC4::PINK;
      }
      else {
        a.overlap_color = {};
        b.overlap_color = {};
      }
    }
  }
}

void
select_pmrects_under_user_drawn_rect(common::Logger& logger, RectFloat const& mouse_rect,
                                     std::vector<PmRect>& rects, ProjMatrix const& proj,
                                     ViewMatrix const& view)
{
  for (auto& rect : rects) {
    RectTransform const rect_tr{rect.rect, rect.transform};

    auto const origin = VIEWPORT.left_top();
    bool constexpr IS_2D = true;
    rect.mouse_selected = collision::overlap(mouse_rect, rect_tr, VIEWPORT, IS_2D);
  }
}

void
on_mouse_press(common::Logger& logger, SDL_MouseMotionEvent const& motion,
    std::vector<PmRect>& rects, CubeEntities& cube_ents, ProjMatrix const& proj,
    ViewMatrix const& view)
{
  auto const mouse_pos     = glm::ivec2{motion.x, motion.y};
  auto const& click_pos_ss = MOUSE_INFO.click_positions.left_right;
  auto const mouse_rect    = MouseRectangleRenderer::make_mouse_rect(click_pos_ss, mouse_pos,
                                                                  VIEWPORT.left_top());
  // 2d
  select_pmrects_under_user_drawn_rect(logger, mouse_rect, rects, proj, view);

  // 3d
  // 1. Convert viewport coordinates to world space.
  auto const mouse_lt = mouse_rect.left_top();
  auto const mouse_rb = mouse_rect.right_bottom();

  auto const viewport = VIEWPORT.rect_float();
  auto const left_top_world_pos = math::space_conversions::screen_to_world(mouse_lt, NEAR, proj, view, viewport);
  auto const right_bottom_world_pos = math::space_conversions::screen_to_world(mouse_rb, NEAR, proj, view, viewport);

  bool constexpr IS_2D = false;
  demo::select_cubes_under_user_drawn_rect(logger, mouse_rect, cube_ents, proj,
                                             view, VIEWPORT, IS_2D);
}

template <typename ...Args>
void
process_mousemotion(Args&&... args)
{
  if (MOUSE_BUTTON_PRESSED) {
    on_mouse_press(FORWARD(args));
  }
}

bool
process_event(common::Logger& logger, SDL_Event& event, PmRects& vp_rects,
              CubeEntities& cube_ents, ProjMatrix const& proj, ViewMatrix const& view,
              PmRect& controlled_rect, CubeEntity& controlled_cube, FrameTime const& ft)
{
  bool const event_type_keydown = event.type == SDL_KEYDOWN;

  if (event_type_keydown) {
    SDL_Keycode const key_pressed = event.key.keysym.sym;
    if (process_keydown(logger, key_pressed, vp_rects, controlled_rect, cube_ents,
                        controlled_cube)) {
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

      for (auto& cube : cube_ents) {
        cube.selected = false;
      }
    }
  }
  return event.type == SDL_QUIT;
}

auto
make_rects(common::Logger& logger, int const num_rects, ShaderProgram& sp, RNG &rng)
{
  auto const& va = sp.va();
  auto const pmake_pmrects = [&](auto const& r) {
    std::vector<PmRect> vector_pms;

    FORI(i, num_rects) {
      auto di = demo::make_perspective_rect_gpuhandle(logger, r, va);
      vector_pms.emplace_back(PmRect{r, MOVE(di)});
    }

    return PmRects{MOVE(vector_pms), &sp};
  };

  auto constexpr MIN = -80;
  auto constexpr MAX = +80;

  auto const left_top     = rng.gen_float_range(MIN, 0);
  auto const right_bottom = rng.gen_float_range(0, MAX);

  auto const prect = RectFloat{
    left_top,
    left_top,
    right_bottom,
    right_bottom
  };
  auto vppm_rects  = pmake_pmrects(prect);

  {
    auto const MIN = glm::vec3{VIEWPORT.left(), VIEWPORT.top(), 0};;
    auto const max = MIN + VEC3{VIEWPORT.width(), VIEWPORT.height(), 0};
    for (auto& rect : vppm_rects.rects) {
      rect.transform.translation = rng.gen_3dposition(MIN, max);
    }
  }

  return vppm_rects;
}

void
draw_scene(common::Logger& logger, UiRenderer& ui_renderer, PmRects& vp_rects,
           CubeEntities& cube_ents, ShaderProgram& wire_sp, CameraMatrices const& cm, DrawState& ds)
{
  OR::set_viewport_and_scissor(VIEWPORT, HEIGHT);

  for (auto& pm_rect : vp_rects.rects) {
    auto& di         = pm_rect.di;
    auto const model = pm_rect.transform.model_matrix();
    ui_renderer.draw_rect(logger, model, di, pm_rect.color, GL_TRIANGLES, ds);
  }

  if (MOUSE_BUTTON_PRESSED) {
    auto const mouse_pos = gl_sdl::mouse_coords();
    auto const& pos_init = MOUSE_INFO.click_positions.left_right;
    MouseRectangleRenderer::draw_mouseselect_rect(logger, pos_init, mouse_pos, LOC4::PURPLE,
                                                  VIEWPORT, ds);
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

  TRY_OR(auto gl_sdl, GlSdl::make_default(logger, TITLE, FULLSCREEN, WIDTH, HEIGHT), on_error);
  OR::init(logger);
  auto& window           = gl_sdl.window;
  auto const window_rect = window.view_rect();
  auto const frustum     = Frustum::from_rect_and_nearfar(window_rect, NEAR, FAR);

  auto color2d_program = static_shaders::BasicMvWithUniformColor::create(logger);
  auto& rect_sp = color2d_program.sp();

  RNG rng;
  auto vp_rects = make_rects(logger, NUM_RECTS, rect_sp, rng);
  assert(!vp_rects.rects.empty());

  bool constexpr IS_2D = true;
  auto cube_ents = demo::gen_cube_entities(logger, NUM_CUBES, window_rect, rect_sp, rng, IS_2D);
  auto wire_sp   = demo::make_wireframe_program(logger);

  auto constexpr ZOOM   = glm::ivec2{0};
  ProjMatrix const proj = CameraORTHO::compute_pm(frustum, VIEWPORT.size(), constants::ZERO, ZOOM);

  auto ui_renderer = UiRenderer::create(logger, VIEWPORT);

  FrameCounter fcounter;
  SDL_Event event;
  bool quit = false;

  Timer timer;
  while (!quit) {
    auto const ft = FrameTime::from_timer(timer);
    ViewMatrix const view  = glm::lookAtRH(CAMERA_POS, CAMERA_VIEW_FWD, CAMERA_VIEW_UP);



    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      auto& controlled_2drect = vp_rects.rects.back();
      auto& controlled_3dcube = cube_ents.back();
      quit = process_event(logger, event, vp_rects, cube_ents, proj, view, controlled_2drect,
                           controlled_3dcube, ft);
    }

    update(logger, vp_rects, cube_ents);

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

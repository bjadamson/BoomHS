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

#include <gl_sdl/common.hpp>

#include <opengl/bind.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>

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
using namespace gl_sdl;
using namespace opengl;

static auto constexpr FOV = glm::radians(110.0f);
static auto constexpr AR  = AspectRatio{4.0f, 3.0f};
static int constexpr NEAR = -1.0;
static int constexpr FAR  = 1.0f;

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

struct CameraMatrices
{
  glm::mat4 pm, vm;

  MOVE_DEFAULT_ONLY(CameraMatrices);
};

auto
make_perspective_rect_gpuhandle(common::Logger& logger, RectFloat const& rect,
                                VertexAttribute const& va)
{
  RectBuffer buffer = RectBuilder{rect}.build();
  return OG::copy_rectangle(logger, va, buffer);
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

void
draw_rectangle_pm(common::Logger& logger, ScreenSize const& ss, Frustum const& frustum,
                  ShaderProgram& sp, DrawInfo& dinfo, Color const& color, GLenum const draw_mode,
                  DrawState& ds)
{
  auto constexpr ZOOM = glm::ivec2{0};
  auto const pm = CameraORTHO::compute_pm(AR, frustum, ss, constants::ZERO, ZOOM);

  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  sp.set_uniform_mat4(logger,  "u_projmatrix", pm);
  sp.set_uniform_color(logger, "u_color", color);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  OR::draw_2delements(logger, draw_mode, sp, dinfo.num_indices());
}

bool
process_keydown(common::Logger& logger, SDL_Keycode const keycode)
{
  auto const rotate_ents = [&](float const angle_degrees, glm::vec3 const& axis) {
    //for (auto& cube_ent : cube_ents) {
      //auto& tr = cube_ent.transform();
      //tr.rotate_degrees(angle_degrees, axis);
    //}
  };
  switch (keycode)
  {
    case SDLK_F10:
    case SDLK_ESCAPE:
      return true;
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
process_mousemotion(common::Logger& logger, SDL_MouseMotionEvent const& motion,
                    PmDrawInfo& pmdi)
{
  auto const mouse_pos   = glm::ivec2{motion.x, motion.y};

  for (auto& pm_rect : pmdi.vp_rects.rects) {
    pm_rect.selected = collision::intersects(mouse_pos, pm_rect.rect);
  }
}

bool
process_event(common::Logger& logger, SDL_Event& event, PmDrawInfo& pmdi, FrameTime const& ft)
{
  bool const event_type_keydown = event.type == SDL_KEYDOWN;

  if (event_type_keydown) {
    SDL_Keycode const key_pressed = event.key.keysym.sym;
    if (process_keydown(logger, key_pressed)) {
      return true;
    }
  }
  else if (event.type == SDL_MOUSEMOTION) {
    process_mousemotion(logger, event.motion, pmdi);
  }
  return event.type == SDL_QUIT;
}

auto
make_pminfo(common::Logger& logger, Viewport const& viewport, ShaderProgram& sp, RNG &rng)
{
  auto const make_perspective_rect = [&](Viewport const& viewport, glm::ivec2 const& offset) {
    auto const gen = [&rng](float const val) { return rng.gen_float_range(val, val + 350); };

    auto const center = viewport.center();
    auto const left   = viewport.left()  + gen(offset.x);
    auto const right  = viewport.right() - gen(offset.x);

    auto const top    = viewport.top()    + gen(offset.y);
    auto const bottom = viewport.bottom() - gen(offset.y);
    return RectFloat{left, top, right, bottom};
  };

  auto const& va = sp.va();
  auto const make_viewportpm_rects = [&](auto const& r, auto const& viewport) {
    std::vector<PmRect> vector_pms;

    auto di = make_perspective_rect_gpuhandle(logger, r, va);
    vector_pms.emplace_back(PmRect{r, MOVE(di)});
    return ViewportPmRects{MOVE(vector_pms), viewport};
  };

  auto const prect     = make_perspective_rect(viewport, IVEC2{50});
  auto pm0 = make_viewportpm_rects(prect, viewport);
  return PmDrawInfo{MOVE(pm0), sp};
}

auto
get_mousepos()
{
  int mouse_x, mouse_y;
  SDL_GetMouseState(&mouse_x, &mouse_y);
  return glm::ivec2{mouse_x, mouse_y};
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

  int constexpr WIDTH = 1024;
  int constexpr HEIGHT = 768;
  auto constexpr VIEWPORT = Viewport{0, 0, WIDTH, HEIGHT};

  auto gl_sdl = TRY_OR(GlSdl::make_default(logger, TITLE, FULLSCREEN, WIDTH, HEIGHT), on_error);

  OR::init(logger);
  ENABLE_SCISSOR_TEST_UNTIL_SCOPE_EXIT();

  auto& window           = gl_sdl.window;
  auto const window_rect = window.view_rect();
  auto const frustum     = Frustum::from_rect_and_nearfar(window_rect, NEAR, FAR);

  RNG rng;
  auto rect_sp            = make_rectangle_program(logger);

  auto pm_info            = make_pminfo(logger, VIEWPORT, rect_sp, rng);
  glm::mat4 const pers_pm = glm::perspective(FOV, AR.compute(), frustum.near, frustum.far);

  FrameCounter fcounter;
  SDL_Event event;
  bool quit = false;

  Timer timer;
  while (!quit) {
    auto const ft = FrameTime::from_timer(timer);
    while ((!quit) && (0 != SDL_PollEvent(&event))) {
      quit = process_event(logger, event, pm_info, ft);
    }

    auto const mouse_pos = get_mousepos();

    auto const screen_size = VIEWPORT.size();
    auto const screen_height = VIEWPORT.height();
    auto const draw_pm = [&](auto& ds, auto& pm_info) {
      auto& vi             = pm_info.vp_rects;
      auto const& viewport = vi.viewport;
      OR::set_viewport_and_scissor(viewport, screen_height);
      for (auto& pm_rect : vi.rects) {
        auto const color = pm_rect.selected ? LOC::ORANGE : LOC::PURPLE;

        auto const pm = CameraORTHO::compute_pm(AR, frustum, screen_size, constants::ZERO, glm::ivec2{0});
        draw_rectangle_pm(logger, viewport.size(), frustum, pm_info.sp, pm_rect.di, color,
                         GL_TRIANGLES, ds);
      }
    };

    DrawState ds;
    draw_pm(ds, pm_info);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(window.raw());

    timer.update();
    fcounter.update();
  }
  return EXIT_SUCCESS;
}

#include <boomhs/ortho_renderer.hpp>

#include <boomhs/perspective_renderer.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui_debug.hpp>

#include <opengl/renderer.hpp>

#include <common/log.hpp>
#include <boomhs/math.hpp>
#include <boomhs/random.hpp>

#include <extlibs/imgui.hpp>
#include <cassert>

using namespace boomhs;
using namespace boomhs::math::constants;
using namespace opengl;

auto static constexpr WINDOW_FLAGS = (0
  | ImGuiWindowFlags_AlwaysAutoResize
  | ImGuiWindowFlags_NoBringToFrontOnFocus
  | ImGuiWindowFlags_NoMove
  | ImGuiWindowFlags_NoScrollbar
  | ImGuiWindowFlags_NoScrollWithMouse
  | ImGuiWindowFlags_NoSavedSettings
  | ImGuiWindowFlags_NoTitleBar
);

namespace
{

void
draw_lhs(GameState& gs, RenderState& rstate, LevelManager& lm, StaticRenderers& srs, Camera& camera,
               RNG& rng, glm::ivec2 const& divide, DrawState& ds, FrameTime const& ft)
{
  auto& fs = rstate.fs;
  auto& zs = fs.zs;
  auto& es = fs.es;
  auto& logger   = es.logger;

  auto& io = es.imgui;

  auto const& frustum = es.frustum;
  auto const vp = Viewport::from_frustum(frustum);
  Viewport const LHS{
    vp.left_top(), vp.half_width(), vp.height()
  };
  float const right  = LHS.right();
  float const bottom = LHS.bottom();
  io.DisplaySize = ImVec2{right, bottom};

  auto& ui_state = es.ui_state;
  if (ui_state.draw_debug_ui) {
    ui_debug::draw("Ortho", WINDOW_FLAGS, es, lm, camera, ft);
  }

  render::set_viewport_and_scissor(LHS, frustum.height());
  PerspectiveRenderer::draw_scene(rstate, lm, ds, camera, rng, srs, ft);
}

void
draw_rhs(RenderState& rstate, LevelManager& lm, StaticRenderers& srs, Camera& camera,
              RNG& rng, int const cutoff_point, DrawState& ds, FrameTime const& ft)
{
  auto& es = rstate.fs.es;
  auto const& frustum = es.frustum;
  auto const vp = Viewport::from_frustum(frustum);

  Viewport const RHS{
    cutoff_point, vp.top(), vp.width(), vp.height()
  };
  render::set_viewport_and_scissor(RHS, frustum.height());
  PerspectiveRenderer::draw_scene(rstate, lm, ds, camera, rng, srs, ft);
}

} // namespace

namespace boomhs
{

void
OrthoRenderer::draw_scene(GameState& gs, RenderState& rstate, LevelManager& lm, DrawState& ds, Camera& camera,
             RNG& rng, StaticRenderers& srs, FrameTime const& ft)
{
  auto& fs = rstate.fs;
  auto& es = fs.es;

  auto const frustum = Viewport::from_frustum(es.frustum);
  int const divide_width  = frustum.width() / 3;
  int const divide_height = frustum.height() / 2;
  auto const divide = IVEC2(divide_width, divide_height);
  draw_lhs(gs, rstate, lm, srs, camera, rng, divide, ds, ft);
  draw_rhs(rstate, lm, srs, camera, rng, divide.x, ds, ft);
}

} // namespace boomhs

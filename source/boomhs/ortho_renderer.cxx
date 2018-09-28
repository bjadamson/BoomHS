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
draw_ortho_lhs(EngineState& es, LevelManager& lm, Camera& camera, PixelT const cutoff_point,
               FrameTime const& ft)
{
  auto const vp = Viewport::from_float_rect(es.frustum.LT_RB_rect());
  Viewport const LHS{
    vp.left(), vp.top(), cutoff_point, vp.bottom()
  };
  render::set_viewport(LHS);

  auto& io = es.imgui;
  float const right  = LHS.right();
  float const bottom = LHS.bottom();
  io.DisplaySize = ImVec2{right, bottom};
  auto& ui_state = es.ui_state;
  if (ui_state.draw_debug_ui) {
    ui_debug::draw("Ortho", WINDOW_FLAGS, es, lm, camera, ft);
  }
}

void
draw_ortho_rhs(RenderState& rstate, DrawState& ds, LevelManager& lm,
              StaticRenderers& static_renderers, Camera& camera, RNG& rng,
              PixelT const cutoff_point, FrameTime const& ft)
{
  auto& es = rstate.fs.es;
  auto const vp = Viewport::from_float_rect(es.frustum.LT_RB_rect());

  Viewport const RHS{
    cutoff_point, vp.top(), vp.right(), vp.right_bottom().y
  };
  render::set_viewport(RHS);
  PerspectiveRenderer::draw_scene(rstate, lm, ds, camera, rng, static_renderers, ft);
}

} // namespace

namespace boomhs
{

void
OrthoRenderer::draw_scene(RenderState& rstate, LevelManager& lm, DrawState& ds, Camera& camera,
             RNG& rng, StaticRenderers& static_renderers, FrameTime const& ft)
{
  auto& fs = rstate.fs;
  auto& es = fs.es;

  PixelT const cutoff_point = 0;
  //draw_ortho_lhs(es, lm, camera, cutoff_point, ft);
  draw_ortho_rhs(rstate, ds, lm, static_renderers, camera, rng, cutoff_point, ft);
}

} // namespace boomhs

#include <boomhs/ortho_renderer.hpp>
#include <boomhs/components.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/perspective_renderer.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui_debug.hpp>

#include <opengl/factory.hpp>
#include <opengl/gpu.hpp>
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

  auto& gfx_state = zs.gfx_state;
  auto& sps       = zs.gfx_state.sps;
  auto& sp        = sps.ref_sp("2dcolor");

  auto const& frustum = es.frustum;
  auto const vp = Viewport::from_frustum(frustum);
  Viewport const LHS{
    vp.left_top(), vp.half_width(), vp.height()
  };

  render::set_viewport_and_scissor(LHS, frustum.height());

  auto top_left    = vp.rect_float();
  auto bottom_left = top_left;

  auto const midpoint = top_left.bottom / 2;
  top_left.bottom = midpoint;
  bottom_left.top = midpoint;

  {
    auto buffer     = OF::RectBuilder{top_left}.build();
    DrawInfo dinfo = gpu::copy_rectangle(logger, sp.va(), buffer);
    BIND_UNTIL_END_OF_SCOPE(logger, sp);

    auto constexpr NEAR   = 1.0f;
    auto constexpr FAR    = -1.0f;

    auto const& f   = es.frustum;
    auto const pm = glm::ortho(f.left_float(), f.right_float(), f.bottom_float(), f.top_float(), NEAR, FAR);
    sp.set_uniform_matrix_4fv(logger, "u_projmatrix", pm);

    auto color = LOC::SANDY_BROWN;
    sp.set_uniform_color(logger, "u_color", color);

    BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
    render::draw_2d(rstate, GL_TRIANGLES, sp, dinfo);
  }

  auto const right_bottom = LHS.right_bottom();
  auto const left_top     = LHS.left_top();


  auto const top_left_imgui    = ImVec2{top_left.left,    top_left.top};
  auto const bottom_left_imgui = ImVec2{bottom_left.left, bottom_left.top};

  auto const lhs_rect = LHS.rect_float();
  ImGui::SetNextWindowPos(bottom_left_imgui);
  ImGui::SetNextWindowSize(top_left_imgui);

  {
    auto const& ttable    = gfx_state.texture_table;
    assert(ttable.find("moon"));
    auto const& moon_ti = *ttable.find("moon");

    auto const draw_button = [&](int const i, TextureInfo const& ti) {
      ImTextureID im_texid = reinterpret_cast<void*>(ti.id);

      imgui_cxx::ImageButtonBuilder image_builder;
      image_builder.frame_padding = 0;
      image_builder.bg_color      = ImColor{255, 255, 255, 255};
      image_builder.tint_color    = ImColor{255, 255, 255, 128};

      auto const size = ImVec2(32, 32);
      image_builder.build(im_texid, size);
    };

    auto const draw_selected_unit_grid = [&]() {
      auto& registry = zs.registry;
      int const COL_WIDTH = 4;

      int count = find_all_entities_with_component<Selectable>(registry)
        .count([](auto const& s) { return s.selected; });
      imgui_cxx::draw_grid(count, COL_WIDTH, draw_button, moon_ti);
    };
    auto constexpr flags = (0 | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoTitleBar);
    auto const draw_window = [&]() {
      imgui_cxx::with_window(draw_selected_unit_grid, "Selected Units", nullptr, flags);
    };
    imgui_cxx::with_stylevars(draw_window, ImGuiStyleVar_ChildRounding, 5.0f);
  }
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

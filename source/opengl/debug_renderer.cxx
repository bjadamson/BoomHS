#include <opengl/debug_renderer.hpp>
#include <opengl/factory.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>

#include <boomhs/components.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/math.hpp>
#include <boomhs/player.hpp>

#include <boomhs/random.hpp>

#include <boomhs/clock.hpp>
#include <extlibs/glm.hpp>

using namespace boomhs;
using namespace boomhs::math;
using namespace opengl;

namespace
{

struct WorldOriginArrows
{
  DrawInfo x_dinfo;
  DrawInfo y_dinfo;
  DrawInfo z_dinfo;
};

WorldOriginArrows
create_axis_arrows(common::Logger &logger, VertexAttribute const& va)
{
  auto constexpr ORIGIN = constants::ZERO;
  ArrowCreateParams constexpr acx{LOC::RED,   ORIGIN, constants::X_UNIT_VECTOR};
  ArrowCreateParams constexpr acy{LOC::GREEN, ORIGIN, constants::Y_UNIT_VECTOR};
  ArrowCreateParams constexpr acz{LOC::BLUE,  ORIGIN, constants::Z_UNIT_VECTOR};

  auto const avx = ArrowFactory::create_vertices(acx);
  auto const avy = ArrowFactory::create_vertices(acy);
  auto const avz = ArrowFactory::create_vertices(acz);

  auto x = OG::copy_arrow(logger, va, avx);
  auto y = OG::copy_arrow(logger, va, avy);
  auto z = OG::copy_arrow(logger, va, avz);
  return WorldOriginArrows{MOVE(x), MOVE(y), MOVE(z)};
}

void
draw_axis(RenderState& rstate, glm::vec3 const& pos)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& zs     = fstate.zs;
  auto& sps    = zs.gfx_state.sps;

  auto& logger = es.logger;

  auto& sp           = sps.ref_sp("3d_pos_color");
  auto  world_arrows = create_axis_arrows(logger, sp.va());

  auto const& ldata = zs.level_data;
  Transform   transform;
  transform.translation = pos;

  auto const draw_axis_arrow = [&](DrawInfo& dinfo) {
    dinfo.while_bound(logger, [&]() { render::draw_2d(rstate, GL_LINES, sp, dinfo); });
  };

  sp.while_bound(logger, [&]() {
    auto const camera_matrix = fstate.camera_matrix();
    render::set_mvpmatrix(logger, camera_matrix, transform.model_matrix(), sp);
    draw_axis_arrow(world_arrows.x_dinfo);
    draw_axis_arrow(world_arrows.y_dinfo);
    draw_axis_arrow(world_arrows.z_dinfo);
  });

  LOG_TRACE("Finished Drawing Global Axis");
}

void
conditionally_draw_player_vectors(RenderState& rstate, Player const& player)
{
  auto& fstate = rstate.fs;
  auto& es       = fstate.es;
  auto& zs       = fstate.zs;
  auto& registry = zs.registry;

  auto& logger = es.logger;

  auto const draw_local_axis = [&rstate](auto const& wo) {
    glm::vec3 const pos = wo.world_position();
    // local-space
    //
    // eye-forward
    auto const fwd = wo.eye_forward();
    render::draw_arrow(rstate, pos, pos + (2.0f * fwd), LOC::PURPLE);

    // eye-up
    auto const up = wo.eye_up();
    render::draw_arrow(rstate, pos, pos + up, LOC::YELLOW);

    // eye-right
    auto const right = wo.eye_right();
    render::draw_arrow(rstate, pos, pos + right, LOC::ORANGE);
  };

  if (es.show_player_localspace_vectors) {
    draw_local_axis(player.world_object());
    draw_local_axis(player.head_world_object());
  }
  if (es.show_player_worldspace_vectors) {
    draw_axis(rstate, player.world_position());
    draw_axis(rstate, player.head_world_object().transform().translation);
  }
}

void
draw_frustum(RenderState& rstate, Frustum const& frustum, glm::mat4 const& model)
{
  auto& fstate     = rstate.fs;
  auto const proj  = fstate.projection_matrix();
  auto const view  = fstate.view_matrix();

  auto const mvp = math::compute_mvp_matrix(model, view, proj);
  glm::mat4 const inv_viewproj = glm::inverse(mvp);

  glm::vec4 const f[8u] =
  {
      // near face
      {-1, -1, frustum.near, 1.0f},
      { 1, -1, frustum.near, 1.0f},
      { 1,  1, frustum.near, 1.0f},
      {-1,  1, frustum.near, 1.0f},

      // far face
      {-1, -1, frustum.far, 1.0f},
      { 1, -1, frustum.far, 1.0f},
      { 1,  1, frustum.far, 1.0f},
      {-1,  1, frustum.far, 1.0f}
  };

  glm::vec3 v[8u];
  for (int i = 0; i < 8; i++)
  {
    glm::vec4 const ff = inv_viewproj * f[i];
    v[i].x = ff.x / ff.w;
    v[i].y = ff.y / ff.w;
    v[i].z = ff.z / ff.w;
  }

  render::draw_line(rstate, v[0], v[1], LOC::BLUE);
  render::draw_line(rstate, v[1], v[2], LOC::BLUE);
  render::draw_line(rstate, v[2], v[3], LOC::BLUE);
  render::draw_line(rstate, v[3], v[0], LOC::BLUE);

  render::draw_line(rstate, v[4], v[5], LOC::GREEN);
  render::draw_line(rstate, v[5], v[6], LOC::GREEN);
  render::draw_line(rstate, v[6], v[7], LOC::GREEN);
  render::draw_line(rstate, v[7], v[4], LOC::GREEN);

  // connecting lines
  render::draw_line(rstate, v[0], v[4], LOC::RED);
  render::draw_line(rstate, v[1], v[5], LOC::RED);
  render::draw_line(rstate, v[2], v[6], LOC::RED);
  render::draw_line(rstate, v[3], v[7], LOC::RED);
}

} // namespace

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// DebugRenderer
void
DebugRenderer::render_scene(RenderState& rstate, LevelManager& lm, Camera& camera,
                                   RNG& rng, FrameTime const& ft)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto& zs       = fstate.zs;
  auto& registry = zs.registry;
  auto& ldata    = zs.level_data;

  if (es.show_grid_lines) {
    render::draw_grid_lines(rstate);
  }

  auto& player = find_player(registry);
  if (es.show_global_axis) {
    draw_axis(rstate, constants::ZERO);
  }

  Transform camera_transform;
  camera_transform.translation = camera.world_position();
  auto const model = camera_transform.model_matrix();

  if (es.draw_view_frustum) {
    draw_frustum(rstate, camera.frustum_ref(), model);
  }

  // if checks happen inside fn
  conditionally_draw_player_vectors(rstate, player);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BlackSceneRenderer
void
BlackSceneRenderer::render_scene(RenderState&, LevelManager&, Camera&, RNG&,
               FrameTime const&)
{
}

} // namespace opengl

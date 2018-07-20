#include <boomhs/billboard.hpp>
#include <boomhs/entity_renderer.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/player.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tree.hpp>

#include <stlw/random.hpp>
#include <window/timer.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

template <typename... Args>
void
draw_entity_fn(RenderState& rstate, GLenum const dm, bool const black_silhoutte, ShaderProgram& spp,
               DrawInfo& dinfo, EntityID const eid, ShaderName& sn, Transform const& transform,
               IsVisible& is_v, AABoundingBox& bbox, Args&&...)
{
  auto&       fstate = rstate.fs;
  auto const& es     = fstate.es;
  auto&       logger = es.logger;
  auto&       zs     = fstate.zs;

  auto& registry = zs.registry;
  auto& sps      = zs.gfx_state.sps;

  auto const& ldata  = zs.level_data;
  auto const& player = ldata.player;

  bool const skip = !is_v.value;
  if (skip) {
    return;
  }

  auto const& tr = transform.translation;
  {
    /*
    // TODO: only call recalulate when the camera moves
    Frustum view_frust;
    view_frust.recalculate(fstate);

    float const halfsize        = glm::length(bbox.max - bbox.min) / 2.0f;
    bool const  bbox_in_frustum = view_frust.cube_in_frustum(tr, halfsize);

    if (!bbox_in_frustum) {
      return;
    }
    */
  }

  bool constexpr SET_NORMALMATRIX = true;

  bool const is_lightsource = registry.has<PointLight>(eid);
  auto const model_matrix   = transform.model_matrix();

  auto& vao = dinfo.vao();

  auto& sp = black_silhoutte ? sps.ref_sp("blacksilhoutte") : spp;
  if (black_silhoutte) {
    bind::global_bind(logger, sp);
  }
  if (black_silhoutte) {
    sp.set_uniform_int1(logger, "u_sampler", 0);
    glActiveTexture(GL_TEXTURE0);
  }
  vao.while_bound(logger, [&]() {
    if (is_lightsource) {
      assert(is_lightsource);
      render::draw_3dlightsource(rstate, dm, model_matrix, sp, dinfo, eid, registry);
      return;
    }

    bool const receives_light = registry.has<Material>(eid);
    if (receives_light) {
      Material const& material = registry.get<Material>(eid);
      render::draw_3dlit_shape(rstate, dm, tr, model_matrix, sp, dinfo, material, registry,
                               SET_NORMALMATRIX);
      return;
    }

    // Can't receive light
    assert(!registry.has<Material>());

    if (!sp.is_2d) {
      auto const camera_matrix = fstate.camera_matrix();
      render::set_mvpmatrix(logger, camera_matrix, model_matrix, sp);
    }
    render::draw(rstate, dm, sp, dinfo);
  });
  if (black_silhoutte) {
    bind::global_unbind(logger, sp);
  }
}

} // namespace

namespace boomhs
{

void
EntityRenderer::render(RenderState& rstate, stlw::float_generator& rng, FrameTime const& ft,
                       bool const black_silhoutte)
{
  auto&       fstate    = rstate.fs;
  auto const& es        = fstate.es;
  auto&       logger    = es.logger;
  auto&       zs        = fstate.zs;
  auto&       gpu_state = zs.gfx_state.gpu_state;

  auto& eh   = gpu_state.entities;
  auto& ebbh = gpu_state.entity_boundingboxes;

  auto& registry = zs.registry;
  auto& sps      = zs.gfx_state.sps;

  auto const& ldata  = zs.level_data;
  auto const& player = ldata.player;

#define COMMON ShaderName, Transform, IsVisible, AABoundingBox
#define COMMON_ARGS auto const eid, auto &sn, auto &transform, auto &is_v, auto &bbox

  auto const draw_entity = [&](COMMON_ARGS, auto&&... args) {
    auto&       dinfo             = eh.lookup(logger, eid);
    auto const& global_light      = ldata.global_light;
    auto const& directional_light = global_light.directional;

    auto& sp = sps.ref_sp(sn.value);
    sp.while_bound(logger, [&]() {
      draw_entity_fn(rstate, GL_TRIANGLES, black_silhoutte, sp, dinfo, eid, sn, transform, is_v,
                     bbox, FORWARD(args));
    });
  };

  auto const draw_textured_junk_fn = [&](COMMON_ARGS, auto& texture_renderable, auto&&... args) {
    auto* ti = texture_renderable.texture_info;
    assert(ti);
    ti->while_bound(logger, [&]() {
      draw_entity(eid, sn, transform, is_v, bbox, texture_renderable, FORWARD(args));
    });
  };

  auto const draw_orbital_body = [&](COMMON_ARGS, auto& bboard, OrbitalBody&,
                                     TextureRenderable& trenderable) {
    auto const bb_type    = bboard.value;
    auto const view_model = Billboard::compute_viewmodel(transform, fstate.view_matrix(), bb_type);

    auto const proj_matrix = fstate.projection_matrix();
    auto const mvp_matrix  = proj_matrix * view_model;
    auto&      sp          = sps.ref_sp(sn.value);
    sp.while_bound(logger, [&]() { render::set_modelmatrix(logger, mvp_matrix, sp); });

    auto* ti = trenderable.texture_info;
    assert(ti);

    ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();
    ti->while_bound(logger, [&]() { draw_entity(eid, sn, transform, is_v, bbox, bboard); });
  };

  auto const draw_junk = [&](COMMON_ARGS, Color&, JunkEntityFromFILE& je) {
    auto& dinfo = eh.lookup(logger, eid);
    auto& sp    = sps.ref_sp(sn.value);
    sp.while_bound(logger, [&]() {
      draw_entity_fn(rstate, je.draw_mode, black_silhoutte, sp, dinfo, eid, sn, transform, is_v,
                     bbox);
    });
  };
  auto const draw_torch = [&](COMMON_ARGS, Torch& torch, TextureRenderable& trenderable) {
    {
      auto& sp = sps.ref_sp(sn.value);

      // Describe glow
      static constexpr double MIN   = 0.3;
      static constexpr double MAX   = 1.0;
      static constexpr double SPEED = 0.135;
      auto const              a     = std::sin(ft.since_start_millis() * M_PI * SPEED);
      float const             glow  = glm::lerp(MIN, MAX, std::abs(a));
      sp.while_bound(logger, [&]() { sp.set_uniform_float1(logger, "u_glow", glow); });
    }

    // randomize the position slightly
    static constexpr auto DISPLACEMENT_MAX = 0.0015f;

    auto copy_transform = transform;
    copy_transform.translation.x += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);
    copy_transform.translation.y += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);
    copy_transform.translation.z += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);

    auto* ti = trenderable.texture_info;
    assert(ti);
    ti->while_bound(logger, [&]() { draw_entity(eid, sn, copy_transform, is_v, bbox, torch); });
  };
  auto const draw_boundingboxes = [&](COMMON_ARGS, Selectable& sel, auto&&...) {
    if (!es.draw_bounding_boxes) {
      return;
    }
    Color const wire_color = sel.selected ? LOC::GREEN : LOC::RED;

    auto& sp    = sps.ref_sp("wireframe");
    auto& dinfo = ebbh.lookup(logger, eid);
    auto  tr    = transform;

    sp.while_bound(logger, [&]() {
      sp.set_uniform_color(logger, "u_wirecolor", wire_color);
      draw_entity_fn(rstate, GL_LINES, black_silhoutte, sp, dinfo, eid, sn, tr, is_v, bbox);
    });
  };

  auto const draw_plain_cube = [&](COMMON_ARGS, CubeRenderable& cr, auto&&... args) {
    draw_entity(eid, sn, transform, is_v, bbox, cr, FORWARD(args));
  };

  auto const draw_tree = [&](COMMON_ARGS, auto& tree, auto&&... args) {
    draw_entity(eid, sn, transform, is_v, bbox, tree, FORWARD(args));
  };
#undef COMMON_ARGS

  // define rendering order here
  // OrbitalBodies always render first.
  registry.view<COMMON, BillboardRenderable, OrbitalBody, TextureRenderable>().each(
      draw_orbital_body);

  registry.view<COMMON, TextureRenderable, JunkEntityFromFILE>().each(draw_textured_junk_fn);

  registry.view<COMMON, Torch, TextureRenderable>().each(draw_torch);
  registry.view<COMMON, Color, JunkEntityFromFILE>().each(draw_junk);
  registry.view<COMMON, TreeComponent>().each(draw_tree);

  // CUBES
  registry.view<COMMON, CubeRenderable, PointLight>().each(draw_plain_cube);
  registry.view<COMMON, Selectable>().each(draw_boundingboxes);
  // registry.view<COMMON, Selectable, JunkEntityFromFILE>().each(draw_boundingboxes);

  registry.view<COMMON, MeshRenderable, NPCData>().each(
      [&](auto&&... args) { draw_entity(FORWARD(args)); });
  registry.view<COMMON, MeshRenderable, PlayerData>().each(
      [&](auto&&... args) { draw_entity(FORWARD(args)); });
#undef COMMON
}

} // namespace boomhs

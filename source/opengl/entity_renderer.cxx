#include <opengl/entity_renderer.hpp>
#include <opengl/renderer.hpp>

#include <boomhs/billboard.hpp>
#include <boomhs/bounding_object.hpp>
#include <boomhs/components.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/material.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/view_frustum.hpp>
#include <boomhs/zone_state.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

// clang-format off
#define RENDER_3D_ENTITIES(                                                                        \
    DRAW_COMMON_________FN,                                                                        \
    DRAW_TORCH__________FN,                                                                        \
    DRAW_DEFAULT_ENTITY_FN,                                                                        \
    DRAW_POINTLIGHTS____FN,                                                                        \
    COMMON______COMPONENTS                                                                         \
    /* last argument is a list of all the components to render */                                  \
    )                                                                                              \
    render_common_3d_entities<                                                                     \
          decltype(DRAW_COMMON_________FN)                                                         \
        , decltype(DRAW_TORCH__________FN)                                                         \
        , decltype(DRAW_DEFAULT_ENTITY_FN)                                                         \
        , decltype(DRAW_POINTLIGHTS____FN)                                                         \
        , COMMON______COMPONENTS                                                                   \
        >                                                                                          \
        (rstate, rng, ft                                                                           \
         , draw_common_fn                                                                          \
         , draw_torch_fn                                                                           \
         , draw_default_entity_fn                                                                  \
         , draw_pointlight_fn                                                                      \
         )
// clang-format on

void
draw_shape_with_light(RenderState& rstate, GLenum const dm, EntityID const eid,
                      EntityRegistry& registry, ShaderProgram& sp, DrawInfo& dinfo,
                      glm::vec3 const& tr, glm::mat4 const& model_matrix)
{
  Material const& material = registry.get<Material>(eid);

  // When drawing entities, we always want the normal matrix set.
  bool constexpr SET_NORMALMATRIX = true;
  render::draw_3dlit_shape(rstate, dm, tr, model_matrix, sp, dinfo, material, registry,
                           SET_NORMALMATRIX);
}

template <typename... Args>
void
draw_entity_common_without_binding_sp(RenderState& rstate, GLenum const dm, ShaderProgram& sp,
                                      DrawInfo& dinfo, EntityID const eid,
                                      Transform const& transform)
{
  auto&      fstate       = rstate.fs;
  auto&      es           = fstate.es;
  auto&      logger       = es.logger;
  auto&      zs           = fstate.zs;
  auto&      registry     = zs.registry;
  auto const model_matrix = transform.model_matrix();

  bool const is_lightsource = registry.has<PointLight>(eid);
  bool const receives_light = registry.has<Material>(eid);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  if (is_lightsource) {
    LOG_WARN("LIGHTSOURCE");
    render::draw_3dlightsource(rstate, dm, model_matrix, sp, dinfo, eid, registry);
  }
  else if (receives_light) {
    LOG_WARN("RECEVIES LIGHT");
    auto const& tr = transform.translation;
    draw_shape_with_light(rstate, dm, eid, registry, sp, dinfo, tr, model_matrix);
    return;
  }
  else {
    LOG_WARN("WITHOUT LIGHT");
    render::draw(logger, rstate.ds, dm, sp, dinfo);
  }
}

// This function performs more work than just drawing the shapes directly.
//
// 1. It checks if the entity is visible, returning early if it is.
// 2. It looks up the DrawInfo in the "Entity handlemap" passed in.
// 3. It binds the provided shader program
// 4. Draws the entity.
template <typename... Args>
void
draw_entity(RenderState& rstate, GLenum const dm, ShaderProgram& sp, EntityID const eid,
            DrawInfo& dinfo, Transform& transform, IsRenderable& is_r, AABoundingBox& bbox,
            Args&&... args)
{
  // If entity is not visible, just return.
  if (is_r.hidden) {
    return;
  }

  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;
  auto& zs     = fstate.zs;

  glm::mat4 const& view_mat = fstate.view_matrix();
  glm::mat4 const& proj_mat = fstate.projection_matrix();
  if (!ViewFrustum::bbox_inside(view_mat, proj_mat, transform, bbox)) {
    return;
  }

  sp.while_bound(logger, [&]() {
    draw_entity_common_without_binding_sp(rstate, dm, sp, dinfo, eid, transform);
  });
}

void
draw_orbital_body(RenderState& rstate, ShaderProgram& sp, EntityID const eid, Transform& transform,
                  IsRenderable& is_r, AABoundingBox& bbox, BillboardRenderable& bboard,
                  OrbitalBody&, TextureRenderable& trenderable)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto const bb_type    = bboard.value;
  auto const view_model = Billboard::compute_viewmodel(transform, fstate.view_matrix(), bb_type);

  auto const proj_matrix = fstate.projection_matrix();
  auto const mvp_matrix  = proj_matrix * view_model;
  sp.while_bound(logger, [&]() {
    sp.set_uniform_mat4(logger, "u_mv", mvp_matrix);
    // render::set_modelmatrix(logger, mvp_matrix, sp);
  });

  auto* ti = trenderable.texture_info;
  assert(ti);

  ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();

  auto& zs           = fstate.zs;
  auto& draw_handles = zs.gfx_state.draw_handles;
  auto& dinfo        = draw_handles.lookup_entity(logger, eid);

  BIND_UNTIL_END_OF_SCOPE(logger, *ti);
  draw_entity(rstate, GL_TRIANGLES, sp, eid, dinfo, transform, is_r, bbox);
}

template <typename DrawCommonFN, typename DrawTorchFN, typename DrawDefaultEntityFN,
          typename DrawPointlightFN, typename... Common>
void
render_common_3d_entities(RenderState& rstate, RNG& rng, FrameTime const& ft,
                          DrawCommonFN const& draw_common_fn, DrawTorchFN const& draw_torch_fn,
                          DrawDefaultEntityFN const& draw_default_entity_fn,
                          DrawPointlightFN const&    draw_pointlight_fn)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  LOG_TRACE("================ BEGIN RENDERING COMMON 3D ENTITIES ================");

  auto& zs       = fstate.zs;
  auto& registry = zs.registry;

  // define rendering order here

  LOG_TRACE("Rendering Torch");
  registry.view<Common..., TextureRenderable, Torch>().each(draw_torch_fn);

  LOG_TRACE("Rendering Book");
  registry.view<Common..., TextureRenderable, Book>().each(draw_default_entity_fn);

  LOG_TRACE("Rendering Weapon");
  registry.view<Common..., TextureRenderable, Weapon>().each(draw_default_entity_fn);

  LOG_TRACE("Rendering Junk");
  registry.view<Common..., JunkEntityFromFILE>().each(draw_default_entity_fn);

  LOG_TRACE("Rendering Trees");
  registry.view<Common..., TreeComponent>().each(draw_common_fn);

  // CUBES
  LOG_TRACE("Rendering Pointlights");
  registry.view<Common..., CubeRenderable, PointLight>().each(draw_pointlight_fn);

  LOG_TRACE("Rendering NPCs");
  registry.view<Common..., MeshRenderable, NPCData>().each(
      [&](auto&&... args) { draw_common_fn(FORWARD(args)); });

  // Only render the player if the camera isn't in FPS mode.
  if (CameraMode::FPS != fstate.camera_mode()) {
    LOG_TRACE("Rendering Player");
    registry.view<Common..., MeshRenderable, Player>().each(
        [&](auto&&... args) { draw_common_fn(FORWARD(args)); });
  }
  LOG_TRACE("================ END RENDERING COMMON 3D ENTITIES ================");
}

} // namespace

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// EntityRenderer
void
EntityRenderer::render2d_billboard(RenderState& rstate, RNG& rng, FrameTime const& ft)
{
  auto& fstate       = rstate.fs;
  auto& es           = fstate.es;
  auto& logger       = es.logger;
  auto& zs           = fstate.zs;
  auto& draw_handles = zs.gfx_state.draw_handles;

  auto& registry = zs.registry;
  auto& sps      = zs.gfx_state.sps;

#define COMMON ShaderName, Transform, IsRenderable, AABoundingBox
#define COMMON_ARGS auto const eid, auto &sn, auto &transform, auto &is_r, auto &bbox

  auto const draw_orbital_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp(sn.value);
    draw_orbital_body(rstate, sp, eid, transform, is_r, bbox, FORWARD(args));
  };

  LOG_TRACE("BEGIN Rendering 2d billboard entities with Default Entity Renderer");
  registry.view<COMMON, BillboardRenderable, OrbitalBody, TextureRenderable>().each(
      draw_orbital_fn);

  LOG_TRACE("BEGIN drawing target reticle with Default Entity Renderer");
  render::draw_targetreticle(rstate, ft);
  LOG_TRACE("END Rendering 2d billboard entities with Default Entity Renderer");
}

void
EntityRenderer::render2d_ui(RenderState& rstate, RNG& rng, FrameTime const& ft)
{
}

void
EntityRenderer::render3d(RenderState& rstate, RNG& rng, FrameTime const& ft)
{
  auto&       fstate       = rstate.fs;
  auto const& es           = fstate.es;
  auto&       logger       = es.logger;
  auto&       zs           = fstate.zs;
  auto&       draw_handles = zs.gfx_state.draw_handles;

  auto& registry = zs.registry;
  auto& sps      = zs.gfx_state.sps;

  auto const draw_common_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp(sn.value);
    assert(!sp.is_2d);
    auto& dinfo = draw_handles.lookup_entity(logger, eid);
    draw_entity(rstate, GL_TRIANGLES, sp, eid, dinfo, transform, is_r, bbox, FORWARD(args));
  };

  auto const draw_default_entity_fn = [&](COMMON_ARGS, auto&&...) {
    auto& sp = sps.ref_sp(sn.value);
    assert(!sp.is_2d);
    if (registry.has<TextureRenderable>(eid)) {
      assert(!registry.has<Color>(eid));

      auto& tr = registry.get<TextureRenderable>(eid);
      auto* ti = tr.texture_info;
      assert(ti);
      ti->while_bound(logger, [&]() { draw_common_fn(eid, sn, transform, is_r, bbox, tr); });
    }
    else {
      // assert(registry.has<Color>(eid));
      auto& dinfo = draw_handles.lookup_entity(logger, eid);
      draw_entity(rstate, GL_TRIANGLES, sp, eid, dinfo, transform, is_r, bbox);
    }
  };
  auto const draw_torch_fn = [&](COMMON_ARGS, TextureRenderable& trenderable, Torch& torch) {
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
    ti->while_bound(logger, [&]() { draw_common_fn(eid, sn, copy_transform, is_r, bbox, torch); });
  };

  auto const draw_boundingboxes = [&](std::pair<Color, Color> const& colors, EntityID const eid,
                                      Transform& transform, AABoundingBox& bbox, Selectable& sel,
                                      auto&&...) {
    if (!es.draw_bounding_boxes) {
      return;
    }
    Color const wire_color = sel.selected ? colors.first : colors.second;

    auto& sp = sps.ref_sp("wireframe");
    auto  tr = transform;

    BIND_UNTIL_END_OF_SCOPE(logger, sp);
    sp.set_uniform_color(logger, "u_wirecolor", wire_color);

    // We needed to bind the shader program to set the uniforms above, no reason to pay to bind
    // it again.
    auto const model_matrix = tr.model_matrix();
    auto&      dinfo        = bbox.draw_info;

    BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
    auto const camera_matrix = fstate.camera_matrix();
    render::set_mvpmatrix(logger, camera_matrix, model_matrix, sp);
    render::draw(logger, rstate.ds, GL_LINES, sp, dinfo);
  };

  auto const& draw_pointlight_fn = draw_common_fn;

  LOG_TRACE("BEGIN Rendering 3d entities with Default Entity Renderer");
  RENDER_3D_ENTITIES(draw_common_fn, draw_torch_fn, draw_default_entity_fn, draw_pointlight_fn,
                     COMMON);
  LOG_TRACE("END Rendering 3d entities with Default Entity Renderer");

#define COMMON_BBOX Transform, AABoundingBox, Selectable
  registry.view<COMMON_BBOX>().each(
      [&](auto&&... args) { draw_boundingboxes(PAIR(LOC::GREEN, LOC::RED), FORWARD(args)); });

  registry.view<COMMON_BBOX, WaterInfo>().each(
      [&](auto&&... args) { draw_boundingboxes(PAIR(LOC::BLUE, LOC::ORANGE), FORWARD(args)); });
#undef COMMON_BBOX
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// SilhouetteEntityRenderer
void
SilhouetteEntityRenderer::render2d_billboard(RenderState& rstate, RNG& rng, FrameTime const& ft)
{
  auto&       fstate   = rstate.fs;
  auto const& es       = fstate.es;
  auto&       logger   = es.logger;
  auto&       zs       = fstate.zs;
  auto&       registry = zs.registry;

  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;

  auto const draw_orbital_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp("2dsilhoutte_uv");
    sp.while_bound(logger, [&]() { sp.set_uniform_color_rgb(logger, "u_color", LOC::WHITE); });

    draw_orbital_body(rstate, sp, eid, transform, is_r, bbox, FORWARD(args));
  };

  LOG_TRACE("BEGIN Rendering Billboard entities with SilhouetteEntityRenderer");
  registry.view<COMMON, BillboardRenderable, OrbitalBody, TextureRenderable>().each(
      draw_orbital_fn);
  LOG_TRACE("END Rendering Billboard entities with SilhouetteEntityRenderer");

  auto const draw_pointlight_fn = [&](COMMON_ARGS, auto&&... args) {};
}

void
SilhouetteEntityRenderer::render2d_ui(RenderState& rstate, RNG& rng, FrameTime const& ft)
{
}

void
SilhouetteEntityRenderer::render3d(RenderState& rstate, RNG& rng, FrameTime const& ft)
{
  auto&       fstate = rstate.fs;
  auto const& es     = fstate.es;
  auto&       logger = es.logger;
  auto&       zs     = fstate.zs;

  auto& gfx_state    = zs.gfx_state;
  auto& sps          = gfx_state.sps;
  auto& draw_handles = gfx_state.draw_handles;

  auto const draw_common_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp("silhoutte_black");
    if (!sp.is_2d) {
      auto& dinfo = draw_handles.lookup_entity(logger, eid);

      BIND_UNTIL_END_OF_SCOPE(logger, sp);
      BIND_UNTIL_END_OF_SCOPE(logger, dinfo);

      auto const camera_matrix = fstate.camera_matrix();
      auto const model_matrix  = transform.model_matrix();
      render::set_mvpmatrix(logger, camera_matrix, model_matrix, sp);
      render::draw(logger, rstate.ds, GL_TRIANGLES, sp, dinfo);
    }
  };

  auto const draw_pointlight_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp(sn.value);

    if (!sp.is_2d) {
      auto& dinfo = draw_handles.lookup_entity(logger, eid);
      draw_entity(rstate, GL_TRIANGLES, sp, eid, dinfo, transform, is_r, bbox, FORWARD(args));
    }
  };

  auto const& draw_torch_fn          = draw_common_fn;
  auto const& draw_default_entity_fn = draw_common_fn;

  LOG_TRACE("BEGIN Rendering 3d entities with SilhouetteEntityRenderer");
  RENDER_3D_ENTITIES(draw_common_fn, draw_torch_fn, draw_default_entity_fn, draw_pointlight_fn,
                     COMMON);
  LOG_TRACE("END Rendering 3d entities with SilhouetteEntityRenderer");
}

#undef COMMON
#undef COMMON_ARGS

} // namespace opengl

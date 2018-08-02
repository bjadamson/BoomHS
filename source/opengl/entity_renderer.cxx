#include <opengl/entity_renderer.hpp>

#include <boomhs/billboard.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/material.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tree.hpp>

#include <opengl/renderer.hpp>

#include <stlw/log.hpp>
#include <stlw/random.hpp>
#include <window/timer.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

// clang-format off
#define RENDER_ENTITIES(                                                                           \
    DRAW_COMMON________FN,                                                                         \
    DRAW_ORBITALS______FN,                                                                         \
    DRAW_TORCH_________FN,                                                                         \
    DRAW_JUNK__________FN,                                                                         \
    DRAW_TEXTURED_JUNK_FN,                                                                         \
    DRAW_POINTLIGHTS___FN,                                                                         \
    COMMON_____COMPONENTS                                                                          \
    /* last argument is a list of all the components to render */                                  \
    )                                                                                              \
    render_common_entities<                                                                        \
          decltype(DRAW_COMMON________FN)                                                          \
        , decltype(DRAW_ORBITALS______FN)                                                          \
        , decltype(DRAW_TORCH_________FN)                                                          \
        , decltype(DRAW_JUNK__________FN)                                                          \
        , decltype(DRAW_TEXTURED_JUNK_FN)                                                          \
        , decltype(DRAW_POINTLIGHTS___FN)                                                          \
        , COMMON_____COMPONENTS                                                                    \
        >                                                                                          \
        (rstate, rng, ft                                                                           \
         , draw_common_fn                                                                          \
         , draw_orbital_fn                                                                         \
         , draw_torch_fn                                                                           \
         , draw_junk_fn                                                                            \
         , draw_textured_junk_fn                                                                   \
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

void
draw_object_withoutlight(RenderState& rstate, GLenum const dm, EntityRegistry& registry,
                         FrameState& fstate, ShaderProgram& sp, DrawInfo& dinfo,
                         glm::mat4 const& model_matrix)
{
  auto& es     = fstate.es;
  auto& logger = es.logger;

  // Can't receive light
  assert(!registry.has<Material>());
  if (!sp.is_2d) {
    auto const camera_matrix = fstate.camera_matrix();
    render::set_mvpmatrix(logger, camera_matrix, model_matrix, sp);
  }
  render::draw(rstate, dm, sp, dinfo);
}

template <typename... Args>
void
draw_entity_common_without_binding_sp(RenderState& rstate, GLenum const dm, ShaderProgram& sp,
                                      DrawInfo& dinfo, EntityID const eid,
                                      Transform const& transform)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;
  auto& zs     = fstate.zs;
  auto& registry = zs.registry;
  auto const model_matrix   = transform.model_matrix();

  auto const draw = [&]() {
    bool const is_lightsource = registry.has<PointLight>(eid);
    bool const receives_light = registry.has<Material>(eid);
    if (is_lightsource) {
      render::draw_3dlightsource(rstate, dm, model_matrix, sp, dinfo, eid, registry);
    }
    else if (receives_light) {
      auto const& tr = transform.translation;
      draw_shape_with_light(rstate, dm, eid, registry, sp, dinfo, tr, model_matrix);
      return;
    }
    else {
      draw_object_withoutlight(rstate, dm, registry, fstate, sp, dinfo, model_matrix);
    }
  };

  auto& vao = dinfo.vao();
  vao.while_bound(logger, draw);
}

bool
bbox_in_frustrum(FrameState const& fstate, AABoundingBox const& bbox)
{
  /*
  // TODO: only call recalulate when the camera moves
  Frustum view_frust;
  view_frust.recalculate(fstate);

  float const halfsize        = glm::length(bbox.max - bbox.min) / 2.0f;
  bool const  bbox_in_frustum = view_frust.cube_in_frustum(tr, halfsize);

  return bbox_in_frustrum;
  */
  return false;
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
                   EntityDrawHandleMap &hmap, Transform& transform, IsVisible& is_v,
                   AABoundingBox& bbox, Args&&... args)
{
  // If entity is not visible, just return.
  if (!is_v.value) {
    return;
  }

  auto& fstate    = rstate.fs;
  auto& es        = fstate.es;
  auto& logger    = es.logger;
  auto& zs        = fstate.zs;

  if (bbox_in_frustrum(fstate, bbox)) {
    return;
  }

  auto& dinfo = hmap.lookup(logger, eid);
  sp.while_bound(logger, [&]() {
    draw_entity_common_without_binding_sp(rstate, dm, sp, dinfo, eid, transform);
  });
}

void
draw_orbital_body(RenderState& rstate, ShaderProgram& sp, EntityID const eid, Transform& transform,
                  IsVisible& is_v, AABoundingBox& bbox, BillboardRenderable& bboard, OrbitalBody&,
                  TextureRenderable& trenderable)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto const bb_type    = bboard.value;
  auto const view_model = Billboard::compute_viewmodel(transform, fstate.view_matrix(), bb_type);

  auto const proj_matrix = fstate.projection_matrix();
  auto const mvp_matrix  = proj_matrix * view_model;
  sp.while_bound(logger, [&]() { render::set_modelmatrix(logger, mvp_matrix, sp); });

  auto* ti = trenderable.texture_info;
  assert(ti);

  ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();

  auto& zs        = fstate.zs;
  auto& gpu_state = zs.gfx_state.gpu_state;
  auto& hmap      = gpu_state.entities;
  ti->while_bound(logger, [&]() { draw_entity(rstate, GL_TRIANGLES, sp, eid, hmap,
        transform, is_v, bbox); });
}



template <typename DrawCommonFN,
          typename DrawOrbitalFN,
          typename DrawTorchFN,
          typename DrawJunkFN,
          typename DrawTexturedJunkFN,
          typename DrawPointlightFN,
          typename... Common>
void
render_common_entities(RenderState& rstate, stlw::float_generator& rng, FrameTime const& ft,
                       DrawCommonFN const&  draw_common_fn,
                       DrawOrbitalFN const& draw_orbital_body_helper,
                       DrawTorchFN const& draw_torch_fn,
                       DrawJunkFN const& draw_junk_fn,
                       DrawTexturedJunkFN const& draw_textured_junk_fn,
                       DrawPointlightFN const& draw_pointlight_fn)
{
  auto& fstate   = rstate.fs;
  auto& zs       = fstate.zs;
  auto& registry = zs.registry;

  // define rendering order here
  // OrbitalBodies always render first.
  registry.view<Common..., BillboardRenderable, OrbitalBody, TextureRenderable>().each(
      draw_orbital_body_helper);

  registry.view<Common..., TextureRenderable, JunkEntityFromFILE>().each(draw_textured_junk_fn);

  registry.view<Common..., Torch, TextureRenderable>().each(draw_torch_fn);
  registry.view<Common..., Color, JunkEntityFromFILE>().each(draw_junk_fn);
  registry.view<Common..., TreeComponent>().each(draw_common_fn);

  // CUBES
  registry.view<Common..., CubeRenderable, PointLight>().each(draw_pointlight_fn);

  registry.view<Common..., MeshRenderable, NPCData>().each(
      [&](auto&&... args) { draw_common_fn(FORWARD(args)); });
  registry.view<Common..., MeshRenderable, Player>().each(
      [&](auto&&... args) { draw_common_fn(FORWARD(args)); });
}

} // namespace

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// EntityRenderer
void
EntityRenderer::render2d_billboard(RenderState& rstate, stlw::float_generator& rng, FrameTime const& ft)
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

#define COMMON                      ShaderName, Transform,       IsVisible,  AABoundingBox
#define COMMON_ARGS auto const eid, auto &sn,   auto &transform, auto &is_v, auto &bbox

  auto const draw_common_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp(sn.value);
    if (sp.is_2d) {
      draw_entity(rstate, GL_TRIANGLES, sp, eid, eh, transform, is_v, bbox, FORWARD(args));
    }
  };

  auto const draw_textured_junk_fn = [&](COMMON_ARGS, auto& texture_renderable, auto&&... args) {
  };

  auto const draw_orbital_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp(sn.value);
    draw_orbital_body(rstate, sp, eid, transform, is_v, bbox, FORWARD(args));
  };

  auto const draw_junk_fn = [&](COMMON_ARGS, Color&, JunkEntityFromFILE& je) {
  };
  auto const draw_torch_fn = [&](COMMON_ARGS, Torch& torch, TextureRenderable& trenderable) {
  };
  auto const draw_boundingboxes = [&](COMMON_ARGS, Selectable& sel, auto&&...) {
  };

  auto const& draw_pointlight_fn          = draw_common_fn;
RENDER_ENTITIES(
  draw_common_fn,
  draw_orbital_fn,
  draw_torch_fn,
  draw_junk_fn,
  draw_textured_junk_fn,
  draw_pointlight_fn,
  COMMON
      );

  render::draw_targetreticle(rstate, ft);

#undef COMMON
#undef COMMON_ARGS
}

void
EntityRenderer::render2d_ui(RenderState& rstate, stlw::float_generator& rng, FrameTime const& ft)
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

#define COMMON                      ShaderName, Transform,       IsVisible,  AABoundingBox
#define COMMON_ARGS auto const eid, auto &sn,   auto &transform, auto &is_v, auto &bbox

  auto const draw_common_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp(sn.value);

    if (sp.is_2d) {
      draw_entity(rstate, GL_TRIANGLES, sp, eid, eh, transform, is_v, bbox, FORWARD(args));
    }
  };

  auto const draw_textured_junk_fn = [&](COMMON_ARGS, auto& texture_renderable, auto&&... args) {
    auto* ti = texture_renderable.texture_info;
    assert(ti);
    ti->while_bound(logger, [&]() {
      draw_common_fn(eid, sn, transform, is_v, bbox, texture_renderable, FORWARD(args));
    });
  };

  auto const draw_orbital_fn = [&](COMMON_ARGS, auto&&... args) {
  };

  auto const draw_junk_fn = [&](COMMON_ARGS, Color&, JunkEntityFromFILE& je) {
  };
  auto const draw_torch_fn = [&](COMMON_ARGS, Torch& torch, TextureRenderable& trenderable) {
  };
  auto const draw_boundingboxes = [&](COMMON_ARGS, Selectable& sel, auto&&...) {
  };

  auto const& draw_pointlight_fn          = draw_common_fn;
RENDER_ENTITIES(
  draw_common_fn,
  draw_orbital_fn,
  draw_torch_fn,
  draw_junk_fn,
  draw_textured_junk_fn,
  draw_pointlight_fn,
  COMMON
      );
#undef COMMON
#undef COMMON_ARGS
}

void
EntityRenderer::render3d(RenderState& rstate, stlw::float_generator& rng, FrameTime const& ft)
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

#define COMMON                      ShaderName, Transform,       IsVisible,  AABoundingBox
#define COMMON_ARGS auto const eid, auto &sn,   auto &transform, auto &is_v, auto &bbox

  auto const draw_common_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp(sn.value);
    if (!sp.is_2d) {
      draw_entity(rstate, GL_TRIANGLES, sp, eid, eh, transform, is_v, bbox, FORWARD(args));
    }
  };

  auto const draw_textured_junk_fn = [&](COMMON_ARGS, auto& texture_renderable, auto&&... args) {
    auto* ti = texture_renderable.texture_info;
    assert(ti);
    ti->while_bound(logger, [&]() {
      draw_common_fn(eid, sn, transform, is_v, bbox, texture_renderable, FORWARD(args));
    });
  };

  auto const draw_orbital_fn = [&](COMMON_ARGS, auto&&... args) {
  };

  auto const draw_junk_fn = [&](COMMON_ARGS, Color&, JunkEntityFromFILE& je) {
    auto& sp    = sps.ref_sp(sn.value);
    if (!sp.is_2d) {
      draw_entity(rstate, je.draw_mode, sp, eid, eh, transform, is_v, bbox);
    }
  };
  auto const draw_torch_fn = [&](COMMON_ARGS, Torch& torch, TextureRenderable& trenderable) {
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
    ti->while_bound(logger, [&]() { draw_common_fn(eid, sn, copy_transform, is_v, bbox, torch); });
  };
  auto const draw_boundingboxes = [&](COMMON_ARGS, Selectable& sel, auto&&...) {
    if (!es.draw_bounding_boxes) {
      return;
    }
    Color const wire_color = sel.selected ? LOC::GREEN : LOC::RED;

    auto& sp    = sps.ref_sp("wireframe");
    auto  tr    = transform;

    sp.while_bound(logger, [&]() {
      sp.set_uniform_color(logger, "u_wirecolor", wire_color);
      auto& dinfo = ebbh.lookup(logger, eid);

      // We needed to bind the shader program to set the uniforms above, no reason to pay to bind
      // it again.
      draw_entity_common_without_binding_sp(rstate, GL_LINES, sp, dinfo, eid, transform);
    });
  };

  auto const& draw_pointlight_fn          = draw_common_fn;
RENDER_ENTITIES(
  draw_common_fn,
  draw_orbital_fn,
  draw_torch_fn,
  draw_junk_fn,
  draw_textured_junk_fn,
  draw_pointlight_fn,
  COMMON
      );

  registry.view<COMMON, Selectable>().each(
      [&](auto&&... args) { draw_boundingboxes(FORWARD(args)); });
#undef COMMON
#undef COMMON_ARGS
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BlackEntityRenderer
void
BlackEntityRenderer::render2d_billboard(RenderState& rstate, stlw::float_generator& rng, FrameTime const& ft)
{
  auto&       fstate = rstate.fs;
  auto const& es     = fstate.es;
  auto&       logger = es.logger;
  auto&       zs     = fstate.zs;

  auto& gfx_state = zs.gfx_state;
  auto& sps = gfx_state.sps;
  auto& gpu_state = gfx_state.gpu_state;
  auto& eh    = gpu_state.entities;

#define COMMON                      ShaderName, Transform,       IsVisible,  AABoundingBox
#define COMMON_ARGS auto const eid, auto& sn,   auto &transform, auto &is_v, auto &bbox

  auto const draw_common_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp("silhoutte_black");
    if (sp.is_2d) {
      draw_entity(rstate, GL_TRIANGLES, sp, eid, eh, transform, is_v, bbox, FORWARD(args));
    }
  };

  auto const draw_orbital_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp("2dsilhoutte_uv");
    sp.while_bound(logger, [&]() { sp.set_uniform_color_3fv(logger, "u_color", LOC::WHITE); });
    draw_orbital_body(rstate, sp, eid, transform, is_v, bbox, FORWARD(args));
  };

  auto const draw_pointlight_fn = [&](COMMON_ARGS, auto&&... args) {
  };
#undef COMMON_ARGS

  auto const& draw_torch_fn          = draw_common_fn;
  auto const& draw_junk_fn           = draw_common_fn;
  auto const& draw_textured_junk_fn  = draw_common_fn;

  RENDER_ENTITIES(
  draw_common_fn,
  draw_orbital_fn,
  draw_torch_fn,
  draw_junk_fn,
  draw_textured_junk_fn,
  draw_pointlight_fn,
  COMMON
      );
}

void
BlackEntityRenderer::render2d_ui(RenderState& rstate, stlw::float_generator& rng, FrameTime const& ft)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;
}

void
BlackEntityRenderer::render3d(RenderState& rstate, stlw::float_generator& rng, FrameTime const& ft)
{
  auto&       fstate = rstate.fs;
  auto const& es     = fstate.es;
  auto&       logger = es.logger;
  auto&       zs     = fstate.zs;

  auto& gfx_state = zs.gfx_state;
  auto& sps = gfx_state.sps;
  auto& gpu_state = gfx_state.gpu_state;
  auto& eh    = gpu_state.entities;

#define COMMON                      ShaderName, Transform,       IsVisible,  AABoundingBox
#define COMMON_ARGS auto const eid, auto& sn,   auto &transform, auto &is_v, auto &bbox

  auto const draw_common_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp("silhoutte_black");
    if (!sp.is_2d) {
      draw_entity(rstate, GL_TRIANGLES, sp, eid, eh, transform, is_v, bbox, FORWARD(args));
    }
  };

  auto const draw_orbital_fn = [&](COMMON_ARGS, auto&&... args) {
  };

  auto const draw_pointlight_fn = [&](COMMON_ARGS, auto&&... args) {
    auto& sp = sps.ref_sp(sn.value);

    if (!sp.is_2d) {
      draw_entity(rstate, GL_TRIANGLES, sp, eid, eh, transform, is_v, bbox, FORWARD(args));
    }
  };
#undef COMMON_ARGS

  auto const& draw_torch_fn          = draw_common_fn;
  auto const& draw_junk_fn           = draw_common_fn;
  auto const& draw_textured_junk_fn  = draw_common_fn;

  RENDER_ENTITIES(
  draw_common_fn,
  draw_orbital_fn,
  draw_torch_fn,
  draw_junk_fn,
  draw_textured_junk_fn,
  draw_pointlight_fn,
  COMMON
      );
}

} // namespace opengl

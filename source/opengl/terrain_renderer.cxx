#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>
#include <opengl/terrain_renderer.hpp>

#include <boomhs/engine.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/material.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/zone_state.hpp>

#include <common/algorithm.hpp>
#include <common/log.hpp>

#include <cassert>

using namespace boomhs;
using namespace opengl;
using namespace gl_sdl;

namespace
{

template <typename FN>
void
render_terrain(RenderState& rstate, EntityRegistry& registry, FrameTime const& ft,
               glm::vec4 const& cull_plane, FN const& fn)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto& zs        = fstate.zs;
  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;

  auto& ldata        = zs.level_data;
  auto& terrain_grid = ldata.terrain;

  // backup state to restore after drawing terrain
  PUSH_CW_STATE_UNTIL_END_OF_SCOPE();

  auto const& dimensions = terrain_grid.config.dimensions;

  auto const draw_piece = [&](auto& terrain) {
    auto const& config = terrain.config;
    glFrontFace(terrain_grid.winding);
    if (terrain_grid.culling_enabled) {
      glEnable(GL_CULL_FACE);
      glCullFace(terrain_grid.culling_mode);
    }
    else {
      glDisable(GL_CULL_FACE);
    }

    Transform tr;

    {
      auto const& terrain_pos = terrain.position();

      auto const& zs           = fstate.zs;
      auto const& ldata        = zs.level_data;
      auto const& terrain_grid = ldata.terrain;
      auto const& dimensions   = terrain_grid.config.dimensions;
      tr.translation.x         = terrain_pos.x * dimensions.x;
      tr.translation.z         = terrain_pos.y * dimensions.y;
    }

    fn(terrain, tr);
  };

  LOG_TRACE("-------------------- Draw Terrain BEGIN ----------------------");
  for (auto& t : terrain_grid) {
    draw_piece(t);
  }
  LOG_TRACE("-------------------- Draw Terrain END  ----------------------");
}

} // namespace

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// DefaultTerrainRenderer
void
DefaultTerrainRenderer::render(RenderState& rstate, MaterialTable const& mat_table,
                               EntityRegistry& registry, FrameTime const& ft,
                               glm::vec4 const& cull_plane)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto& zs           = fstate.zs;
  auto& ldata        = zs.level_data;
  auto& ttable       = zs.gfx_state.texture_table;
  auto& terrain_grid = ldata.terrain;

  auto& mat = mat_table.find("terrain");

  auto const& dimensions = terrain_grid.config.dimensions;

  auto const fn = [&](auto& terrain, auto const& tr) {
    auto& sp = terrain.shader();
    sp.while_bound(logger, [&]() {
      auto const& config = terrain.config;
      shader::set_uniform(logger, sp, "u_uvmodifier", config.uv_modifier);
      shader::set_uniform(logger, sp, "u_clipPlane", cull_plane);

      auto& dinfo = terrain.draw_info();

      auto const draw_fn = [&]() {
        dinfo.while_bound(logger, [&]() {
          bool constexpr SET_NORMALMATRIX = true;
          auto const model_matrix         = tr.model_matrix();
          render::draw_3dlit_shape(rstate, GL_TRIANGLE_STRIP, tr.translation, model_matrix, sp,
                                   dinfo, mat, registry, SET_NORMALMATRIX);
        });
      };
      this->while_bound(draw_fn, logger, terrain, ttable);
    });
  };

  render_terrain(rstate, registry, ft, cull_plane, fn);
}

void
DefaultTerrainRenderer::bind_impl(common::Logger& logger, Terrain const& terrain,
                                  opengl::TextureTable& ttable)
{
  auto const bind = [&](size_t const tunit) {
    glActiveTexture(GL_TEXTURE0 + tunit);
    auto& tinfo = *ttable.find(terrain.texture_name(tunit));
    bind::global_bind(logger, tinfo);
  };

  auto const& config         = terrain.config;
  auto const& bound_textures = config.texture_names;
  FOR(i, bound_textures.textures.size()) { bind(i); }
}

void
DefaultTerrainRenderer::unbind_impl(common::Logger& logger, Terrain const& terrain,
                                    opengl::TextureTable& ttable)
{
  auto const unbind = [&](size_t const tunit) {
    auto& tinfo = *ttable.find(terrain.texture_name(tunit));
    bind::global_unbind(logger, tinfo);
  };

  auto const& config         = terrain.config;
  auto const& bound_textures = config.texture_names;
  FOR(i, bound_textures.textures.size()) { unbind(i); }
  glActiveTexture(GL_TEXTURE0);
}

std::string
DefaultTerrainRenderer::to_string() const
{
  return "DefaultTerrainRenderer";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// SilhouetteTerrainRenderer
SilhouetteTerrainRenderer::SilhouetteTerrainRenderer(ShaderProgram& sp)
    : sp_(&sp)
{
}

void
SilhouetteTerrainRenderer::render(RenderState&    rstate, MaterialTable const&,
                                  EntityRegistry& registry, FrameTime const& ft,
                                  glm::vec4 const& cull_plane)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto const fn = [&](auto& terrain, auto const& tr) {
    auto const& config = terrain.config;

    auto& dinfo = terrain.draw_info();
    sp_->while_bound(logger, [&]() {
      dinfo.while_bound(logger, [&]() {
        auto const model_matrix = tr.model_matrix();
        render::draw_3dblack_water(rstate, GL_TRIANGLE_STRIP, model_matrix, *sp_, dinfo);
      });
    });
  };

  render_terrain(rstate, registry, ft, cull_plane, fn);
}

} // namespace opengl

#include <opengl/terrain_renderer.hpp>
#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>

#include <boomhs/heightmap.hpp>
#include <boomhs/state.hpp>
#include <boomhs/terrain.hpp>

#include <stlw/algorithm.hpp>
#include <stlw/log.hpp>

#include <cassert>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

template <typename FN>
void
render_terrain(Transform& transform, RenderState& rstate, EntityRegistry& registry,
               FrameTime const& ft, glm::vec4 const& cull_plane, FN const& fn)
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
  auto&       tr         = transform.translation;

  auto const draw_piece = [&](auto& terrain) {
    {
      auto const& terrain_pos = terrain.position();
      tr.x                    = terrain_pos.x * dimensions.x;
      tr.z                    = terrain_pos.y * dimensions.y;
    }

    auto const& config = terrain.config;
    glFrontFace(terrain_grid.winding);
    if (terrain_grid.culling_enabled) {
      glEnable(GL_CULL_FACE);
      glCullFace(terrain_grid.culling_mode);
    }
    else {
      glDisable(GL_CULL_FACE);
    }

    fn(terrain);
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
DefaultTerrainRenderer::render(RenderState& rstate, EntityRegistry& registry, FrameTime const& ft,
                             glm::vec4 const& cull_plane)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto& zs           = fstate.zs;
  auto& ldata        = zs.level_data;
  auto& ttable       = zs.gfx_state.texture_table;
  auto& terrain_grid = ldata.terrain;

  Transform transform;
  Material  mat;

  auto const& dimensions   = terrain_grid.config.dimensions;
  auto const& model_matrix = transform.model_matrix();

  auto const fn = [&](auto& terrain) {
    auto& sp = terrain.shader();
    sp.while_bound(logger, [&]() {
      auto const& config = terrain.config;
      sp.set_uniform_float1(logger, "u_uvmodifier", config.uv_modifier);
      sp.set_uniform_vec4(logger, "u_clipPlane", cull_plane);

      auto& dinfo = terrain.draw_info();

      auto const draw_fn = [&]() {
        auto& vao = dinfo.vao();
        vao.while_bound(logger, [&]() {
          bool constexpr SET_NORMALMATRIX = true;
          render::draw_3dlit_shape(rstate, GL_TRIANGLE_STRIP, transform.translation, model_matrix,
                                   sp, dinfo, mat, registry, SET_NORMALMATRIX);
        });
      };
      this->while_bound(draw_fn, logger, terrain, ttable);
    });
  };

  render_terrain(transform, rstate, registry, ft, cull_plane, fn);
}

void
DefaultTerrainRenderer::bind_impl(stlw::Logger& logger, Terrain const& terrain,
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
DefaultTerrainRenderer::unbind_impl(stlw::Logger& logger, Terrain const& terrain,
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
// BlackTerrainRenderer
BlackTerrainRenderer::BlackTerrainRenderer(ShaderProgram& sp)
    : sp_(sp)
{
}

void
BlackTerrainRenderer::render(RenderState& rstate, EntityRegistry& registry, FrameTime const& ft,
                             glm::vec4 const& cull_plane)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto& zs        = fstate.zs;
  auto& gfx_state = zs.gfx_state;

  Transform   transform;
  auto const& model_matrix = transform.model_matrix();

  auto const fn = [&](auto& terrain) {
    auto const& config = terrain.config;

    auto& dinfo = terrain.draw_info();
    auto& vao   = dinfo.vao();

    sp_.while_bound(logger, [&]() {
      vao.while_bound(logger, [&]() {
        render::draw_3dblack_water(rstate, GL_TRIANGLE_STRIP, model_matrix, sp_, dinfo);
      });
    });
  };

  render_terrain(transform, rstate, registry, ft, cull_plane, fn);
}

} // namespace opengl

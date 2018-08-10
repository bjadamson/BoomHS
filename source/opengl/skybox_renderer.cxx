#include <opengl/skybox_renderer.hpp>
#include <boomhs/state.hpp>

#include <opengl/constants.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>
#include <boomhs/math.hpp>

#include <cassert>
#include <window/timer.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

///////////////////////////////////////////////////////////////////////////////////////////////////
// SkyboxRenderer
SkyboxRenderer::SkyboxRenderer(stlw::Logger& logger, DrawInfo&& dinfo, TextureInfo& day,
                               TextureInfo& night, ShaderProgram& sp)
    : dinfo_(MOVE(dinfo))
    , day_(&day)
    , night_(&night)
    , sp_(sp)
{
  sp_.while_bound(logger, [&]() {
    sp_.set_uniform_int1(logger, "u_cube_sampler1", 0);
    sp_.set_uniform_int1(logger, "u_cube_sampler1", 1);
  });

  auto const set_fields = [&](auto& ti, GLenum const tunit) {
    glActiveTexture(tunit);
    ON_SCOPE_EXIT([&]() { glActiveTexture(tunit); });
    ti.while_bound(logger, [&]() {
      ti.set_fieldi(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      ti.set_fieldi(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      ti.set_fieldi(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    });
  };
  set_fields(*day_, GL_TEXTURE0);
  set_fields(*night_, GL_TEXTURE1);
}

void
SkyboxRenderer::render(RenderState& rstate, DrawState& ds, FrameTime const& ft)
{
  auto& fstate = rstate.fs;
  auto& zs     = fstate.zs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  if (!es.draw_skybox) {
    return;
  }

  auto const& ldata = zs.level_data;
  auto const& fog   = ldata.fog;

  glActiveTexture(GL_TEXTURE0);
  ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

  BIND_UNTIL_END_OF_SCOPE(logger, *day_);

  glActiveTexture(GL_TEXTURE1);
  BIND_UNTIL_END_OF_SCOPE(logger, *night_);

  // Converting the "current hour" to a value in [0.0, 1.0]
  auto const calculate_blend = [&]() {
    auto const& hour_of_day = static_cast<float>(es.time.hours());
    auto const  frac        = hour_of_day / 24.0f;
    auto        blend       = 1.0f - std::abs(math::squared(-frac) + frac);
    if (blend < 0.0f) {
      blend = -blend;
    }
    return blend;
  };

  // Create a view matrix that has it's translation components zero'd out.
  //
  // The effect of this is the view matrix contains just the rotation, which is what's desired
  // for rendering the skybox.
  auto view_matrix  = fstate.view_matrix();
  view_matrix[3][0] = 0.0f;
  view_matrix[3][1] = 0.0f;
  view_matrix[3][2] = 0.0f;

  auto const proj_matrix   = fstate.projection_matrix();
  auto const camera_matrix = proj_matrix * view_matrix;

  auto const draw_fn = [&]() { render::draw_2d(rstate, GL_TRIANGLES, sp_, dinfo_); };

  sp_.while_bound(logger, [&]() {
    {
      auto const& skybox     = ldata.skybox;
      auto const& transform  = skybox.transform();
      auto const  mvp_matrix = camera_matrix * transform.model_matrix();
      sp_.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);
    }
    sp_.set_uniform_color(logger, "u_fog.color", fog.color);

    auto const blend = calculate_blend();
    sp_.set_uniform_float1(logger, "u_blend_factor", blend);

    auto& vao = dinfo_.vao();
    vao.while_bound(logger, draw_fn);
  });
}

void
SkyboxRenderer::set_day(opengl::TextureInfo* ti)
{
  assert(ti);
  day_ = ti;
}

void
SkyboxRenderer::set_night(opengl::TextureInfo* ti)
{
  assert(ti);
  day_ = ti;
}

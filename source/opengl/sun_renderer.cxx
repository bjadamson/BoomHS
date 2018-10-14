#include <opengl/sun_renderer.hpp>
#include <opengl/factory.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderbuffer.hpp>
#include <opengl/renderer.hpp>

#include <boomhs/engine.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/vertex_interleave.hpp>
#include <boomhs/viewport.hpp>
#include <boomhs/zone_state.hpp>

using namespace boomhs;
using namespace opengl;
using namespace gl_sdl;

namespace
{

void
setup(common::Logger& logger, TextureInfo& ti, GLint const v)
{
  glActiveTexture(v);
  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  });
}

} // namespace

namespace opengl
{

SunshaftBuffers::SunshaftBuffers(common::Logger& logger)
    : fbo(FrameBuffer{opengl::make_fbo(logger)})
    , rbo(RBInfo{})
{
}

SunshaftRenderer::SunshaftRenderer(common::Logger& logger, Viewport const& view_port,
                                   ShaderProgram& sp)
    : buffers_(SunshaftBuffers{logger})
    , sp_(&sp)
{
  auto const w = view_port.width(), h = view_port.height();

  auto& fbo = buffers_.fbo;
  buffers_.tbo = fbo->attach_color_buffer(logger, w, h, GL_TEXTURE1);
  buffers_.rbo = fbo->attach_render_buffer(logger, w, h);

  {
    ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });
    setup(logger, texture_info(), GL_TEXTURE0);
  }

  // connect texture units to shader program
  sp_->while_bound(logger, [&]() { sp_->set_uniform_int1(logger, "u_sampler", 0); });
}

void
SunshaftRenderer::render(RenderState& rstate, DrawState& ds, LevelManager& lm, Camera& camera,
                         FrameTime const& ft)

{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto const& zs                = fstate.zs;
  auto const& ldata             = zs.level_data;
  auto const& global_light      = ldata.global_light;
  auto const& directional_light = global_light.directional;

  auto&    ti    = texture_info();
  auto const v    = VertexFactory::build_default();
  auto const uv   = UvFactory::build_rectangle(ti.uv_max);
  auto const vuvs = vertex_interleave(v, uv);

  DrawInfo dinfo = gpu::copy_rectangle_uvs(logger, sp_->va(), vuvs);

  glm::vec2 const pos{0.00f, 0.00f};
  glm::vec2 const scale{1.00f, 1.00f};

  Transform transform;
  transform.translation = glm::vec3{pos.x, pos.y, 0.0f};
  transform.scale       = glm::vec3{scale.x, scale.y, 1.0};

  auto const model_matrix = transform.model_matrix();
  sp_->while_bound(logger, [&]() {
    render::set_modelmatrix(logger, model_matrix, *sp_);
    sp_->set_uniform_vec2(logger, "u_dirlight.screenspace_pos", directional_light.screenspace_pos);

    glActiveTexture(GL_TEXTURE0);
    dinfo.while_bound(logger, [&]() { render::draw_2d(rstate, GL_TRIANGLES, *sp_, ti, dinfo); });
    // glActiveTexture(GL_TEXTURE0);
  });
}

} // namespace opengl

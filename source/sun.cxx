#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>
#include <boomhs/sun.hpp>
#include <opengl/factory.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderbuffer.hpp>

using namespace opengl;
using namespace window;

namespace
{

void
setup(stlw::Logger& logger, TextureInfo& ti, GLint const v)
{
  glActiveTexture(v);
  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  });
}

} // namespace

namespace boomhs
{

SunshaftBuffers::SunshaftBuffers(stlw::Logger& logger, ScreenSize const& ss)
    : fbo(FrameBuffer{opengl::make_fbo(logger, ss)})
    , rbo(RBInfo{})
{
}

SunshaftRenderer::SunshaftRenderer(stlw::Logger& logger, ScreenSize const& ss, ShaderProgram& sp)
    : buffers_(SunshaftBuffers{logger, ss})
    , sp_(sp)
{
  // TODO: structured binding bug
  auto const size = buffers_.fbo->dimensions.size();
  auto const w = size.first, h = size.second;

  with_sunshaft_fbo(logger, [&]() {
    buffers_.tbo = create_texture_attachment(logger, w, h, GL_TEXTURE1);
    buffers_.rbo = create_depth_buffer_attachment(logger, w, h);
  });

  {
    ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });
    setup(logger, texture_info(), GL_TEXTURE0);
  }

  // connect texture units to shader program
  sp_.while_bound(logger, [&]() { sp_.set_uniform_int1(logger, "u_sampler", 0); });
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

  auto const v = OF::rectangle_vertices();

  auto&    ti    = texture_info();
  DrawInfo dinfo = gpu::copy_rectangle_uvs(logger, sp_.va(), v, ti);

  glm::vec2 const pos{0.00f, 0.00f};
  glm::vec2 const scale{1.00f, 1.00f};

  Transform transform;
  transform.translation = glm::vec3{pos.x, pos.y, 0.0f};
  transform.scale       = glm::vec3{scale.x, scale.y, 1.0};

  auto const model_matrix = transform.model_matrix();
  sp_.while_bound(logger, [&]() {
    render::set_modelmatrix(logger, model_matrix, sp_);
    sp_.set_uniform_vec2(logger, "u_dirlight.screenspace_pos", directional_light.screenspace_pos);

    auto& vao = dinfo.vao();
    glActiveTexture(GL_TEXTURE0);
    vao.while_bound(logger,
                    [&]() { render::draw_2d(rstate, GL_TRIANGLES, sp_, ti, dinfo, false); });
    //glActiveTexture(GL_TEXTURE0);
  });
}

} // namespace boomhs

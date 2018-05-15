#include <boomhs/water_fbos.hpp>
#include <opengl/shader.hpp>
#include <stlw/log.hpp>

#include <cassert>
#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

auto constexpr TEXTURE_RESOLUTION_FACTOR = 4;

auto
make_fbo(stlw::Logger& logger, ScreenSize const& ss)
{
  FBInfo fb{{0, 0, ss.width, ss.height}, ss};
  fb.while_bound(logger, []() { glDrawBuffer(GL_COLOR_ATTACHMENT0); });
  return fb;
}

TextureInfo
create_texture_attachment(stlw::Logger& logger, int const width, int const height)
{
  assert(width > 0 && height > 0);

  TextureInfo ti;
  ti.target = GL_TEXTURE_2D;
  ti.gen_texture(logger, 1);

  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    // attach texture to FBO
    //
    // TODO: I think this code assumes that the FBO is currently bound?
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ti.id, 0);
  });

  ti.height = height;
  ti.width  = width;

  ti.uv_max = 1.0f;
  return ti;
}

auto
create_depth_texture_attachment(stlw::Logger& logger, int const width, int const height)
{
  assert(width > 0 && height > 0);

  GLuint tbo;
  glGenTextures(1, &tbo);
  glBindTexture(GL_TEXTURE_2D, tbo);
  ON_SCOPE_EXIT([]() { glBindTexture(GL_TEXTURE_2D, 0); });

  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT,
               GL_FLOAT, nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // TODO: I think this code assumes that the FBO is currently bound?
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tbo, 0);
  return tbo;
}

auto
create_depth_buffer_attachment(stlw::Logger& logger, int const width, int const height)
{
  RBInfo rbinfo;
  rbinfo.while_bound(logger, [&]() {
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbinfo.id);
  });
  return RenderBuffer{MOVE(rbinfo)};
}

} // namespace

namespace boomhs
{

WaterFrameBuffers::WaterFrameBuffers(stlw::Logger& logger, ScreenSize const& screen_size,
                                     ShaderProgram& sp, TextureInfo& diffuse, TextureInfo& dudv)
    : sp_(sp)
    , diffuse_(diffuse)
    , dudv_(dudv)
    , reflection_fbo_(FrameBuffer{make_fbo(logger, screen_size)})
    , reflection_rbo_(RenderBuffer{RBInfo{}})
    , refraction_fbo_(FrameBuffer{make_fbo(logger, screen_size)})
{

  with_reflection_fbo(logger, [&]() {
    reflection_tbo_ = create_texture_attachment(logger, reflection_fbo_->dimensions.w,
                                                reflection_fbo_->dimensions.h);
  });

  with_refraction_fbo(logger, [&]() {
    refraction_tbo_ = create_texture_attachment(logger, refraction_fbo_->dimensions.w,
                                                refraction_fbo_->dimensions.h);
    refraction_dbo_ = create_depth_texture_attachment(logger, refraction_fbo_->dimensions.w,
                                                      refraction_fbo_->dimensions.h);
  });

  // connect texture units to shader program
  sp_.while_bound(logger, [&]() {
    // Bind texture units to shader uniforms
    sp_.set_uniform_int1(logger, "u_texture_sampler", 0);
    sp_.set_uniform_int1(logger, "u_reflect_sampler", 1);
    sp_.set_uniform_int1(logger, "u_refract_sampler", 2);
    // sp_.set_uniform_int1(logger, "u_dudv_sampler", 3);
  });
}

WaterFrameBuffers::~WaterFrameBuffers() {}

void
WaterFrameBuffers::bind_impl(stlw::Logger& logger)
{
  glActiveTexture(GL_TEXTURE0);
  bind::global_bind(logger, diffuse_);

  glActiveTexture(GL_TEXTURE1);
  bind::global_bind(logger, reflection_tbo_);
  bind::global_bind(logger, reflection_rbo_.resource());

  glActiveTexture(GL_TEXTURE2);
  bind::global_bind(logger, refraction_tbo_);

  // glActiveTexture(GL_TEXTURE3);
  // bind::global_bind(logger, dudv_);
}

void
WaterFrameBuffers::unbind_impl(stlw::Logger& logger)
{
  bind::global_unbind(logger, diffuse_);
  bind::global_unbind(logger, reflection_tbo_);
  bind::global_unbind(logger, reflection_rbo_.resource());
  bind::global_unbind(logger, refraction_tbo_);
  // bind::global_unbind(logger, dudv_);

  glActiveTexture(GL_TEXTURE0);
}

std::string
WaterFrameBuffers::to_string() const
{
  // clang format-off
  return fmt::sprintf("WaterFrameBuffer "
                      "{"
                      "{diffuse: (tbo) %s}, "
                      "{dudv: (tbo) %s}, "
                      "{reflection: (fbo) %s, (tbo) %s, rbo(%s)}, "
                      "{refraction: (fbo) %s, (tbo) %s, dbo(%u)}"
                      "}",
                      diffuse_.to_string(), dudv_.to_string(),

                      reflection_fbo_->to_string(), reflection_tbo_.to_string(),
                      reflection_rbo_->to_string(),

                      refraction_fbo_->to_string(), refraction_tbo_.to_string(), refraction_dbo_);
  // clang format-on
}

} // namespace boomhs

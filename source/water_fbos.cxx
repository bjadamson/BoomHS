#include <boomhs/water_fbos.hpp>
#include <opengl/shader.hpp>
#include <stlw/log.hpp>

#include <cassert>
#include <extlibs/glew.hpp>

using namespace opengl;

namespace
{

int constexpr REFLECTION_WIDTH  = 640;
int constexpr REFLECTION_HEIGHT = 360;

int constexpr REFRACTION_WIDTH  = 1280;
int constexpr REFRACTION_HEIGHT = 720;

int
create_fbo()
{
  GLuint fbo = 0;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // render to color attachment 0
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  return fbo;
}

void
bind_fbo(int const fbo, int const width, int const height)
{
  assert(width > 0 && height > 0);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glViewport(0, 0, width, height);
}

TextureInfo
create_texture_attachment(stlw::Logger& logger, int const width, int const height)
{
  assert(width > 0 && height > 0);

  TextureInfo ti;
  ti.mode = GL_TEXTURE_2D;
  glGenTextures(1, &ti.id);

  while_bound(logger, ti, [&]() {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

int
create_depth_texture_attachment(int width, int height)
{
  assert(width > 0 && height > 0);

  GLuint tbo;
  glGenTextures(1, &tbo);
  glBindTexture(GL_TEXTURE_2D, tbo);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT,
               GL_FLOAT, nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // TODO: I think this code assumes that the FBO is currently bound?
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tbo, 0);
  return tbo;
}

} // namespace

namespace boomhs
{

WaterFrameBuffers::WaterFrameBuffers(stlw::Logger& logger, ShaderProgram& sp)
    : sp_(sp)
{
  {
    reflection_fbo_ = create_fbo();
    reflection_tbo_ = create_texture_attachment(logger, REFLECTION_WIDTH, REFLECTION_HEIGHT);
    reflection_dbo_ = create_depth_texture_attachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
    unbind_all_fbos();
  }
  {
    refraction_fbo_ = create_fbo();
    refraction_tbo_ = create_texture_attachment(logger, REFRACTION_WIDTH, REFRACTION_HEIGHT);
    refraction_dbo_ = create_depth_texture_attachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
    unbind_all_fbos();
  }

  // connect texture units to shader program
  while_bound(logger, sp_, [&]() {
    sp_.set_uniform_int1(logger, "u_reflect_sampler", 0);
    sp_.set_uniform_int1(logger, "u_refract_sampler", 1);
  });
}

WaterFrameBuffers::~WaterFrameBuffers()
{
  // glDeleteFramebuffers(1, &fbo); };

  // GL30.glDeleteFramebuffers(reflection_fbo);
  // GL11.glDeleteTextures(reflection_tbo_);
  // GL30.glDeleteRenderbuffers(reflection_dbo_);
  // GL30.glDeleteFramebuffers(refraction_fbo_);
  // GL11.glDeleteTextures(refraction_tbo_);
  // GL11.glDeleteTextures(refraction_dbo_);
}

void
WaterFrameBuffers::bind_reflection_fbo()
{
  bind_fbo(reflection_fbo_, REFLECTION_WIDTH, REFLECTION_HEIGHT);
}

void
WaterFrameBuffers::bind_refraction_fbo()
{
  bind_fbo(refraction_fbo_, REFRACTION_WIDTH, REFRACTION_HEIGHT);
}

void
WaterFrameBuffers::unbind_all_fbos()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, 1024, 768);
}

TextureInfo const&
WaterFrameBuffers::reflection_ti() const
{
  return reflection_tbo_;
}

TextureInfo const&
WaterFrameBuffers::refraction_ti() const
{
  return refraction_tbo_;
}

int
WaterFrameBuffers::refraction_depth_tid() const
{
  return refraction_dbo_;
}

} // namespace boomhs

#include <opengl/framebuffer.hpp>
#include <opengl/renderer.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

TextureInfo
make_fb_ti(common::Logger& logger, int const width, int const height, GLenum const tu)
{
  assert(width > 0 && height > 0);

  TextureInfo ti;
  ti.target = GL_TEXTURE_2D;
  ti.gen_texture(logger, 1);

  glActiveTexture(tu);
  ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  });
  ti.height = height;
  ti.width  = width;

  ti.uv_max = 1.0f;
  return ti;
}

} // namespace

namespace opengl
{

FBInfo::FBInfo() { glGenFramebuffers(1, &id); }

void
FBInfo::bind_impl(common::Logger& logger)
{
  glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void
FBInfo::unbind_impl(common::Logger& logger)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
FBInfo::destroy_impl()
{
  glDeleteFramebuffers(1, &id);
}

std::string
FBInfo::to_string() const
{
  return fmt::sprintf("(FBInfo) id: %u", id);
}

TextureInfo
FBInfo::attach_color_buffer(common::Logger& logger, int const width, int const height,
                            GLenum const tu)
{
  auto ti = make_fb_ti(logger, width, height, tu);
  ti.while_bound(logger, [&]() {
    // allocate GPU memory for texture
    glTexImage2D(ti.target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  });

  // attach texture to FBO
  while_bound(logger, [&]() {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ti.target, ti.id, 0);
  });
  return ti;
}

TextureInfo
FBInfo::attach_depth_buffer(common::Logger& logger, int const width, int const height,
                            GLenum const tu)
{
  auto ti = make_fb_ti(logger, width, height, tu);
  ti.while_bound(logger, [&]() {
    // allocate GPU memory for texture
    glTexImage2D(ti.target, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 nullptr);
  });
  // attach texture to FBO
  while_bound(logger, [&]() {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ti.target, ti.id, 0);
  });
  return ti;
}

RenderBuffer
FBInfo::attach_render_buffer(common::Logger& logger, int const width, int const height)
{
  RBInfo rbinfo;
  rbinfo.while_bound(
      logger, [&]() { glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height); });

  // attach render buffer to FBO
  while_bound(logger, [&]() {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbinfo.id);
  });
  return RenderBuffer{MOVE(rbinfo)};
}

} // namespace opengl

#include <boomhs/water_fbos.hpp>
#include <opengl/shader.hpp>
#include <stlw/log.hpp>

#include <cassert>
#include <extlibs/glew.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

auto
make_reflection_fbo(stlw::Logger& logger, ScreenSize const& screen_size)
{
  FBInfo fb{{0, 0, 1024, 768}, screen_size};
  fb.while_bound(logger, []() { glDrawBuffer(GL_COLOR_ATTACHMENT0); });
  return fb;
}

auto
make_refraction_fbo(stlw::Logger& logger, ScreenSize const& screen_size)
{
  FBInfo fb{{0, 0, 1024, 768}, screen_size};
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

    ti.set_fieldi(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ti.set_fieldi(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    // attach texture to FBO
    //
    // TODO: I think this code assumes that the FBO is currently bound?
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ti.id(), 0);
  });

  ti.height = height;
  ti.width  = width;

  ti.uv_max = 1.0f;
  return ti;
}

auto
create_depth_texture_attachment(int const width, int const height)
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
create_depth_buffer_attachment(int const width, int const height)
{
  GLuint depth_buffer;
  glGenRenderbuffers(1, &depth_buffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width,
          height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
          GL_RENDERBUFFER, depth_buffer);
  return depth_buffer;
}

} // namespace

namespace boomhs
{

WaterFrameBuffers::WaterFrameBuffers(stlw::Logger& logger, ScreenSize const& screen_size,
                                     ShaderProgram& sp)
    : sp_(sp)
    , reflection_fbo_(FrameBuffer{make_reflection_fbo(logger, screen_size)})
    , refraction_fbo_(FrameBuffer{make_refraction_fbo(logger, screen_size)})
{
  auto const create = [&](auto const& fbo, auto const& depth_function) {
    auto const& dimensions = fbo->dimensions;
    auto const  width = dimensions.w, height = dimensions.h;
    auto const  tbo = create_texture_attachment(logger, width, height);
    auto const  dbo = depth_function(width, height);
    return std::make_pair(tbo, dbo);
  };
  auto const reflection_fn = [&]() {
    auto const reflection = create(reflection_fbo_, create_depth_buffer_attachment);
    reflection_tbo_       = reflection.first;
    reflection_dbo_       = reflection.second;
  };
  auto const refraction_fn = [&]() {
    auto const refraction = create(refraction_fbo_, create_depth_texture_attachment);
    refraction_tbo_       = refraction.first;
    refraction_dbo_       = refraction.second;
  };

  // logic starts here
  with_reflection(logger, reflection_fn);
  with_refraction(logger, refraction_fn);

  // connect texture units to shader program
  sp_.while_bound(logger, [&]() {
    // Bind texture units to shader uniforms
    sp_.set_uniform_int1(logger, "u_reflect_sampler", 0);
    sp_.set_uniform_int1(logger, "u_refract_sampler", 1);
  });
}

WaterFrameBuffers::~WaterFrameBuffers() {}

TextureInfo&
WaterFrameBuffers::reflection_ti()
{
  return reflection_tbo_;
}

TextureInfo&
WaterFrameBuffers::refraction_ti()
{
  return refraction_tbo_;
}

int
WaterFrameBuffers::refraction_depth_tid() const
{
  return refraction_dbo_;
}

} // namespace boomhs

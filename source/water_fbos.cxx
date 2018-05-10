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
make_reflection_fbo()
{
  int constexpr REFLECTION_WIDTH  = 640;
  int constexpr REFLECTION_HEIGHT = 360;
  return FBInfo{{0, 0, 640, 360}};
}

auto
make_refraction_fbo()
{
  return FBInfo{{0, 0, 1270, 720}};
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
    , reflection_fbo_(FrameBuffer{make_reflection_fbo()})
    , refraction_fbo_(FrameBuffer{make_refraction_fbo()})
{
  auto const create = [&](auto const& fbo) {
    auto const& dimensions = fbo->dimensions;
    auto const width = dimensions.w, height = dimensions.h;
    auto const tbo = create_texture_attachment(logger, width, height);
    auto const dbo = create_depth_texture_attachment(width, height);
    return std::make_pair(tbo, dbo);
  };
  auto const reflection_fn = [&]() {
    auto const reflection = create(reflection_fbo_);
    reflection_tbo_       = reflection.first;
    reflection_dbo_       = reflection.second;
  };
  auto const refraction_fn = [&]() {
    auto const refraction = create(refraction_fbo_);
    refraction_tbo_       = refraction.first;
    refraction_dbo_       = refraction.second;
  };

  // logic starts here
  while_bound(logger, reflection_fbo_, reflection_fn);
  while_bound(logger, refraction_fbo_, refraction_fn);

  // connect texture units to shader program
  while_bound(logger, sp_, [&]() {
    sp_.set_uniform_int1(logger, "u_reflect_sampler", 0);
    sp_.set_uniform_int1(logger, "u_refract_sampler", 1);
  });
}

WaterFrameBuffers::~WaterFrameBuffers() {}

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

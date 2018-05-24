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
create_texture_attachment(stlw::Logger& logger, int const width, int const height,
                          GLenum const texture_unit)
{
  assert(width > 0 && height > 0);

  TextureInfo ti;
  ti.target = GL_TEXTURE_2D;
  ti.gen_texture(logger, 1);

  glActiveTexture(texture_unit);
  ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

  ti.while_bound(logger, [&]() {
    // allocate memory for texture
    glTexImage2D(ti.target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // adjust texture fields
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // attach texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ti.target, ti.id, 0);
  });

  ti.height = height;
  ti.width  = width;

  ti.uv_max = 1.0f;
  return ti;
}

auto
create_depth_texture_attachment(stlw::Logger& logger, int const width, int const height,
                                GLenum const texture_unit)
{
  assert(width > 0 && height > 0);

  TextureInfo ti;
  ti.target = GL_TEXTURE_2D;
  ti.gen_texture(logger, 1);

  glActiveTexture(texture_unit);
  ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(ti.target, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 nullptr);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ti.target, ti.id, 0);
  });

  ti.height = height;
  ti.width  = width;

  ti.uv_max = 1.0f;
  return ti;
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReflectionBuffers
ReflectionBuffers::ReflectionBuffers(stlw::Logger& logger, ScreenSize const& ss)
    : fbo(FrameBuffer{make_fbo(logger, ss)})
    , rbo(RBInfo{})
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RefractionBuffers
RefractionBuffers::RefractionBuffers(stlw::Logger& logger, ScreenSize const& ss)
    : fbo(FrameBuffer{make_fbo(logger, ss)})
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// WaterFrameBuffers
WaterFrameBuffers::WaterFrameBuffers(stlw::Logger& logger, ScreenSize const& screen_size,
                                     ShaderProgram& sp, TextureInfo& diffuse, TextureInfo& dudv,
                                     TextureInfo& normal)
    : sp_(sp)
    , diffuse_(diffuse)
    , dudv_(dudv)
    , normal_(normal)
    , reflection_(logger, screen_size)
    , refraction_(logger, screen_size)
{

  {
    // TODO: structured binding bug
    auto const size = reflection_.fbo->dimensions.size();
    auto const w = size.first, h = size.second;

    with_reflection_fbo(logger, [&]() {
      reflection_.tbo = create_texture_attachment(logger, w, h, GL_TEXTURE1);
      reflection_.rbo = create_depth_buffer_attachment(logger, w, h);
    });
  }

  {
    // TODO: structured binding bug
    auto const size = refraction_.fbo->dimensions.size();
    auto const w = size.first, h = size.second;

    with_refraction_fbo(logger, [&]() {
      GLenum const tu = GL_TEXTURE2;
      refraction_.tbo = create_texture_attachment(logger, w, h, tu);
      refraction_.dbo = create_depth_texture_attachment(logger, w, h, tu);
    });
  }

  auto const setup = [&](auto& ti, auto const v) {
    glActiveTexture(v);
    ti.while_bound(logger, [&]() {
      ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    });
  };

  {
    ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });
    setup(diffuse_, GL_TEXTURE0);
    setup(reflection_.tbo, GL_TEXTURE1);
    setup(refraction_.tbo, GL_TEXTURE2);
    setup(dudv_, GL_TEXTURE3);
    setup(normal_, GL_TEXTURE4);
    setup(refraction_.dbo, GL_TEXTURE5);
  }

  // connect texture units to shader program
  sp_.while_bound(logger, [&]() {
    sp_.set_uniform_int1(logger, "u_diffuse_sampler", 0);
    sp_.set_uniform_int1(logger, "u_reflect_sampler", 1);
    sp_.set_uniform_int1(logger, "u_refract_sampler", 2);
    sp_.set_uniform_int1(logger, "u_dudv_sampler", 3);
    sp_.set_uniform_int1(logger, "u_normal_sampler", 4);
    sp_.set_uniform_int1(logger, "u_depth_sampler", 5);
  });
}

WaterFrameBuffers::~WaterFrameBuffers() {}

void
WaterFrameBuffers::bind_impl(stlw::Logger& logger)
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glActiveTexture(GL_TEXTURE0);
  bind::global_bind(logger, diffuse_);

  glActiveTexture(GL_TEXTURE1);
  bind::global_bind(logger, reflection_.tbo);
  bind::global_bind(logger, reflection_.rbo.resource());

  glActiveTexture(GL_TEXTURE2);
  bind::global_bind(logger, refraction_.tbo);

  glActiveTexture(GL_TEXTURE3);
  bind::global_bind(logger, dudv_);

  glActiveTexture(GL_TEXTURE4);
  bind::global_bind(logger, normal_);

  glActiveTexture(GL_TEXTURE5);
  bind::global_bind(logger, refraction_.dbo);
}

void
WaterFrameBuffers::unbind_impl(stlw::Logger& logger)
{
  glDisable(GL_BLEND);

  bind::global_unbind(logger, diffuse_);
  bind::global_unbind(logger, reflection_.tbo);
  bind::global_unbind(logger, reflection_.rbo.resource());
  bind::global_unbind(logger, refraction_.tbo);
  bind::global_unbind(logger, dudv_);
  bind::global_unbind(logger, normal_);
  bind::global_unbind(logger, refraction_.dbo);

  glActiveTexture(GL_TEXTURE0);
}

std::string
WaterFrameBuffers::to_string() const
{
  // clang format-off
  return fmt::sprintf(
      "WaterFrameBuffer "
      "{"
      "{diffuse: (tbo) %s}, "
      "{dudv: (tbo) %s}, "
      "{normal: (tbo) %s}, "
      //"{depth: (tbo) %s}, "
      "{reflection: (fbo) %s, (tbo) %s, rbo(%s)}, "
      "{refraction: (fbo) %s, (tbo) %s, dbo(%s)}"
      "}",
      diffuse_.to_string(), dudv_.to_string(), normal_.to_string(),

      reflection_.fbo->to_string(), reflection_.tbo.to_string(), reflection_.rbo->to_string(),

      refraction_.fbo->to_string(), refraction_.tbo.to_string(), refraction_.dbo.to_string());
  // clang format-on
}

} // namespace boomhs

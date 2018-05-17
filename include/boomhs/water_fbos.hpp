#pragma once
#include <boomhs/dimensions.hpp>
#include <opengl/bind.hpp>
#include <opengl/framebuffer.hpp>
#include <opengl/renderbuffer.hpp>
#include <opengl/texture.hpp>
#include <stlw/log.hpp>

namespace opengl
{
class ShaderProgram;
} // namespace opengl

namespace boomhs
{

struct ReflectionBuffers
{
  opengl::FrameBuffer  fbo;
  opengl::TextureInfo  tbo;
  opengl::RenderBuffer rbo;

  ReflectionBuffers(stlw::Logger&, ScreenSize const&);
};

struct RefractionBuffers
{
  opengl::FrameBuffer fbo;
  opengl::TextureInfo tbo;
  opengl::TextureInfo dbo;

  RefractionBuffers(stlw::Logger&, ScreenSize const&);
};

class WaterFrameBuffers
{
  opengl::ShaderProgram& sp_;
  opengl::TextureInfo&   diffuse_;
  opengl::TextureInfo&   dudv_;
  opengl::TextureInfo&   normal_;

  ReflectionBuffers reflection_;
  RefractionBuffers refraction_;

public:
  WaterFrameBuffers(stlw::Logger&, ScreenSize const&, opengl::ShaderProgram&, opengl::TextureInfo&,
                    opengl::TextureInfo&, opengl::TextureInfo&);
  ~WaterFrameBuffers();

  // public members
  opengl::DebugBoundCheck debug_check;

  void bind_impl(stlw::Logger&);
  void unbind_impl(stlw::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  auto& refr() { return refraction_.dbo; }

  template <typename FN>
  void with_reflection_fbo(stlw::Logger& logger, FN const& fn)
  {
    reflection_.fbo->while_bound(logger, fn);
  }

  template <typename FN>
  void with_refraction_fbo(stlw::Logger& logger, FN const& fn)
  {
    refraction_.fbo->while_bound(logger, fn);
  }

  std::string to_string() const;
};

} // namespace boomhs

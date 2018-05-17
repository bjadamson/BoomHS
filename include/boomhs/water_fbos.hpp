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

class WaterFrameBuffers
{
  opengl::ShaderProgram& sp_;
  opengl::TextureInfo&   diffuse_;
  opengl::TextureInfo&   dudv_;
  opengl::TextureInfo&   normal_;

  opengl::FrameBuffer  reflection_fbo_;
  opengl::TextureInfo  reflection_tbo_;
  opengl::RenderBuffer reflection_rbo_;

  opengl::FrameBuffer refraction_fbo_;
  opengl::TextureInfo refraction_tbo_;
  opengl::TextureInfo refraction_dbo_;

public:
  WaterFrameBuffers(stlw::Logger&, ScreenSize const&, opengl::ShaderProgram&, opengl::TextureInfo&,
                    opengl::TextureInfo&, opengl::TextureInfo&);
  ~WaterFrameBuffers();

  // public members
  opengl::DebugBoundCheck debug_check;

  void bind_impl(stlw::Logger&);
  void unbind_impl(stlw::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  template <typename FN>
  void with_reflection_fbo(stlw::Logger& logger, FN const& fn)
  {
    reflection_fbo_->while_bound(logger, fn);
  }

  template <typename FN>
  void with_refraction_fbo(stlw::Logger& logger, FN const& fn)
  {
    refraction_fbo_->while_bound(logger, fn);
  }

  std::string to_string() const;
};

} // namespace boomhs

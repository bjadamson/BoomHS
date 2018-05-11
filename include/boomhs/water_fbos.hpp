#pragma once
#include <boomhs/dimensions.hpp>
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

  opengl::FrameBuffer reflection_fbo_;
  opengl::TextureInfo reflection_tbo_;
  int                 reflection_dbo_;

  opengl::FrameBuffer refraction_fbo_;
  opengl::TextureInfo refraction_tbo_;
  int                 refraction_dbo_;

public:
  WaterFrameBuffers(stlw::Logger&, ScreenSize const&, opengl::ShaderProgram&);
  ~WaterFrameBuffers();

  template <typename FN>
  void with_reflection(stlw::Logger& logger, FN const& fn)
  {
    while_bound(logger, reflection_fbo_, fn);
  }

  template <typename FN>
  void with_refraction(stlw::Logger& logger, FN const& fn)
  {
    while_bound(logger, refraction_fbo_, fn);
  }

  opengl::TextureInfo& reflection_ti();
  opengl::TextureInfo& refraction_ti();
  int                        refraction_depth_tid() const;
};

} // namespace boomhs

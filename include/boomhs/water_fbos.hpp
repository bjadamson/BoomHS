#pragma once
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
  int                    reflection_fbo_;
  opengl::TextureInfo    reflection_tbo_;
  int                    reflection_dbo_;

  int                 refraction_fbo_;
  opengl::TextureInfo refraction_tbo_;
  int                 refraction_dbo_;

public:
  WaterFrameBuffers(stlw::Logger&, opengl::ShaderProgram&);

  ~WaterFrameBuffers();

  void bind_reflection_fbo();
  void bind_refraction_fbo();
  void unbind_all_fbos();

  opengl::TextureInfo const& reflection_ti() const;
  opengl::TextureInfo const& refraction_ti() const;
  int                        refraction_depth_tid() const;
};

} // namespace boomhs

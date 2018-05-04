#pragma once
#include <opengl/texture.hpp>

namespace boomhs
{

class WaterFrameBuffers
{
  static int const REFLECTION_WIDTH;
  static int const REFLECTION_HEIGHT;

  static int const REFRACTION_WIDTH;
  static int const REFRACTION_HEIGHT;

  int                 reflection_fbo_;
  opengl::TextureInfo reflection_tbo_;
  int                 reflection_dbo_;

  int                 refraction_fbo_;
  opengl::TextureInfo refraction_tbo_;
  int                 refraction_dbo_;

public:
  WaterFrameBuffers();

  ~WaterFrameBuffers();

  void bind_reflection_fbo();
  void bind_refraction_fbo();
  void unbind_all_fbos();

  opengl::TextureInfo const& reflection_ti() const;
  opengl::TextureInfo const& refraction_ti() const;
  int refraction_depth_tid() const;
};

} // ns boomhs

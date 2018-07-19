#pragma once
#include <boomhs/dimensions.hpp>

#include <opengl/framebuffer.hpp>
#include <opengl/renderbuffer.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{
class Camera;
class DrawState;
class LevelManager;
class RenderState;

struct SunshaftBuffers
{
  opengl::FrameBuffer  fbo;
  opengl::TextureInfo  tbo;
  opengl::RenderBuffer rbo;

  SunshaftBuffers(stlw::Logger&, ScreenSize const&);

  NO_COPY(SunshaftBuffers);
  MOVE_DEFAULT(SunshaftBuffers);
};

class SunshaftRenderer
{
  opengl::ShaderProgram& sp_;
  SunshaftBuffers        buffers_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(SunshaftRenderer);

  explicit SunshaftRenderer(stlw::Logger&, ScreenSize const&, opengl::ShaderProgram&);

  auto& texture_info() { return buffers_.tbo; }

  template <typename FN>
  void with_sunshaft_fbo(stlw::Logger& logger, FN const& fn)
  {
    buffers_.fbo->while_bound(logger, fn);
  }

  void render(RenderState&, DrawState&, LevelManager&, Camera&, window::FrameTime const&);
};

} // namespace boomhs

#pragma once
#include <boomhs/screen_size.hpp>

#include <opengl/framebuffer.hpp>
#include <opengl/renderbuffer.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <common/log.hpp>
#include <common/type_macros.hpp>

namespace boomhs
{
class Camera;
class LevelManager;
} // namespace boomhs

namespace window
{
class FrameTime;
} // namespace window

namespace opengl
{
class DrawState;
class RenderState;

struct SunshaftBuffers
{
  opengl::FrameBuffer  fbo;
  opengl::TextureInfo  tbo;
  opengl::RenderBuffer rbo;

  SunshaftBuffers(common::Logger&, boomhs::ScreenSize const&);

  NO_COPY(SunshaftBuffers);
  MOVE_DEFAULT(SunshaftBuffers);
};

class SunshaftRenderer
{
  opengl::ShaderProgram& sp_;
  SunshaftBuffers        buffers_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(SunshaftRenderer);

  explicit SunshaftRenderer(common::Logger&, boomhs::ScreenSize const&, opengl::ShaderProgram&);

  auto& texture_info() { return buffers_.tbo; }

  template <typename FN>
  void with_sunshaft_fbo(common::Logger& logger, FN const& fn)
  {
    buffers_.fbo->while_bound(logger, fn);
  }

  void render(RenderState&, DrawState&, boomhs::LevelManager&, boomhs::Camera&,
              window::FrameTime const&);
};

} // namespace opengl

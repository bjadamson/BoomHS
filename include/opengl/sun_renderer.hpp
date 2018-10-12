#pragma once
#include <boomhs/viewport.hpp>

#include <opengl/framebuffer.hpp>
#include <opengl/renderbuffer.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <common/log.hpp>
#include <common/type_macros.hpp>

namespace boomhs
{
class Camera;
class FrameTime;
class LevelManager;
} // namespace boomhs

namespace opengl
{
class DrawState;
class RenderState;

struct SunshaftBuffers
{
  opengl::FrameBuffer  fbo;
  opengl::TextureInfo  tbo;
  opengl::RenderBuffer rbo;

  SunshaftBuffers(common::Logger&);

  NOCOPY_MOVE_DEFAULT(SunshaftBuffers);
};

class SunshaftRenderer
{
  opengl::ShaderProgram* sp_;
  SunshaftBuffers        buffers_;

public:
  NOCOPY_MOVE_DEFAULT(SunshaftRenderer);

  explicit SunshaftRenderer(common::Logger&, boomhs::Viewport const&, opengl::ShaderProgram&);

  auto& texture_info() { return buffers_.tbo; }

  template <typename FN>
  void with_sunshaft_fbo(common::Logger& logger, FN const& fn)
  {
    buffers_.fbo->while_bound(logger, fn);
  }

  void render(RenderState&, DrawState&, boomhs::LevelManager&, boomhs::Camera&,
              boomhs::FrameTime const&);
};

} // namespace opengl

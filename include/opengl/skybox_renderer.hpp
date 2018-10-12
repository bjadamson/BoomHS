#pragma once
#include <common/log.hpp>
#include <common/type_macros.hpp>
#include <opengl/draw_info.hpp>

namespace boomhs
{
class FrameTime;
} // namespace boomhs

namespace opengl
{
struct RenderState;
struct DrawState;
class ShaderProgram;
struct TextureInfo;

class SkyboxRenderer
{
  opengl::DrawInfo dinfo_;

  opengl::TextureInfo*   day_;
  opengl::TextureInfo*   night_;
  opengl::ShaderProgram* sp_;

public:
  NOCOPY_MOVE_DEFAULT(SkyboxRenderer);
  SkyboxRenderer(common::Logger&, opengl::DrawInfo&&, opengl::TextureInfo&, opengl::TextureInfo&,
                 opengl::ShaderProgram&);

  void render(RenderState&, DrawState&, boomhs::FrameTime const&);
  void set_day(opengl::TextureInfo*);
  void set_night(opengl::TextureInfo*);
};

} // namespace opengl

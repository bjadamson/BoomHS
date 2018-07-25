#pragma once
#include <opengl/draw_info.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{
struct RenderState;
struct DrawState;
} // namespace boomhs

namespace window
{
class FrameTime;
} // namespace window

namespace opengl
{
class ShaderProgram;
struct TextureInfo;

class SkyboxRenderer
{
  opengl::DrawInfo dinfo_;

  opengl::TextureInfo*   day_;
  opengl::TextureInfo*   night_;
  opengl::ShaderProgram& sp_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(SkyboxRenderer);
  SkyboxRenderer(stlw::Logger&, opengl::DrawInfo&&, opengl::TextureInfo&, opengl::TextureInfo&,
                 opengl::ShaderProgram&);

  void render(boomhs::RenderState&, boomhs::DrawState&, window::FrameTime const&);
  void set_day(opengl::TextureInfo*);
  void set_night(opengl::TextureInfo*);
};

} // namespace opengl

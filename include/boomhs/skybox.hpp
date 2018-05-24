#pragma once
#include <boomhs/types.hpp>
#include <opengl/draw_info.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{
class ShaderProgram;
struct TextureInfo;
} // namespace opengl

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{
class RenderState;

class Skybox
{
  Transform transform_;
  float     speed_;

public:
  Skybox();
  MOVE_CONSTRUCTIBLE_ONLY(Skybox);
  void update(window::FrameTime const&);

  auto const& transform() const { return transform_; }
  auto&       transform() { return transform_; }
};

class SkyboxRenderer
{
  opengl::DrawInfo dinfo_;

  opengl::TextureInfo&   day_;
  opengl::TextureInfo&   night_;
  opengl::ShaderProgram& sp_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(SkyboxRenderer);
  SkyboxRenderer(stlw::Logger&, opengl::DrawInfo&&, opengl::TextureInfo&, opengl::TextureInfo&,
                 opengl::ShaderProgram&);

  void render(RenderState&, window::FrameTime const&);
};

} // namespace boomhs

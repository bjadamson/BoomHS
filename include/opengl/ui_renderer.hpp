#pragma once
#include <boomhs/color.hpp>
#include <boomhs/math.hpp>
#include <boomhs/viewport.hpp>
#include <common/log.hpp>

// TODO: this header is only included for Color2DProgram
#include <opengl/renderer.hpp>
#include <opengl/types.hpp>

namespace boomhs
{
class AspectRatio;
} // namespace boomhs

namespace opengl
{
class  DrawInfo;
struct DrawState;
class  ShaderProgram;

class UiRenderer
{
  Color2DProgram  color_2dsp_;
  ShaderProgram&  lineloop_sp_;

  glm::mat4       pm_;

  void draw_color_rect_impl(common::Logger&, boomhs::ModelViewMatrix const&, DrawInfo&,
                       boomhs::Color const&, GLenum, DrawState&);

  UiRenderer(Color2DProgram&&, ShaderProgram&, boomhs::Viewport const&, boomhs::AspectRatio const&);

  NO_MOVE_OR_COPY(UiRenderer);
public:

  void resize(boomhs::Viewport const&, boomhs::AspectRatio const&);

  void draw_color_rect(common::Logger&, boomhs::ModelMatrix const&, DrawInfo&, boomhs::Color const&, GLenum, DrawState&);
  void draw_color_rect(common::Logger&, DrawInfo&, boomhs::Color const&, GLenum, DrawState&);

  template <typename ...Args>
  static auto
  create(common::Logger& logger, ShaderProgram& line_loop, Args&&... args)
  {
    auto color2d_program = Color2DProgram::create(logger);
    return UiRenderer{MOVE(color2d_program), line_loop, FORWARD(args)};
  }
};

} // namespace opengl

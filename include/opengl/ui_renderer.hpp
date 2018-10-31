#pragma once
#include <boomhs/color.hpp>
#include <boomhs/math.hpp>
#include <boomhs/viewport.hpp>
#include <common/log.hpp>

// TODO: this header is only included for static_shaders::BasicMvWithUniformColor
#include <opengl/draw_info.hpp>
#include <opengl/shader.hpp>
#include <opengl/types.hpp>

namespace boomhs
{
class AspectRatio;
} // namespace boomhs

namespace opengl
{
class  DrawInfo;
struct DrawState;
//class  ShaderProgram;

namespace static_shaders
{

class BasicMvWithUniformColor
{
  ShaderProgram sp_;

  BasicMvWithUniformColor(ShaderProgram&& sp) : sp_(MOVE(sp)) {}
public:

  auto& sp() { return sp_; }

  static BasicMvWithUniformColor create(common::Logger&);
};

} // namespace static_shaders

struct ProgramAndDrawInfo
{
  static_shaders::BasicMvWithUniformColor program;
  DrawInfo dinfo;

  // ctor
  ProgramAndDrawInfo(static_shaders::BasicMvWithUniformColor&&, DrawInfo&&);

  static ProgramAndDrawInfo create_rect(common::Logger&, boomhs::RectFloat const&,
                                        static_shaders::BasicMvWithUniformColor&&, GLenum);
  static ProgramAndDrawInfo create_rect(common::Logger&, boomhs::RectFloat const&, GLenum);
};
using MouseRectangle = ProgramAndDrawInfo;

class UiRenderer
{
  static_shaders::BasicMvWithUniformColor  color_2dsp_;

  glm::mat4       pm_;

  void draw_rect_impl(common::Logger&, boomhs::ModelViewMatrix const&, DrawInfo&,
                       boomhs::Color const&, GLenum, ShaderProgram&, DrawState&);

  UiRenderer(static_shaders::BasicMvWithUniformColor&&, boomhs::Viewport const&,
             boomhs::AspectRatio const&);

  NO_MOVE_OR_COPY(UiRenderer);
public:

  void resize(boomhs::Viewport const&, boomhs::AspectRatio const&);

  void draw_rect(common::Logger&, boomhs::ModelMatrix const&, DrawInfo&, boomhs::Color const&,
                       GLenum, DrawState&);
  void draw_rect(common::Logger&, DrawInfo&, boomhs::Color const&, GLenum, DrawState&);

  template <typename ...Args>
  static auto
  create(common::Logger& logger, Args&&... args)
  {
    auto program = static_shaders::BasicMvWithUniformColor::create(logger);
    return UiRenderer{MOVE(program), FORWARD(args)};
  }
};

} // namespace opengl

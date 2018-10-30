#include <opengl/ui_renderer.hpp>
#include <opengl/bind.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>

#include <boomhs/camera.hpp>

using namespace boomhs;
using namespace boomhs::math;

namespace opengl
{

UiRenderer::UiRenderer(Color2DProgram&& color_2dsp, ShaderProgram& line_loop, Viewport const& vp,
                       AspectRatio const& ar)
    : color_2dsp_(MOVE(color_2dsp))
    , lineloop_sp_(line_loop)
{
  resize(vp, ar);
}

void
UiRenderer::resize(Viewport const& vp, AspectRatio const& ar)
{
  int constexpr NEAR_PM = -1.0;
  int constexpr FAR_PM  = 1.0f;
  Frustum const fr{
    vp.left(),
    vp.right(),
    vp.bottom(),
    vp.top(),
    NEAR_PM,
    FAR_PM
  };

  auto constexpr ZOOM = glm::ivec2{0};
  pm_ = CameraORTHO::compute_pm(ar, fr, vp.size(), constants::ZERO, ZOOM);
}

void
UiRenderer::draw_color_rect_impl(common::Logger& logger, ModelViewMatrix const& mv, DrawInfo& di,
                            Color const& color, GLenum const dm, DrawState& ds)
{
  auto& sp = this->color_2dsp_.sp();
  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  shader::set_uniform(logger, sp, "u_mv", mv);
  shader::set_uniform(logger, sp, "u_color", color);

  BIND_UNTIL_END_OF_SCOPE(logger, di);
  OR::draw_2delements(logger, dm, sp, di.num_indices());
}

void
UiRenderer::draw_color_rect(common::Logger& logger, DrawInfo& di, Color const& color,
                            GLenum const dm, DrawState& ds)
{
  auto const& mv = pm_;
  draw_color_rect_impl(logger, mv, di, color, dm, ds);
}

void
UiRenderer::draw_color_rect(common::Logger& logger, ModelMatrix const& mmatrix, DrawInfo& di,
                            Color const& color, GLenum const dm, DrawState& ds)
{
  auto const mv = pm_ * mmatrix;
  draw_color_rect_impl(logger, mv, di, color, dm, ds);
}

} // namespace opengl

#include <opengl/ui_renderer.hpp>
#include <opengl/bind.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/shape.hpp>

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
UiRenderer::draw_rect_impl(common::Logger& logger, ModelViewMatrix const& mv, DrawInfo& di,
                           Color const& color, GLenum const dm, ShaderProgram& sp, DrawState& ds)
{
  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  shader::set_uniform(logger, sp, "u_mv", mv);
  shader::set_uniform(logger, sp, "u_color", color);

  BIND_UNTIL_END_OF_SCOPE(logger, di);
  OR::draw_2delements(logger, dm, sp, di.num_indices());
}

void
UiRenderer::draw_color_impl(common::Logger& logger, ModelViewMatrix const& mv, DrawInfo& di,
                            Color const& color, DrawState& ds)
{
  draw_rect_impl(logger, mv, di, color, GL_TRIANGLES, color_2dsp_.sp(), ds);
}

void
UiRenderer::draw_color_rect(common::Logger& logger, DrawInfo& di, Color const& color, DrawState& ds)
{
  auto const& mv = pm_;
  draw_color_impl(logger, mv, di, color, ds);
}

void
UiRenderer::draw_color_rect(common::Logger& logger, ModelMatrix const& mmatrix, DrawInfo& di,
                            Color const& color, DrawState& ds)
{
  auto const mv = pm_ * mmatrix;
  draw_color_impl(logger, mv, di, color, ds);
}

void
UiRenderer::draw_line_rect(common::Logger& logger, DrawInfo& di, Color const& color, DrawState& ds)
{
  draw_rect_impl(logger, pm_, di, color, GL_LINE_LOOP, lineloop_sp_, ds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Color2DProgram
Color2DProgram
Color2DProgram::create(common::Logger& logger)
{
  constexpr auto vert_source = GLSL_SOURCE(R"GLSL(
in vec3 a_position;

uniform mat4 u_mv;
uniform vec4 u_color;

out vec4 v_color;

void main()
{
  gl_Position = u_mv * vec4(a_position, 1.0);
  v_color = u_color;
}
)GLSL");

  static auto frag_source = GLSL_SOURCE(R"GLSL(
in vec4 v_color;

out vec4 fragment_color;

void main()
{
  fragment_color = v_color;
}
)GLSL");

  auto api = AttributePointerInfo{0, GL_FLOAT, AttributeType::POSITION, 3};
  auto sp  =  make_shaderprogram_expect(logger, vert_source, frag_source, MOVE(api));
  return Color2DProgram{MOVE(sp)};
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Color2DRect
Color2DRect::Color2DRect(Color2DProgram&& p, DrawInfo&& dinfo)
    : program_(MOVE(p))
    , dinfo_(MOVE(dinfo))
{
}

Color2DRect
Color2DRect::create(common::Logger& logger, RectFloat const& rect, Color2DProgram&& program)
{
  auto builder   = RectBuilder{rect};
  builder.line   = {};
  auto const& va = program.sp().va();
  auto dinfo     = OG::copy_rectangle(logger, va, builder.build());

  return Color2DRect{MOVE(program), MOVE(dinfo)};
}

Color2DRect
Color2DRect::create(common::Logger& logger, RectFloat const& rect)
{
  auto program = Color2DProgram::create(logger);
  return create(logger, rect, MOVE(program));
}

} // namespace opengl

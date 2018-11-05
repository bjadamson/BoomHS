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

UiRenderer::UiRenderer(static_shaders::BasicMvWithUniformColor&& sp2d, Viewport const& vp)
  : sp2d_(MOVE(sp2d))
{
  resize(vp);
}

void
UiRenderer::resize(Viewport const& vp)
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
  pm_ = CameraORTHO::compute_pm(fr, vp.size(), constants::ZERO, ZOOM);
}

void
UiRenderer::draw_rect_impl(common::Logger& logger, ModelViewMatrix const& mv, DrawInfo& di,
                           Color const& color, GLenum const dm, ShaderProgram& sp, DrawState& ds)
{
  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  shader::set_uniform(logger, sp, "u_mv", mv);
  shader::set_uniform(logger, sp, "u_color", color);

  BIND_UNTIL_END_OF_SCOPE(logger, di);
  OR::draw_2delements(logger, dm, sp, di.num_indices(), ds);
}

void
UiRenderer::draw_rect(common::Logger& logger, DrawInfo& di, Color const& color, GLenum const dm, DrawState& ds)
{
  auto const& mv = pm_;
  draw_rect_impl(logger, mv, di, color, dm, sp2d_.sp(), ds);
}

void
UiRenderer::draw_rect(common::Logger& logger, ModelMatrix const& mmatrix, DrawInfo& di,
                            Color const& color, GLenum const dm, DrawState& ds)
{
  auto const mv = pm_ * mmatrix;
  draw_rect_impl(logger, mv, di, color, dm, sp2d_.sp(), ds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// static_shaders::BasicMvWithUniformColor
static_shaders::BasicMvWithUniformColor
static_shaders::BasicMvWithUniformColor::create(common::Logger& logger)
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
  return static_shaders::BasicMvWithUniformColor{MOVE(sp)};
}

///////////////////////////////////////////////////////////////////////////////////////////////
// ProgramAndDrawInfo
ProgramAndDrawInfo::ProgramAndDrawInfo(static_shaders::BasicMvWithUniformColor&& p, DrawInfo&& di)
    : program(MOVE(p))
    , dinfo(MOVE(di))
{
}

ProgramAndDrawInfo
ProgramAndDrawInfo::create_rect(common::Logger& logger, RectFloat const& rect,
                                static_shaders::BasicMvWithUniformColor&& program,
                                GLenum const dm)
{
  auto builder      = RectBuilder{rect};
  builder.draw_mode = dm;
  auto const& va    = program.sp().va();
  auto dinfo        = OG::copy_rectangle(logger, va, builder.build());

  return ProgramAndDrawInfo{MOVE(program), MOVE(dinfo)};
}

ProgramAndDrawInfo
ProgramAndDrawInfo::create_rect(common::Logger& logger, RectFloat const& rect, GLenum const dm)
{
  auto program = static_shaders::BasicMvWithUniformColor::create(logger);
  return create_rect(logger, rect, MOVE(program), dm);
}

} // namespace opengl

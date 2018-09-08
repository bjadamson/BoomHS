#pragma once
#include <boomhs/frame.hpp>
#include <boomhs/terrain.hpp>

#include <boomhs/lighting.hpp>
#include <common/log.hpp>
#include <boomhs/colors.hpp>

#include <extlibs/glm.hpp>
#include <string>
#include <vector>

namespace boomhs
{
struct ScreenDimensions;
class EntityRegistry;
class FrameTime;
class Frustum;
class HandleManager;
class LevelManager;
class Material;
struct EngineState;
class Player;
struct Transform;
struct ZoneState;
} // namespace boomhs

namespace opengl
{
class DrawInfo;
class ShaderProgram;
class ShaderPrograms;
struct TextureInfo;

struct DrawState
{
  size_t num_vertices;
  size_t num_drawcalls;

  bool const draw_wireframes;

  DrawState(bool);

  std::string to_string() const;
};

struct RenderState
{
  boomhs::FrameState& fs;
  DrawState&          ds;

  explicit RenderState(boomhs::FrameState& f, DrawState& d)
      : fs(f)
      , ds(d)
  {
  }
};

struct WindingState
{
  GLint state;
};
struct CullingState
{
  GLboolean enabled;
  GLint     mode;
};

// CW => CullingWinding
struct CWState
{
  WindingState winding;
  CullingState culling;
};

struct BlendState
{
  GLboolean enabled;
  GLint     source_alpha, dest_alpha;
  GLint     source_rgb, dest_rgb;
};

} // namespace opengl

#define PUSH_CW_STATE_UNTIL_END_OF_SCOPE()                                                         \
  auto const cw_state = render::read_cwstate();                                                    \
  ON_SCOPE_EXIT([&]() { render::set_cwstate(cw_state); });

#define PUSH_BLEND_STATE_UNTIL_END_OF_SCOPE()                                                      \
  auto const blend_state = render::read_blendstate();                                              \
  ON_SCOPE_EXIT([&]() { render::set_blendstate(blend_state); });

#define ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT()                                                   \
  PUSH_BLEND_STATE_UNTIL_END_OF_SCOPE();                                                           \
  glEnable(GL_BLEND);                                                                              \
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#define ENABLE_ADDITIVE_BLENDING_UNTIL_SCOPE_EXIT()                                                \
  PUSH_BLEND_STATE_UNTIL_END_OF_SCOPE();                                                           \
  glEnable(GL_BLEND);                                                                              \
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

namespace opengl::render
{

CWState
read_cwstate();

void
set_cwstate(CWState const&);

BlendState
read_blendstate();

void
set_blendstate(BlendState const&);

void
init(common::Logger&);

void
clear_screen(boomhs::Color const&);

// TODO: keep these extract rest to sub-renderers
void
draw_2d(RenderState&, GLenum, ShaderProgram&, DrawInfo&);

void
draw_2d(RenderState&, GLenum, ShaderProgram&, TextureInfo&, DrawInfo&);

void
draw_3dlightsource(RenderState&, GLenum, glm::mat4 const&, ShaderProgram&, DrawInfo&,
                   boomhs::EntityID, boomhs::EntityRegistry&);

void
draw_3dshape(RenderState&, GLenum, glm::mat4 const&, ShaderProgram&, DrawInfo&);

void
draw_3dblack_water(RenderState&, GLenum, glm::mat4 const&, ShaderProgram&, DrawInfo&);

void
draw_3dlit_shape(RenderState&, GLenum, glm::vec3 const&, glm::mat4 const&, ShaderProgram&,
                 DrawInfo&, boomhs::Material const&, boomhs::EntityRegistry&, bool);

// TODO: move rest to sub-renderers or something
void
draw(common::Logger&, DrawState&, GLenum, ShaderProgram&, DrawInfo&);

//////////
// Direct drawing API
void
draw_2delements(common::Logger&, GLenum, ShaderProgram&, GLuint);

void
draw_2delements(common::Logger&, GLenum, ShaderProgram&, TextureInfo&, GLuint);

void
draw_elements(common::Logger&, GLenum, ShaderProgram&, GLuint);

//////////

void
draw_arrow(RenderState&, glm::vec3 const&, glm::vec3 const&, boomhs::Color const&);

void
draw_fbo_testwindow(RenderState&, glm::vec2 const&, glm::vec2 const&, ShaderProgram&, TextureInfo&);

void
draw_line(RenderState&, glm::vec3 const&, glm::vec3 const&, boomhs::Color const&);

void
draw_targetreticle(RenderState&, boomhs::FrameTime const&);

void
draw_grid_lines(RenderState&);

void
set_modelmatrix(common::Logger&, glm::mat4 const&, ShaderProgram&);

void
set_mvpmatrix(common::Logger&, glm::mat4 const&, glm::mat4 const&, ShaderProgram&);

void
set_viewport(boomhs::ScreenDimensions const&);

} // namespace opengl::render

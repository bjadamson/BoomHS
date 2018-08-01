#pragma once
#include <boomhs/components.hpp>
#include <boomhs/terrain.hpp>

#include <opengl/colors.hpp>
#include <opengl/frame.hpp>
#include <opengl/lighting.hpp>
#include <stlw/log.hpp>

#include <extlibs/glm.hpp>
#include <vector>

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{
class Camera;
struct Dimensions;
class EntityRegistry;
class HandleManager;
class LevelManager;
class Material;
struct EngineState;
struct Player;
struct Transform;
struct ZoneState;
} // namespace boomhs

namespace stlw
{
class float_generator;
} // namespace stlw

namespace opengl
{
class DrawInfo;
class ShaderProgram;
class ShaderPrograms;
struct TextureInfo;

struct RenderState
{
  FrameState& fs;
  DrawState&  ds;

  explicit RenderState(FrameState& f, DrawState& d)
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
init(stlw::Logger&, boomhs::Dimensions const&);

void
clear_screen(Color const&);

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
conditionally_draw_player_vectors(RenderState&, boomhs::Player const&);

void
draw(RenderState&, GLenum, ShaderProgram&, DrawInfo&);

void
draw_arrow(RenderState&, glm::vec3 const&, glm::vec3 const&, Color const&);

void
draw_fbo_testwindow(RenderState&, glm::vec2 const&, glm::vec2 const&, TextureInfo&);

void
draw_global_axis(RenderState&);

void
draw_inventory_overlay(RenderState&);

void
draw_local_axis(RenderState&, glm::vec3 const&);

void
draw_targetreticle(RenderState&, window::FrameTime const&);

void
draw_grid_lines(RenderState&);

void
set_modelmatrix(stlw::Logger&, glm::mat4 const&, ShaderProgram&);

void
set_mvpmatrix(stlw::Logger&, glm::mat4 const&, glm::mat4 const&, ShaderProgram&);

} // namespace opengl::render

#pragma once
#include <boomhs/components.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/tilegrid.hpp>
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <stlw/log.hpp>

#include <extlibs/glm.hpp>
#include <vector>

namespace opengl
{
class DrawInfo;
class ShaderProgram;
class ShaderPrograms;
struct TextureInfo;
} // namespace opengl

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{
class Camera;
class EntityRegistry;
class HandleManager;
class LevelManager;
struct EngineState;
struct RiverInfo;
struct Transform;
class TileGrid;
struct TiledataState;
class WorldObject;
struct ZoneState;
} // namespace boomhs

namespace stlw
{
class float_generator;
} // namespace stlw

namespace boomhs
{
struct Dimensions;

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
  GLint     source, dest;
};

} // namespace boomhs

#define PUSH_CW_STATE_UNTIL_END_OF_SCOPE()                                                         \
  auto const cw_state = render::read_cwstate();                                                    \
  ON_SCOPE_EXIT([&]() { render::set_cwstate(cw_state); });

#define PUSH_BLEND_STATE_UNTIL_END_OF_SCOPE()                                                      \
  auto const blend_state = render::read_blendstate();                                              \
  ON_SCOPE_EXIT([&]() { render::set_blendstate(blend_state); });

#define ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT()                                                   \
  glEnable(GL_BLEND);                                                                              \
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);                                               \
  ON_SCOPE_EXIT([]() { glDisable(GL_BLEND); });

#define ENABLE_ADDITIVE_BLENDING_UNTIL_SCOPE_EXIT()                                                \
  glEnable(GL_BLEND);                                                                              \
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);                                                               \
  ON_SCOPE_EXIT([]() { glDisable(GL_BLEND); });

namespace boomhs::render
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
clear_screen(opengl::Color const&);

// TODO: keep these extract rest to sub-renderers
void
draw_2d(RenderState&, GLenum, opengl::ShaderProgram&, opengl::DrawInfo&, bool);

void
draw_2d(RenderState&, GLenum, opengl::ShaderProgram&, opengl::TextureInfo&, opengl::DrawInfo&,
        bool);

void
draw_3dlightsource(RenderState&, GLenum, glm::mat4 const&, opengl::ShaderProgram&,
                   opengl::DrawInfo&, EntityID, EntityRegistry&);

void
draw_3dshape(RenderState&, GLenum, glm::mat4 const&, opengl::ShaderProgram&, opengl::DrawInfo&);

void
draw_3dblack_water(RenderState&, GLenum, glm::mat4 const&, opengl::ShaderProgram&,
                   opengl::DrawInfo&);

void
draw_3dlit_shape(RenderState&, GLenum, glm::vec3 const&, glm::mat4 const&, opengl::ShaderProgram&,
                 opengl::DrawInfo&, opengl::Material const&, EntityRegistry&, bool);

// TODO: move rest to sub-renderers or something
void
conditionally_draw_player_vectors(RenderState&, WorldObject const&);

void
draw(RenderState&, GLenum, opengl::ShaderProgram&, opengl::DrawInfo&);

void
draw_arrow(RenderState&, glm::vec3 const&, glm::vec3 const&, opengl::Color const&);

void
draw_arrow_abovetile_and_neighbors(RenderState&, TilePosition const&);

void
draw_fbo_testwindow(RenderState&, glm::vec2 const&, glm::vec2 const&, opengl::TextureInfo&);

void
draw_global_axis(RenderState&);

void
draw_inventory_overlay(RenderState&);

void
draw_local_axis(RenderState&, glm::vec3 const&);

void
draw_targetreticle(RenderState&, window::FrameTime const&);

void
draw_rivers(RenderState&, window::FrameTime const&);

void
draw_stars(RenderState&, window::FrameTime const&);

void
draw_tilegrid(RenderState&, TiledataState const&, window::FrameTime const&);

void
draw_tilegrid(RenderState&, TiledataState const&);

void
render_scene(RenderState&, LevelManager&, stlw::float_generator&, window::FrameTime const&,
             glm::vec4 const&);

void
set_modelmatrix(stlw::Logger&, glm::mat4 const&, opengl::ShaderProgram&);

void
set_mvpmatrix(stlw::Logger&, glm::mat4 const&, glm::mat4 const&, opengl::ShaderProgram&);

} // namespace boomhs::render

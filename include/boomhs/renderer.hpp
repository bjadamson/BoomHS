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

} // namespace boomhs

namespace boomhs::render
{

void
init(stlw::Logger&, boomhs::Dimensions const&);

void
clear_screen(opengl::Color const&);

// TODO: keep these extract rest to sub-renderers
void
draw_2d(RenderState&, GLenum, opengl::ShaderProgram&, opengl::DrawInfo&, bool);

void
draw_2d(RenderState&, GLenum, opengl::ShaderProgram&, opengl::TextureInfo&, opengl::DrawInfo&, bool);

void
draw_3dshape(RenderState&, GLenum, glm::mat4 const&, opengl::ShaderProgram&, opengl::DrawInfo&);

void
draw_3dlit_shape(RenderState&, GLenum, glm::vec3 const&, glm::mat4 const&, opengl::ShaderProgram&,
                 opengl::DrawInfo&, opengl::Material const&, EntityRegistry&, bool, bool);

// TODO: move rest to sub-renderers or something
void
conditionally_draw_player_vectors(RenderState&, WorldObject const&);

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
draw_entities(RenderState&, stlw::float_generator&, window::FrameTime const&);

void
draw_targetreticle(RenderState&, window::FrameTime const&);

void
draw_rivers(RenderState&, window::FrameTime const&);

void
draw_stars(RenderState&, window::FrameTime const&);

void
draw_terrain(RenderState&, EntityRegistry& registry, window::FrameTime const&, glm::vec4 const&);

void
draw_tilegrid(RenderState&, TiledataState const&, window::FrameTime const&);

void
draw_tilegrid(RenderState&, TiledataState const&);

void
render_scene(RenderState&, LevelManager&, stlw::float_generator&, window::FrameTime const&,
             glm::vec4 const&);

void
set_modelmatrix(stlw::Logger&, glm::mat4 const&, opengl::ShaderProgram&);

} // namespace boomhs::render

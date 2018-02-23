#pragma once
#include <boomhs/components.hpp>
#include <boomhs/tilegrid.hpp>
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <window/sdl.hpp>
#include <stlw/log.hpp>

#include <vector>

namespace opengl
{
class DrawInfo;
class ShaderProgram;
class ShaderPrograms;
} // ns opengl

namespace window
{
class FrameTime;
} // ns window

namespace boomhs
{
class EntityRegistry;
class Camera;
class HandleManager;
struct EngineState;
struct RiverInfo;
struct Transform;
class TileGrid;
struct TiledataState;
class WorldObject;
struct ZoneState;
} // ns boomhs

namespace boomhs
{

struct RenderState
{
  EngineState &es;
  ZoneState &zs;
};

} // ns boomhs

namespace boomhs::render
{

void
init(window::Dimensions const&);

void
clear_screen(opengl::Color const&);

void
conditionally_draw_player_vectors(RenderState &, WorldObject const &);

void
draw_arrow(RenderState &, glm::vec3 const&, glm::vec3 const&, opengl::Color const&);

void
draw_arrow_abovetile_and_neighbors(RenderState &, TilePosition const&);

void
draw_global_axis(RenderState &, EntityRegistry &);

void
draw_local_axis(RenderState &, EntityRegistry &, glm::vec3 const &);

void
draw_entities(RenderState &);

void
draw_targetreticle(RenderState &, window::FrameTime const&);

void
draw_rivers(RenderState &, window::FrameTime const&);

void
draw_stars(RenderState &, window::FrameTime const&);

void
draw_terrain(RenderState &);

void
draw_tilegrid(RenderState &, TiledataState const&, window::FrameTime const&);

void
draw_tilegrid(RenderState &, TiledataState const&);

} // ns boomhs::render

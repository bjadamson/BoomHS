#pragma once
#include <boomhs/components.hpp>
#include <boomhs/tiledata.hpp>
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <window/sdl.hpp>
#include <stlw/log.hpp>

#include <entt/entt.hpp>
#include <vector>

namespace opengl
{
class DrawInfo;
class ShaderProgram;
class ShaderPrograms;
} // ns opengl

namespace window
{
struct FrameTime;
} // ns window

namespace boomhs
{
class Camera;
class HandleManager;
struct EngineState;
struct RenderArgs;
struct RiverInfo;
struct Transform;
class TileData;
struct TiledataState;
class WorldObject;
struct ZoneState;
} // ns boomhs

namespace boomhs
{

struct RenderArgs
{
  Camera const& camera;
  WorldObject const& player;

  stlw::Logger &logger;
  opengl::GlobalLight const& global_light;

  bool const draw_normals;
};

} // ns boomhs

namespace boomhs::render
{

void
init(window::Dimensions const&);

void
clear_screen(opengl::Color const&);

void
conditionally_draw_player_vectors(RenderArgs const&, WorldObject const &, EngineState &,
    ZoneState &);

void
draw(RenderArgs const&, Transform const&, opengl::ShaderProgram &, opengl::DrawInfo const&,
    uint32_t const, entt::DefaultRegistry &);

void
draw_arrow(RenderArgs const&, ZoneState &, glm::vec3 const&, glm::vec3 const&, opengl::Color const&);

void
draw_arrow_abovetile_and_neighbors(RenderArgs const&, TilePosition const&, ZoneState &);

void
draw_global_axis(RenderArgs const&, entt::DefaultRegistry &, opengl::ShaderPrograms &);

void
draw_local_axis(RenderArgs const& rargs, entt::DefaultRegistry &, opengl::ShaderPrograms &,
    glm::vec3 const &);

void
draw_entities(RenderArgs const&, EngineState const&, ZoneState &);

void
draw_rivers(RenderArgs const&, ZoneState &, window::FrameTime const&);

void
draw_terrain(RenderArgs const&, ZoneState &);

void
draw_tiledata(RenderArgs const&, TiledataState const&, ZoneState &, window::FrameTime const&);

void
draw_tilegrid(RenderArgs const&, TiledataState const&, ZoneState &);

} // ns boomhs::render

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
struct RenderArgs;
struct RiverInfo;
struct Transform;
class TileData;
struct TiledataState;
class WorldObject;
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
enable_depth_tests();

void
disable_depth_tests();

void
init(window::Dimensions const&);

void
clear_screen(opengl::Color const&);

void
draw(RenderArgs const&, Transform const&, opengl::ShaderProgram &, opengl::DrawInfo const&,
    uint32_t const, entt::DefaultRegistry &);

void
draw_rivers(RenderArgs const&, opengl::ShaderProgram &, opengl::DrawInfo const&,
    entt::DefaultRegistry &, window::FrameTime const&, uint32_t, RiverInfo const&);

void
draw_tiledata(RenderArgs const&, HandleManager &, TileData const&, TiledataState const&,
    opengl::ShaderPrograms &, entt::DefaultRegistry &, window::FrameTime const&);

void
draw_tilegrid(RenderArgs const&, Transform const&, opengl::ShaderProgram &, opengl::DrawInfo const&);

} // ns boomhs::render

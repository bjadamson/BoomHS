#pragma once
#include <boomhs/components.hpp>
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
} // ns opengl

namespace window
{
struct FrameTime;
} // ns window

namespace boomhs
{
class Camera;
struct RenderArgs;
struct Transform;
class TileMap;
struct TilemapState;
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

struct DrawPlusArgs
{
  opengl::ShaderProgram &sp;
  opengl::DrawInfo const& dinfo;

  uint32_t const eid;
};

struct DrawHashtagArgs
{
  opengl::ShaderProgram &sp;
  opengl::DrawInfo const& dinfo;

  uint32_t const eid;
};

struct DrawStairsDownArgs
{
  opengl::ShaderProgram &sp;
  opengl::DrawInfo const& dinfo;

  uint32_t const eid;
};

struct DrawStairsUpArgs
{
  opengl::ShaderProgram &sp;
  opengl::DrawInfo const& dinfo;

  uint32_t const eid;
};

void
draw_rivers(RenderArgs const&, opengl::ShaderProgram &, opengl::DrawInfo const&,
    entt::DefaultRegistry &, window::FrameTime const&, uint32_t, uint32_t);

struct DrawTilemapArgs
{
  DrawPlusArgs plus;
  DrawHashtagArgs hashtag;
  DrawHashtagArgs river;
  DrawStairsDownArgs stairs_down;
  DrawStairsUpArgs stairs_up;
};

void
draw_tilemap(RenderArgs const&, DrawTilemapArgs &, TileMap const&, TilemapState const&,
    entt::DefaultRegistry &, window::FrameTime const&);

void
draw_tilegrid(RenderArgs const&, Transform const&, opengl::ShaderProgram &, opengl::DrawInfo const&);

} // ns boomhs::render

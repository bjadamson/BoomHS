#pragma once
#include <opengl/colors.hpp>
#include <window/sdl.hpp>

namespace opengl
{
class DrawInfo;
class ShaderProgram;
} // ns opengl

namespace boomhs
{
struct RenderArgs;
struct Transform;
class TileMap;
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
draw(RenderArgs const&, Transform const&, opengl::ShaderProgram &, opengl::DrawInfo const&);

struct DrawTilemapArgs
{
  opengl::DrawInfo const& hashtag_dinfo;
  opengl::ShaderProgram &hashtag_shader_program;

  opengl::DrawInfo const& plus_dinfo;
  opengl::ShaderProgram &plus_shader_program;
};

void
draw_tilemap(RenderArgs const&, Transform const&, DrawTilemapArgs &&, TileMap const&,
    bool const reveal_map);

void
draw_tilegrid(RenderArgs const&, Transform const&, opengl::ShaderProgram &, opengl::DrawInfo const&);

} // ns boomhs::render

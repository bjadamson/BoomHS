#pragma once
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <window/sdl.hpp>
#include <stlw/log.hpp>

namespace opengl
{
class DrawInfo;
class ShaderProgram;
} // ns opengl

namespace boomhs
{
class Camera;
struct RenderArgs;
struct LightColors;
struct Transform;
struct RenderableObject;
class TileMap;
} // ns boomhs

namespace boomhs
{

static constexpr auto INIT_ATTENUATION_INDEX = 8;

struct LightColors
{
  opengl::Color ambient = LOC::WHITE;
  opengl::Color diffuse = LOC::WHITE;
  opengl::Color specular = LOC::WHITE;

  opengl::Attenuation attenuation = opengl::ATTENUATION_VALUE_TABLE[INIT_ATTENUATION_INDEX];

  // TODO: this is a hack, should be based on other entities position(s)
  glm::vec3 single_light_position{0.0f, 0.0f, 0.0f};
};

struct RenderArgs
{
  Camera const& camera;
  RenderableObject const& player;

  stlw::Logger &logger;
  std::vector<Transform*> &entities;
  LightColors const& light;
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

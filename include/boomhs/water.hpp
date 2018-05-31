#pragma once
#include <boomhs/obj.hpp>
#include <boomhs/water_fbos.hpp>

#include <opengl/draw_info.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <extlibs/glm.hpp>
#include <stlw/log.hpp>

namespace window
{
class FrameTime;
} // namespace window

namespace stlw
{
class float_generator;
} // namespace stlw

namespace boomhs
{
class Camera;
struct EngineState;
class LevelManager;
class DrawState;
class RenderState;
class SkyboxRenderer;

struct WaterTileThing
{
};

struct WaterInfo
{
  glm::vec2              position;
  opengl::DrawInfo       dinfo;
  opengl::ShaderProgram& shader;
  opengl::TextureInfo*   tinfo;

  float wave_offset = 0.0f;

  //
  // constructors
  NO_COPY(WaterInfo);
  MOVE_DEFAULT(WaterInfo);
  WaterInfo(glm::vec2 const&, opengl::DrawInfo&&, opengl::ShaderProgram&, opengl::TextureInfo&);
};

struct WaterInfoConfig
{
  glm::vec2 const& position;
  glm::vec2 const& dimensions;
  size_t const     num_vertexes;
};

struct WaterFactory
{
  static ObjData generate_water_data(stlw::Logger&, glm::vec2 const&, size_t);

  static WaterInfo generate_info(stlw::Logger&, WaterInfoConfig const&, opengl::ShaderProgram&,
                                 opengl::TextureInfo&);

  static WaterInfo make_default(stlw::Logger&, opengl::ShaderPrograms&, opengl::TextureTable&);
};

class WaterRenderer
{
  WaterFrameBuffers fbos_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(WaterRenderer);

  explicit WaterRenderer(WaterFrameBuffers&&);
  void render_refraction(EngineState&, DrawState&, LevelManager&, Camera&, SkyboxRenderer&,
                         stlw::float_generator&, window::FrameTime const&);
  void render_reflection(EngineState&, DrawState&, LevelManager&, Camera&, SkyboxRenderer&,
                         stlw::float_generator&, window::FrameTime const&);
  void render_water(RenderState&, DrawState&, LevelManager&, Camera&, window::FrameTime const&);
};

} // namespace boomhs

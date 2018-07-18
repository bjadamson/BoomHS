#pragma once
#include <boomhs/dimensions.hpp>
#include <boomhs/obj.hpp>

#include <opengl/bind.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/framebuffer.hpp>
#include <opengl/renderbuffer.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glm.hpp>

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
  glm::vec2            position;
  opengl::DrawInfo*    dinfo = nullptr;
  opengl::TextureInfo* tinfo = nullptr;

  float wave_offset = 0.0f;

  //
  // constructors
  NO_COPY(WaterInfo);
  MOVE_DEFAULT(WaterInfo);
  WaterInfo() = default;
};

struct WaterFactory
{
  static ObjData generate_water_data(stlw::Logger&, glm::vec2 const&, size_t);

  static WaterInfo generate_info(stlw::Logger&, opengl::TextureInfo&);

  static WaterInfo make_default(stlw::Logger&, opengl::ShaderPrograms&, opengl::TextureTable&);
};

class BasicWaterRenderer
{
  stlw::Logger&          logger_;
  opengl::ShaderProgram& sp_;
  opengl::TextureInfo&   diffuse_;
  opengl::TextureInfo&   normal_;

public:
  BasicWaterRenderer(stlw::Logger&, opengl::TextureInfo&, opengl::TextureInfo&,
                     opengl::ShaderProgram&);
  MOVE_CONSTRUCTIBLE_ONLY(BasicWaterRenderer);

  void render_water(RenderState&, DrawState&, LevelManager&, Camera&, window::FrameTime const&);
};

struct ReflectionBuffers
{
  opengl::FrameBuffer  fbo;
  opengl::TextureInfo  tbo;
  opengl::RenderBuffer rbo;

  ReflectionBuffers(stlw::Logger&, ScreenSize const&);

  NO_COPY(ReflectionBuffers);
  MOVE_DEFAULT(ReflectionBuffers);
};

struct RefractionBuffers
{
  opengl::FrameBuffer fbo;
  opengl::TextureInfo tbo;
  opengl::TextureInfo dbo;

  RefractionBuffers(stlw::Logger&, ScreenSize const&);

  NO_COPY(RefractionBuffers);
  MOVE_DEFAULT(RefractionBuffers);
};

class AdvancedWaterRenderer
{
  opengl::ShaderProgram& sp_;
  opengl::TextureInfo&   diffuse_;
  opengl::TextureInfo&   dudv_;
  opengl::TextureInfo&   normal_;

  ReflectionBuffers reflection_;
  RefractionBuffers refraction_;

  template <typename FN>
  void with_reflection_fbo(stlw::Logger& logger, FN const& fn)
  {
    reflection_.fbo->while_bound(logger, fn);
  }

  template <typename FN>
  void with_refraction_fbo(stlw::Logger& logger, FN const& fn)
  {
    refraction_.fbo->while_bound(logger, fn);
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(AdvancedWaterRenderer);

  explicit AdvancedWaterRenderer(stlw::Logger&, ScreenSize const&, opengl::ShaderProgram&,
                                 opengl::TextureInfo&, opengl::TextureInfo&, opengl::TextureInfo&);
  void render_refraction(EngineState&, DrawState&, LevelManager&, Camera&, SkyboxRenderer&,
                         stlw::float_generator&, window::FrameTime const&);
  void render_reflection(EngineState&, DrawState&, LevelManager&, Camera&, SkyboxRenderer&,
                         stlw::float_generator&, window::FrameTime const&);
  void render_water(RenderState&, DrawState&, LevelManager&, Camera&, window::FrameTime const&);
};

} // namespace boomhs

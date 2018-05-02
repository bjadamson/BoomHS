#pragma once
#include <boomhs/obj.hpp>

#include <opengl/draw_info.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <extlibs/glm.hpp>
#include <stlw/log.hpp>

namespace boomhs
{

struct WaterTileThing
{
};

struct WaterInfo
{
  glm::vec2           position;
  opengl::DrawInfo    dinfo;
  opengl::TextureInfo tinfo;

  //
  // constructors
  NO_COPY(WaterInfo);
  MOVE_DEFAULT(WaterInfo);
  WaterInfo(glm::vec2 const&, opengl::DrawInfo&&, opengl::TextureInfo const&);
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

  static WaterInfo generate_info(stlw::Logger&, WaterInfoConfig const&,
                                 opengl::ShaderProgram const&, opengl::TextureInfo const&);

  static WaterInfo
  make_default(stlw::Logger&, opengl::ShaderPrograms const&, opengl::TextureTable const&);
};

} // namespace boomhs

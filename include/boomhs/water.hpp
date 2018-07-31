#pragma once
#include <boomhs/dimensions.hpp>
#include <boomhs/obj.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glm.hpp>

namespace opengl
{
class DrawInfo;
class TextureInfo;
class TextureTable;
class ShaderPrograms;
} // namespace opengl

namespace boomhs
{
class EntityRegistry;

struct WaterInfo
{
  glm::vec2            position;
  opengl::DrawInfo*    dinfo = nullptr;
  opengl::TextureInfo* tinfo = nullptr;

  opengl::Color mix_color     = LOC::SLATE_BLUE;
  float         mix_intensity = 0.25f;

  float wave_offset   = 0.0f;
  float wind_speed    = 50.0f;
  float wave_strength = 0.01f;

  //
  // constructors
  NO_COPY(WaterInfo);
  MOVE_DEFAULT(WaterInfo);
  WaterInfo() = default;
};

struct WaterFactory
{
  static ObjData generate_water_data(stlw::Logger&, glm::vec2 const&, size_t);

  static WaterInfo&
  make_default(stlw::Logger&, opengl::ShaderPrograms&, opengl::TextureTable&, EntityRegistry&);
};

} // namespace boomhs

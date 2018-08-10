#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/obj.hpp>
#include <boomhs/screen_size.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <array>
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

opengl::ShaderProgram&
graphics_mode_to_water_shader(boomhs::GameGraphicsMode, opengl::ShaderPrograms&);

struct WaterInfo
{
  glm::vec2            position;
  EntityID             eid;
  opengl::TextureInfo* tinfo = nullptr;

  glm::vec2    dimensions;
  unsigned int num_vertexes;

  opengl::Color mix_color     = LOC::SLATE_BLUE;
  float         mix_intensity = 0.25f;

  float     wave_offset    = 0.0f;
  float     wave_strength  = 0.01f;
  glm::vec2 flow_direction = glm::normalize(glm::vec2{1.0f, 1.0f});

  //
  // constructors
  NO_COPY(WaterInfo);
  MOVE_DEFAULT(WaterInfo);
  WaterInfo() = default;
};

struct WaterFactory
{
  static ObjData generate_water_data(stlw::Logger&, glm::vec2 const&, size_t);

  static WaterInfo& make_default(stlw::Logger&, opengl::ShaderPrograms&, opengl::TextureTable&,
                                 EntityID, EntityRegistry&);
};

} // namespace boomhs

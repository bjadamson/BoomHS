#include <boomhs/camera.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/mesh.hpp>
#include <boomhs/state.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/water.hpp>

#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

#include <window/timer.hpp>

#include <stlw/log.hpp>
#include <stlw/random.hpp>

#include <cassert>
#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace boomhs
{

ShaderProgram&
graphics_mode_to_water_shader(GameGraphicsMode const dwo, opengl::ShaderPrograms& sps)
{
  ShaderProgram* sp = nullptr;

  switch (dwo) {
  case GameGraphicsMode::Basic:
    sp = &sps.ref_sp("water_basic");
    break;
  case GameGraphicsMode::Medium:
    sp = &sps.ref_sp("water_medium");
    break;
  case GameGraphicsMode::Advanced:
    sp = &sps.ref_sp("water_advanced");
    break;
  default:
    std::abort();
  }

  assert(sp);
  return *sp;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// WaterFactory
ObjData
WaterFactory::generate_water_data(stlw::Logger& logger, glm::vec2 const& dimensions,
                                  size_t const num_vertexes)
{
  auto const count = num_vertexes * num_vertexes;

  ObjData data;
  data.num_vertexes = count;

  data.vertices = MeshFactory::generate_rectangle_mesh(logger, dimensions, num_vertexes);

  bool constexpr TILE = true;
  data.uvs            = MeshFactory::generate_uvs(logger, dimensions, num_vertexes, TILE);

  data.indices = MeshFactory::generate_indices(logger, num_vertexes);

  return data;
}

WaterInfo&
WaterFactory::make_default(stlw::Logger& logger, ShaderPrograms& sps, TextureTable& ttable,
    EntityID const eid, EntityRegistry& registry)
{
  LOG_TRACE("Generating water");

  auto texture_o = ttable.find("water-diffuse");
  assert(texture_o);
  auto& ti = *texture_o;

  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_WRAP_S, GL_REPEAT);
    ti.set_fieldi(GL_TEXTURE_WRAP_T, GL_REPEAT);
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  });
  LOG_TRACE("Finished generating water");

  auto& winfo = registry.assign<WaterInfo>(eid);
  winfo.tinfo = &ti;

  auto const min = glm::vec3{-0.5, -0.2, -0.5};
  auto const max = glm::vec3{0.5f, 0.2, 0.5};
  AABoundingBox::add_to_entity(eid, registry, min, max);

  return winfo;
}

} // namespace boomhs

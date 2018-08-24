#include <boomhs/camera.hpp>
#include <boomhs/bounding_object.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/mesh.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/water.hpp>

#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

#include <boomhs/clock.hpp>

#include <common/log.hpp>
#include <boomhs/random.hpp>

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
WaterFactory::generate_water_data(common::Logger& logger, glm::vec2 const& dimensions,
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
WaterFactory::make_default(common::Logger& logger, ShaderPrograms& sps, TextureTable& ttable,
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

  auto& wi = registry.assign<WaterInfo>(eid);
  wi.tinfo = &ti;
  wi.dimensions   = glm::vec2{20};
  wi.num_vertexes = 4;

  registry.assign<IsRenderable>(eid);
  auto& tr = registry.assign<Transform>(eid);

  // hack
  tr.translation.y = 0.05f;

  auto const xdim = wi.dimensions.x;
  auto const zdim = wi.dimensions.y;
  auto constexpr WATER_HEIGHT = 0.005f;

  auto const min = glm::vec3{0,    -WATER_HEIGHT, 0};
  auto const max = glm::vec3{xdim, WATER_HEIGHT,  zdim};
  AABoundingBox::add_to_entity(logger, sps, eid, registry, min, max);

  return wi;
}

} // namespace boomhs

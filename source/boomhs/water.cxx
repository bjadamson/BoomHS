#include <boomhs/camera.hpp>
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

WaterInfo
WaterFactory::generate_info(stlw::Logger& logger, TextureInfo& tinfo)
{
  WaterInfo wi{};
  wi.tinfo = &tinfo;

  return wi;
}

WaterInfo
WaterFactory::make_default(stlw::Logger& logger, ShaderPrograms& sps, TextureTable& ttable)
{
  LOG_TRACE("Generating water");

  auto texture_o = ttable.find("water-diffuse");
  assert(texture_o);
  auto& ti = *texture_o;

  auto wi = generate_info(logger, ti);

  auto& tinfo = wi.tinfo;
  tinfo->while_bound(logger, [&]() {
    tinfo->set_fieldi(GL_TEXTURE_WRAP_S, GL_REPEAT);
    tinfo->set_fieldi(GL_TEXTURE_WRAP_T, GL_REPEAT);
    tinfo->set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    tinfo->set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  });
  LOG_TRACE("Finished generating water");
  return wi;
}

} // namespace boomhs

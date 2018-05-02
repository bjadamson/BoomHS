#include <boomhs/mesh.hpp>
#include <boomhs/water.hpp>

#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

using namespace opengl;

namespace boomhs
{

WaterInfo::WaterInfo(glm::vec2 const& p, DrawInfo&& d, TextureInfo const& t)
    : position(p)
    , dinfo(MOVE(d))
    , tinfo(t)
{
}

ObjData
WaterFactory::generate_water_data(stlw::Logger& logger, glm::vec2 const& dimensions,
                                  size_t const num_vertexes)
{
  auto const count = num_vertexes * num_vertexes;

  ObjData data;
  data.num_vertexes = count;

  data.vertices = MeshFactory::generate_rectangle_mesh(logger, dimensions, num_vertexes);

  data.normals = MeshFactory::generate_flat_normals(dimensions);
  data.uvs     = MeshFactory::generate_uvs(dimensions, num_vertexes);

  data.indices = MeshFactory::generate_indices(num_vertexes);

  return data;
}

WaterInfo
WaterFactory::generate_info(stlw::Logger& logger, WaterInfoConfig const& wic,
                            ShaderProgram const& sp, TextureInfo const& ti)
{
  auto const data = generate_water_data(logger, wic.dimensions, wic.num_vertexes);
  LOG_TRACE_SPRINTF("Generated water piece: %s", data.to_string());

  BufferFlags const flags{true, true, false, true};
  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto              di     = gpu::copy_gpu(logger, GL_TRIANGLE_STRIP, sp, buffer, ti);

  return WaterInfo{wic.position, MOVE(di), ti};
}

WaterInfo
WaterFactory::make_default(stlw::Logger& logger, ShaderPrograms const& sps,
                           TextureTable const& ttable)
{
  LOG_TRACE("Generating water");
  glm::vec2 const       pos{0, 0};
  size_t const          num_vertexes = 128;
  glm::vec2 const       dimensions{static_cast<float>(num_vertexes)};
  WaterInfoConfig const wic{pos, dimensions, num_vertexes};

  auto texture_o = ttable.find("water");
  assert(texture_o);
  auto const& tinfo = *texture_o;

  auto& sp = sps.ref_sp("terrain");
  auto  wi = WaterFactory::generate_info(logger, wic, sp, tinfo);
  LOG_TRACE("Finished generating water");

  return wi;
}

} // namespace boomhs

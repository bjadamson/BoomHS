#include <boomhs/mesh.hpp>
#include <boomhs/water.hpp>

#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

using namespace opengl;

namespace boomhs
{

WaterInfo::WaterInfo(glm::vec2 const& p, DrawInfo&& d, ShaderProgram& s, TextureInfo const& t)
    : position(p)
    , dinfo(MOVE(d))
    , shader(s)
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
WaterFactory::generate_info(stlw::Logger& logger, WaterInfoConfig const& wic, ShaderProgram& sp,
                            TextureInfo const& tinfo)
{
  auto const data = generate_water_data(logger, wic.dimensions, wic.num_vertexes);
  LOG_TRACE_SPRINTF("Generated water piece: %s", data.to_string());

  BufferFlags const flags{true, true, false, true};
  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto              di     = gpu::copy_gpu(logger, GL_TRIANGLE_STRIP, sp, buffer, tinfo);

  return WaterInfo{wic.position, MOVE(di), sp, tinfo};
}

WaterInfo
WaterFactory::make_default(stlw::Logger& logger, ShaderPrograms& sps, TextureTable const& ttable)
{
  LOG_TRACE("Generating water");
  glm::vec2 const       pos{0, 0};
  size_t const          num_vertexes = 128;
  glm::vec2 const       dimensions{static_cast<float>(num_vertexes)};
  WaterInfoConfig const wic{pos, dimensions, num_vertexes};

  auto texture_o = ttable.find("water");
  assert(texture_o);
  auto& ti = *texture_o;

  // TODO: why does using the "water" shader... not behave the same as using the terrain shader?
  // auto& sp = sps.ref_sp("water");
  auto& sp = sps.ref_sp("terrain");

  auto wi = generate_info(logger, wic, sp, ti);

  auto& tinfo = wi.tinfo;
  tinfo.while_bound([&]() {
    tinfo.set_fieldi(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    tinfo.set_fieldi(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  });
  LOG_TRACE("Finished generating water");
  return wi;
}

} // namespace boomhs

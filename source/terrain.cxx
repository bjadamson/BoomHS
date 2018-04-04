#include <boomhs/obj.hpp>
#include <boomhs/obj_store.hpp>
#include <boomhs/terrain.hpp>

#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

#include <stlw/algorithm.hpp>
#include <stlw/log.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

// Algorithm modified from:
// https://www.youtube.com/watch?v=yNYwZMmgTJk&list=PLRIWtICgwaX0u7Rf9zkZhLoLuZVfUksDP&index=14
ObjData
generate_terrain_data()
{
  auto const VC      = Terrain::VERTEX_COUNT;
  int const  count   = VC * VC;
  int const  count4x = count * 4;
  int const  count3x = count * 3;
  int const  count2x = count * 2;

  ObjData data;
  data.num_vertices = count;

  auto& vertices = data.vertices;
  vertices.reserve(count4x);

  auto& normals = data.normals;
  normals.reserve(count3x);

  auto& uvs = data.uvs;
  uvs.reserve(count2x);

  // vertex
  FORI(i, VC)
  {
    FORI(j, VC)
    {
      float const x = (float)j / ((float)VC - 1) * Terrain::SIZE;
      float const y = 3;
      float const z = (float)i / ((float)VC - 1) * Terrain::SIZE;
      float const w = 1.0f;

      vertices.emplace_back(x);
      vertices.emplace_back(y);
      vertices.emplace_back(z);
      vertices.emplace_back(w);

      normals.emplace_back(0);
      normals.emplace_back(1);
      normals.emplace_back(0);

      float const u = (float)j / ((float)VC - 1);
      float const v = (float)i / ((float)VC - 1);

      uvs.emplace_back(u);
      uvs.emplace_back(v);
    }
  }

  int const num_indices = 6 * (VC - 1) * (VC - 1);
  auto&     indices     = data.indices;
  indices.reserve(num_indices);

  // indices
  int ip = 0;
  FORI(gz, VC - 1)
  {
    FORI(gx, VC - 1)
    {
      int const top_left     = (gz * VC) + gx;
      int const top_right    = top_left + 1;
      int const bottom_left  = ((gz + 1) * VC) + gx;
      int const bottom_right = bottom_left + 1;

      indices.emplace_back(top_left);
      indices.emplace_back(bottom_left);
      indices.emplace_back(top_right);

      indices.emplace_back(top_right);
      indices.emplace_back(bottom_left);
      indices.emplace_back(bottom_right);
    }
  }
  return data;
}

} // namespace

namespace boomhs
{

int const Terrain::SIZE         = 800;
int const Terrain::VERTEX_COUNT = 128;

Terrain::Terrain(glm::vec2 const& pos, DrawInfo&& di, TextureInfo const& ti)
    : pos_(pos)
    , di_(MOVE(di))
    , ti_(ti)
{
  pos_ = pos * Terrain::SIZE;
}

} // namespace boomhs

namespace boomhs::terrain
{

Terrain
generate(stlw::Logger& logger, glm::vec2 const& pos, ShaderProgram& sp, TextureInfo const& ti)
{
  QueryAttributes const qa{true, true, false, true};
  auto const            data   = generate_terrain_data();
  auto const            buffer = ObjStore::create_interleaved_buffer(logger, data, qa);

  auto di = gpu::copy_gpu(logger, GL_TRIANGLES, sp, buffer, ti);
  return Terrain{pos, MOVE(di), ti};
}

} // namespace boomhs::terrain

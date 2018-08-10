#pragma once
#include <boomhs/obj.hpp>

#include <common/log.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <string>
#include <vector>

namespace opengl
{
struct ImageData;
class TextureTable;
} // namespace opengl

namespace boomhs
{

class Heightmap
{
  using HeightmapData = std::vector<uint8_t>;
  int           width_;
  HeightmapData data_;

  COPY_DEFAULT(Heightmap);

public:
  explicit Heightmap(int);
  MOVE_DEFAULT(Heightmap);

  // methods
  Heightmap clone() const { return *this; }

  uint8_t&       data(size_t, size_t);
  uint8_t const& data(size_t, size_t) const;

  void   reserve(size_t);
  void   add(uint8_t);
  size_t size() const { return data_.size(); }

  std::string to_string() const;
};

using HeightmapResult = Result<Heightmap, std::string>;

} // namespace boomhs

namespace boomhs
{
struct TerrainConfig;
} // namespace boomhs

namespace boomhs::heightmap
{

HeightmapResult
load_fromtable(common::Logger&, opengl::TextureTable const&, std::string const&);

boomhs::ObjVertices
generate_normals(int, int, bool, Heightmap const&);

HeightmapResult
parse(opengl::ImageData const&);

HeightmapResult
parse(common::Logger&, char const*);

HeightmapResult
parse(common::Logger&, std::string const&);

void
update_vertices_from_heightmap(common::Logger&, boomhs::TerrainConfig const&, Heightmap const&,
                               boomhs::ObjVertices&);

} // namespace boomhs::heightmap

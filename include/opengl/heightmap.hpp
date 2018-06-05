#pragma once
#include <opengl/texture.hpp>

#include <boomhs/obj.hpp>

#include <stlw/log.hpp>
#include <stlw/result.hpp>

#include <string>
#include <vector>

namespace opengl
{
struct ImageData;

using HeightmapData   = std::vector<uint8_t>;
using HeightmapResult = Result<HeightmapData, std::string>;

} // namespace opengl

namespace boomhs
{
struct TerrainConfig;
} // namespace boomhs

namespace opengl::heightmap
{

HeightmapResult
load_fromtable(stlw::Logger&, TextureTable const&, std::string const&);

boomhs::ObjVertices
generate_normals(int, int, bool, HeightmapData const&);

HeightmapResult
parse(ImageData const&);

HeightmapResult
parse(stlw::Logger&, char const*);

HeightmapResult
parse(stlw::Logger&, std::string const&);

void
update_vertices_from_heightmap(stlw::Logger&, boomhs::TerrainConfig const&, HeightmapData const&,
                               boomhs::ObjVertices&);

} // namespace opengl::heightmap

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
struct TerrainPieceConfig;
} // namespace boomhs

namespace opengl::heightmap
{

HeightmapResult
load_fromtable(stlw::Logger&, TextureTable const&, std::string const&);

boomhs::ObjData::vertices_t
generate_normals(int, int, bool, HeightmapData const&);

HeightmapResult
parse(ImageData const&);

HeightmapResult
parse(stlw::Logger&, char const*);

HeightmapResult
parse(stlw::Logger&, std::string const&);

void
update_vertices_from_heightmap(stlw::Logger&, boomhs::TerrainPieceConfig const&,
                               HeightmapData const&, boomhs::ObjData::vertices_t&);

} // namespace opengl::heightmap

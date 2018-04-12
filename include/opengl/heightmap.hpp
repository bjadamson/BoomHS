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

namespace opengl::heightmap
{

boomhs::ObjData::vertices_t
generate_normals(int, int, HeightmapData const&);

HeightmapResult
parse(ImageData const&);

HeightmapResult
parse(stlw::Logger&, char const*);

HeightmapResult
parse(stlw::Logger&, std::string const&);

} // namespace opengl::heightmap

#pragma once
#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shapes.hpp>

#include <stlw/log.hpp>

#include <extlibs/glm.hpp>

namespace boomhs
{
class Obj;
class ObjData;
struct ShaderName;
} // namespace boomhs

namespace opengl
{
struct ArrowCreateParams;
struct ShaderPrograms;
class TextureInfo;
struct VertexBuffer;
class VertexAttribute;

} // namespace opengl

namespace opengl::gpu
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Arrows
DrawInfo
copy_arrow(stlw::Logger&, VertexAttribute const&, ArrowVertices const&);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Cubes
DrawInfo
copy_cube_gpu(stlw::Logger&, CubeVertices const&, VertexAttribute const&);

DrawInfo
copy_cube_wireframe_gpu(stlw::Logger&, CubeVertices const&, VertexAttribute const&);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Rectangles
DrawInfo
copy_rectangle(stlw::Logger&, VertexAttribute const&, RectBuffer const&);

DrawInfo
copy_rectangle_uvs(stlw::Logger&, VertexAttribute const&, RectangleVertices const&,
                   TextureInfo const&);

///////////////////////////////////////////////////////////////////////////////////////////////////
// General
DrawInfo
create_terrain_grid(stlw::Logger&, VertexAttribute const&, glm::vec2 const&, bool, Color const&);

DrawInfo
create_modelnormals(stlw::Logger&, VertexAttribute const&, glm::mat4 const&, boomhs::Obj const&,
                    Color const&);

DrawInfo
copy_gpu(stlw::Logger&, VertexAttribute const&, boomhs::ObjData const&);

DrawInfo
copy_gpu(stlw::Logger&, VertexAttribute const&, VertexBuffer const&);

void
overwrite_vertex_buffer(stlw::Logger&, VertexAttribute const&, DrawInfo&, boomhs::ObjData const&);

} // namespace opengl::gpu
namespace OG = opengl::gpu;

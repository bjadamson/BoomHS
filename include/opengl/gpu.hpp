#pragma once
#include <boomhs/colors.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shapes.hpp>

#include <common/log.hpp>

#include <extlibs/glm.hpp>

namespace boomhs
{
class Obj;
class ObjData;
struct ShaderName;
} // namespace boomhs

namespace opengl
{
struct ShaderPrograms;
class TextureInfo;
struct VertexBuffer;
class VertexAttribute;
struct GridVerticesIndices;

} // namespace opengl

namespace opengl::gpu
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Arrows
DrawInfo
copy_arrow(common::Logger&, VertexAttribute const&, ArrowVertices const&);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Lines
DrawInfo
copy_line(common::Logger&, VertexAttribute const&, LineVertices const&);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Cubes
DrawInfo
copy_cube_gpu(common::Logger&, CubeVertices const&, VertexAttribute const&);

DrawInfo
copy_cube_wireframe_gpu(common::Logger&, CubeVertices const&, VertexAttribute const&);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Rectangles
DrawInfo
copy_rectangle(common::Logger&, VertexAttribute const&, RectBuffer const&);

DrawInfo
copy_rectangle_uvs(common::Logger&, VertexAttribute const&, RectangleUvVertices const&);

///////////////////////////////////////////////////////////////////////////////////////////////////
// General
DrawInfo
copy_grid_gpu(common::Logger&, VertexAttribute const&, GridVerticesIndices const&);

DrawInfo
create_modelnormals(common::Logger&, VertexAttribute const&, glm::mat4 const&, boomhs::Obj const&,
                    boomhs::Color const&);

DrawInfo
copy_gpu(common::Logger&, VertexAttribute const&, boomhs::ObjData const&);

DrawInfo
copy_gpu(common::Logger&, VertexAttribute const&, VertexBuffer const&);

void
overwrite_vertex_buffer(common::Logger&, VertexAttribute const&, DrawInfo&, boomhs::ObjData const&);

} // namespace opengl::gpu
namespace OG = opengl::gpu;

#pragma once
#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shapes.hpp>

#include <stlw/log.hpp>

#include <extlibs/glm.hpp>

namespace opengl
{
struct ArrowCreateParams;
struct ShaderPrograms;
class TextureInfo;
struct VertexBuffer;
class VertexAttribute;

} // namespace opengl

namespace boomhs
{
class Obj;
class ObjData;
struct ShaderName;
} // namespace boomhs

namespace opengl::gpu
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Arrows
DrawInfo
create_arrow_2d(stlw::Logger&, VertexAttribute const&, ArrowCreateParams&&);

DrawInfo
create_arrow(stlw::Logger&, VertexAttribute const&, ArrowCreateParams&&);

struct WorldOriginArrows
{
  DrawInfo x_dinfo;
  DrawInfo y_dinfo;
  DrawInfo z_dinfo;
};

WorldOriginArrows
create_axis_arrows(stlw::Logger&, VertexAttribute const&);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Cubes
DrawInfo
copy_cubecolor_gpu(stlw::Logger&, CubeMinMax const&, VertexAttribute const&, Color const&);

inline DrawInfo
copy_cubecolor_gpu(stlw::Logger& logger, CubeMinMax const& cr, VertexAttribute const& sp,
                   glm::vec3 const& c)
{
  return copy_cubecolor_gpu(logger, cr, sp, Color{c.x, c.y, c.z, 1.0f});
}

DrawInfo
copy_cube_gpu(stlw::Logger&, CubeVertices const&, VertexAttribute const&);

DrawInfo
copy_cube_wireframe_gpu(stlw::Logger&, CubeVertices const&, VertexAttribute const&);

// DrawInfo
// copy_cubenormalcolor_gpu(stlw::Logger&, CubeMinMax const&, VertexAttribute const&, Color
// const&);

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

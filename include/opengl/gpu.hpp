#pragma once
#include <opengl/draw_info.hpp>
#include <opengl/factory.hpp>
#include <opengl/global.hpp>
#include <opengl/texture.hpp>
#include <opengl/vertex_attribute.hpp>

#include <boomhs/components.hpp>
#include <boomhs/obj.hpp>

#include <stlw/log.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>

#include <array>

namespace opengl
{
struct VertexBuffer;

struct CubeVertices
{
  glm::vec3 min, max;
};

} // namespace opengl

namespace opengl::gpu
{

// Arrows
///////////////////////////////////////////////////////////////////////////////////////////////////
DrawInfo
create_arrow_2d(stlw::Logger&, VertexAttribute const&, OF::ArrowCreateParams&&);

DrawInfo
create_arrow(stlw::Logger&, VertexAttribute const&, OF::ArrowCreateParams&&);

struct WorldOriginArrows
{
  DrawInfo x_dinfo;
  DrawInfo y_dinfo;
  DrawInfo z_dinfo;
};

WorldOriginArrows
create_axis_arrows(stlw::Logger&, VertexAttribute const&);

// Cubes
///////////////////////////////////////////////////////////////////////////////////////////////////
DrawInfo
copy_cubecolor_gpu(stlw::Logger&, CubeVertices const&, VertexAttribute const&, Color const&);

inline DrawInfo
copy_cubecolor_gpu(stlw::Logger& logger, CubeVertices const& cr, VertexAttribute const& sp,
                   glm::vec3 const& c)
{
  return copy_cubecolor_gpu(logger, cr, sp, Color{c.x, c.y, c.z, 1.0f});
}

DrawInfo
copy_cubevertexonly_gpu(stlw::Logger&, CubeVertices const&, VertexAttribute const&);

DrawInfo
copy_cube_wireframevertexonly_gpu(stlw::Logger&, CubeVertices const&, VertexAttribute const&);

DrawInfo
copy_cubenormalcolor_gpu(stlw::Logger&, CubeVertices const&, VertexAttribute const&, Color const&);

DrawInfo
copy_cubetexture_gpu(stlw::Logger&, CubeVertices const&, VertexAttribute const&);

// Rectangles
///////////////////////////////////////////////////////////////////////////////////////////////////
DrawInfo
copy_rectangle(stlw::Logger&, VertexAttribute const&, OF::RectBuffer const&);

DrawInfo
copy_rectangle_uvs(stlw::Logger&, VertexAttribute const&, OF::RectangleVertices const&,
                   TextureInfo const&);

// General
///////////////////////////////////////////////////////////////////////////////////////////////////
DrawInfo
create_tilegrid(stlw::Logger&, VertexAttribute const&, boomhs::TileGrid const&,
                bool const show_yaxis_lines, Color const& color = LOC::RED);

DrawInfo
create_modelnormals(stlw::Logger&, VertexAttribute const&, glm::mat4 const&, boomhs::Obj const&,
                    Color const&);

DrawInfo
copy_gpu(stlw::Logger&, VertexAttribute const&, boomhs::ObjData const&);

DrawInfo
copy_gpu(stlw::Logger&, VertexAttribute const&, VertexBuffer const&);

} // namespace opengl::gpu
namespace OG = opengl::gpu;

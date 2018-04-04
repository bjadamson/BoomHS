#pragma once
#include <opengl/draw_info.hpp>
#include <opengl/factory.hpp>
#include <opengl/global.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>

#include <boomhs/obj.hpp>

#include <extlibs/fmt.hpp>
#include <stlw/log.hpp>

#include <array>
#include <extlibs/glew.hpp>
#include <optional>

namespace boomhs
{
struct ObjBuffer;
} // namespace boomhs

namespace opengl::gpu
{

// Arrows
///////////////////////////////////////////////////////////////////////////////////////////////////
DrawInfo
create_arrow_2d(stlw::Logger&, ShaderProgram const&, OF::ArrowCreateParams&&);

DrawInfo
create_arrow(stlw::Logger&, ShaderProgram const&, OF::ArrowCreateParams&&);

struct WorldOriginArrows
{
  DrawInfo x_dinfo;
  DrawInfo y_dinfo;
  DrawInfo z_dinfo;
};

WorldOriginArrows
create_axis_arrows(stlw::Logger&, ShaderProgram&);

// Cubes
///////////////////////////////////////////////////////////////////////////////////////////////////
DrawInfo
copy_cubecolor_gpu(stlw::Logger&, ShaderProgram const&, Color const&);

inline DrawInfo
copy_cubecolor_gpu(stlw::Logger& logger, ShaderProgram const& sp, glm::vec3 const& c)
{
  return copy_cubecolor_gpu(logger, sp, Color{c.x, c.y, c.z, 1.0f});
}

DrawInfo
copy_cubevertexonly_gpu(stlw::Logger&, ShaderProgram const&);

DrawInfo
copy_cubenormalcolor_gpu(stlw::Logger&, ShaderProgram const&, Color const&);

DrawInfo
copy_cubetexture_gpu(stlw::Logger&, ShaderProgram const&, TextureInfo const&);

// Rectangles
///////////////////////////////////////////////////////////////////////////////////////////////////
DrawInfo
copy_rectangle(stlw::Logger&, GLenum, ShaderProgram&, OF::RectBuffer const&,
               std::optional<TextureInfo> const&);

DrawInfo
copy_rectangle_uvs(stlw::Logger&, OF::RectangleVertices const&, ShaderProgram const&,
                   TextureInfo const&);

// General
///////////////////////////////////////////////////////////////////////////////////////////////////
DrawInfo
create_tilegrid(stlw::Logger&, ShaderProgram const&, boomhs::TileGrid const&,
                bool const show_yaxis_lines, Color const& color = LOC::RED);

DrawInfo
create_modelnormals(stlw::Logger&, ShaderProgram const&, glm::mat4 const&, boomhs::Obj const&,
                    Color const&);

DrawInfo
copy_gpu(stlw::Logger&, GLenum, ShaderProgram&, boomhs::ObjData const&,
  std::optional<TextureInfo> const&);

DrawInfo
copy_gpu(stlw::Logger&, GLenum, ShaderProgram&, boomhs::ObjBuffer const&,
         std::optional<TextureInfo> const&);

} // namespace opengl::gpu
namespace OG = opengl::gpu;

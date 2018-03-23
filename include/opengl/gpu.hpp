#pragma once
#include <opengl/draw_info.hpp>
#include <opengl/factory.hpp>
#include <opengl/global.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>

#include <extlibs/fmt.hpp>
#include <stlw/log.hpp>

#include <extlibs/glew.hpp>
#include <optional>

namespace boomhs
{
struct ObjBuffer;
} // namespace boomhs

namespace opengl::gpu
{

struct ArrowCreateParams
{
  Color const& color;

  glm::vec3 start;
  glm::vec3 end;

  float const tip_length_factor = 4.0f;
};

struct ArrowEndpoints
{
  glm::vec3 p1;
  glm::vec3 p2;
};

DrawInfo
create_arrow_2d(stlw::Logger&, ShaderProgram const&, ArrowCreateParams&&);

DrawInfo
create_arrow(stlw::Logger&, ShaderProgram const&, ArrowCreateParams&&);

DrawInfo
create_tilegrid(stlw::Logger&, ShaderProgram const&, boomhs::TileGrid const&,
                bool const show_yaxis_lines, Color const& color = LOC::RED);

DrawInfo
create_modelnormals(stlw::Logger&, ShaderProgram const&, glm::mat4 const&, boomhs::Obj const&,
                    Color const&);

struct WorldOriginArrows
{
  DrawInfo x_dinfo;
  DrawInfo y_dinfo;
  DrawInfo z_dinfo;
};

WorldOriginArrows
create_axis_arrows(stlw::Logger&, ShaderProgram&);

DrawInfo
copy_colorcube_gpu(stlw::Logger&, ShaderProgram const&, Color const&);

inline DrawInfo
copy_colorcube_gpu(stlw::Logger& logger, ShaderProgram const& sp, glm::vec3 const& c)
{
  return copy_colorcube_gpu(logger, sp, Color{c.x, c.y, c.z, 1.0f});
}

DrawInfo
copy_vertexonlycube_gpu(stlw::Logger&, ShaderProgram const&);

DrawInfo
copy_normalcolorcube_gpu(stlw::Logger&, ShaderProgram const&, Color const&);

DrawInfo
copy_texturecube_gpu(stlw::Logger&, ShaderProgram const&, TextureInfo const&);

DrawInfo
copy_cube_14indices_gpu(stlw::Logger&, ShaderProgram const&, std::optional<TextureInfo> const&);

DrawInfo
copy_gpu(stlw::Logger&, GLenum, ShaderProgram&, boomhs::ObjBuffer const&,
         std::optional<TextureInfo> const&);

DrawInfo
copy_rectangle(stlw::Logger&, GLenum, ShaderProgram&, OF::RectBuffer const&,
               std::optional<TextureInfo> const&);

DrawInfo
copy_rectangle_uvs(stlw::Logger&, ShaderProgram const&, std::optional<TextureInfo> const&);

template <typename INDICES, typename VERTICES>
void
copy_synchronous(stlw::Logger& logger, ShaderProgram const& sp, DrawInfo const& dinfo,
                 VERTICES const& vertices, INDICES const& indices)
{
  // Activate VAO
  global::vao_bind(dinfo.vao());

  glBindBuffer(GL_ARRAY_BUFFER, dinfo.vbo());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinfo.ebo());

  auto const& va = sp.va();
  va.upload_vertex_format_to_glbound_vao(logger);

  // copy the vertices
  LOG_TRACE_SPRINTF("inserting '%i' vertices into GL_BUFFER_ARRAY\n", vertices.size());
  auto const  vertices_size = vertices.size() * sizeof(GLfloat);
  auto const& vertices_data = vertices.data();
  glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices_data, GL_STATIC_DRAW);

  // copy the vertice rendering order
  LOG_TRACE_SPRINTF("inserting '%i' indices into GL_ELEMENT_BUFFER_ARRAY\n", indices.size());
  auto const  indices_size = sizeof(GLuint) * indices.size();
  auto const& indices_data = indices.data();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_data, GL_STATIC_DRAW);
}

} // namespace opengl::gpu
namespace OG = opengl::gpu;

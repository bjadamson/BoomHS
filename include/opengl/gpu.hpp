#pragma once
#include <boomhs/colors.hpp>
#include <boomhs/vertex_factory.hpp>

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

namespace detail
{

template <typename VERTICES, typename INDICES>
void
copy_synchronous(common::Logger& logger, VertexAttribute const& va, DrawInfo &dinfo,
                 VERTICES const& vertices, INDICES const& indices)
{
  auto const bind_and_copy = [&]() {
    glBindBuffer(GL_ARRAY_BUFFER, dinfo.vbo());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinfo.ebo());

    va.upload_vertex_format_to_glbound_vao(logger);

    // copy the vertices
    LOG_DEBUG_SPRINTF("inserting '%i' vertices into GL_BUFFER_ARRAY\n", vertices.size());
    auto const  vertices_size = vertices.size() * sizeof(GLfloat);
    auto const& vertices_data = vertices.data();
    glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices_data, GL_STATIC_DRAW);

    // copy the vertice rendering order
    LOG_DEBUG_SPRINTF("inserting '%i' indices into GL_ELEMENT_BUFFER_ARRAY\n", indices.size());
    auto const  indices_size = sizeof(GLuint) * indices.size();
    auto const& indices_data = indices.data();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_data, GL_STATIC_DRAW);
  };

  LOG_TRACE("Starting synchronous cpu -> gpu copy");
  auto &vao = dinfo.vao();
  vao.while_bound(logger, bind_and_copy);
  LOG_TRACE("cpu -> gpu copy complete");
}

} // namespace detail

DrawInfo
copy(common::Logger &, VertexAttribute const&, boomhs::VertexFactory::ArrowVertices const&);

DrawInfo
copy(common::Logger &, VertexAttribute const&, boomhs::VertexFactory::LineVertices const&);

DrawInfo
copy(common::Logger&, VertexAttribute const&, boomhs::VertexFactory::GridVerticesIndices const&);

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
copy_rectangle(common::Logger &, VertexAttribute const&, RectLineBuffer const&);

DrawInfo
copy_rectangle_uvs(common::Logger&, VertexAttribute const&, RectangleUvVertices const&);

///////////////////////////////////////////////////////////////////////////////////////////////////
// General


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

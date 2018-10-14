#include <opengl/gpu.hpp>
#include <opengl/factory.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/global.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>
#include <opengl/vertex_attribute.hpp>

#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/obj.hpp>
#include <boomhs/obj_store.hpp>
#include <boomhs/terrain.hpp>


#include <common/algorithm.hpp>
#include <boomhs/math.hpp>
#include <common/type_macros.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>
#include <array>

using namespace boomhs;
using namespace opengl;
using namespace opengl::gpu;
using namespace glm;

namespace
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

template<typename V, typename I>
DrawInfo
copy_gpu_impl(common::Logger &logger, VertexAttribute const& va,
    V const& vertices, I const& indices)
{
  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{vertices.size(), num_indices};
  copy_synchronous(logger, va, dinfo, vertices, indices);
  return dinfo;
}

template <size_t N, size_t M>
DrawInfo
make_drawinfo(common::Logger &logger, VertexAttribute const& va,
    std::array<float, N> const& vertex_data, std::array<GLuint, M> const& indices)
{
  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{vertex_data.size(), num_indices};
  copy_synchronous(logger, va, dinfo, vertex_data, indices);
  return dinfo;
}

} // ns anon

namespace opengl::gpu
{

DrawInfo
copy(common::Logger &logger, VertexAttribute const& va,
    VertexFactory::ArrowVertices const& vertices)
{
  return make_drawinfo(logger, va, vertices, VertexFactory::ARROW_INDICES);
}

DrawInfo
copy_line(common::Logger &logger, VertexAttribute const& va,
    LineVertices const& vertices)
{
  auto const& INDICES = LineFactory::INDICES;
  DrawInfo dinfo{vertices.size(), INDICES.size()};
  copy_synchronous(logger, va, dinfo, vertices, INDICES);
  return dinfo;
}

DrawInfo
copy_grid_gpu(common::Logger &logger, VertexAttribute const& va,
              GridVerticesIndices const& grid)
{
  auto const& indices  = grid.indices;
  auto const& vertices = grid.vertices;

  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{vertices.size(), num_indices};
  copy_synchronous(logger, va, dinfo, vertices, indices);
  return dinfo;
}

DrawInfo
copy_cube_gpu(common::Logger &logger, CubeVertices const& cv, VertexAttribute const& va)
{
  return make_drawinfo(logger, va, cv, OF::CUBE_INDICES);
}

DrawInfo
copy_cube_wireframe_gpu(common::Logger& logger, CubeVertices const& cv,
    VertexAttribute const& va)
{
  return make_drawinfo(logger, va, cv, OF::CUBE_WIREFRAME_INDICES);
}

DrawInfo
copy_gpu(common::Logger &logger, VertexAttribute const& va,
    ObjData const& data)
{
  auto const qa    = BufferFlags::from_va(va);
  auto interleaved = VertexBuffer::create_interleaved(logger, data, qa);

  return copy_gpu_impl(logger, va, interleaved.vertices, interleaved.indices);
}

DrawInfo
copy_gpu(common::Logger &logger, VertexAttribute const& va, VertexBuffer const& object)
{
  auto const& v = object.vertices;
  auto const& i = object.indices;
  return copy_gpu_impl(logger, va, v, i);
}

DrawInfo
copy_rectangle(common::Logger &logger, VertexAttribute const& va,
    RectBuffer const& buffer)
{
  auto const& v = buffer.vertices;
  auto const& i = buffer.indices;
  return copy_gpu_impl(logger, va, v, i);
}

DrawInfo
copy_rectangle(common::Logger &logger, VertexAttribute const& va,
    RectLineBuffer const& buffer)
{
  auto const& v = buffer.vertices;
  auto const& i = buffer.indices;
  return copy_gpu_impl(logger, va, v, i);
}

DrawInfo
copy_rectangle_uvs(common::Logger &logger, VertexAttribute const& va,
                   RectangleUvVertices const& vertices)
{
  auto const& i = OF::RECTANGLE_INDICES;

  DrawInfo dinfo{vertices.size(), i.size()};
  copy_synchronous(logger, va, dinfo, vertices, i);
  return dinfo;
}

void
overwrite_vertex_buffer(common::Logger& logger, VertexAttribute const& va, DrawInfo &dinfo,
                 ObjData const& objdata)
{
  auto const upload = [&]() {
    glBindBuffer(GL_ARRAY_BUFFER, dinfo.vbo());
    va.upload_vertex_format_to_glbound_vao(logger);

    auto const qa    = BufferFlags::from_va(va);
    auto interleaved = VertexBuffer::create_interleaved(logger, objdata, qa);

    auto const& vertices      = interleaved.vertices;
    auto const  vertices_size = vertices.size() * sizeof(GLfloat);
    auto const& vertices_data = vertices.data();
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_size, vertices_data);
  };

  auto &vao = dinfo.vao();
  vao.while_bound(logger, upload);
}

} // ns opengl::gpu

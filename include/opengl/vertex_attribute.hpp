#pragma once
#include <opengl/glew.hpp>
#include <stlw/log.hpp>

namespace opengl
{

struct AttributePointerInfo
{
  static constexpr auto INVALID_TYPE = -1;

  GLuint index = 0;
  GLint type = INVALID_TYPE;
  GLsizei component_count = 0;
};

class VertexAttribute
{
public:
  static constexpr GLsizei API_BUFFER_SIZE = 4;

private:
  std::size_t const num_apis;
  GLsizei const stride;
  std::array<AttributePointerInfo, API_BUFFER_SIZE> apis;

public:
  MOVE_CONSTRUCTIBLE_ONLY(VertexAttribute);
  explicit VertexAttribute(std::size_t const n_apis, GLsizei const stride_p,
      std::array<AttributePointerInfo, API_BUFFER_SIZE> &&array)
    : num_apis(n_apis)
    , stride(stride_p)
    , apis(MOVE(array))
  {
  }
  void upload_vertex_format_to_glbound_vao(stlw::Logger &) const;
};

inline
auto make_vertex_attribute(std::initializer_list<AttributePointerInfo> list)
{
  // Requested to many APIs. Increase maximum number (more memory per instance required)
  assert(list.size() <= VertexAttribute::API_BUFFER_SIZE);

  std::array<AttributePointerInfo, VertexAttribute::API_BUFFER_SIZE> infos;
  std::copy(list.begin(), list.end(), infos.begin());

  GLsizei stride = 0;
  for(auto const& it : list) {
    stride += it.component_count;
  }

  return VertexAttribute{list.size(), stride, MOVE(infos)};
}

namespace va // vertex_attribute
{

VertexAttribute
vertex_color(stlw::Logger &);

VertexAttribute
vertex_normal_color(stlw::Logger &);

VertexAttribute
vertex_uv2d(stlw::Logger &);

VertexAttribute
vertex_normal_uv3d(stlw::Logger &);

VertexAttribute
vertex_only(stlw::Logger &);

} // ns va
} // ns opengl

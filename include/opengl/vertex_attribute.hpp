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

  AttributePointerInfo() = default;

  COPY_DEFAULT(AttributePointerInfo);
  AttributePointerInfo(GLuint const i, GLint const t, GLsizei const cc)
    : index(i)
    , type(t)
    , component_count(cc)
  {
  }
  AttributePointerInfo(AttributePointerInfo &&other)
    : AttributePointerInfo(other.index, other.type, other.component_count)
  {
    other.index = 0;
    other.type = INVALID_TYPE;
    other.component_count = 0;
  }
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

template<typename ITB, typename ITE>
auto
make_vertex_attribute(ITB const begin, ITE const end)
{
  // Requested to many APIs. Increase maximum number (more memory per instance required)
  std::size_t const num_vas = std::distance(begin, end);
  assert(num_vas <= VertexAttribute::API_BUFFER_SIZE);

  std::array<AttributePointerInfo, VertexAttribute::API_BUFFER_SIZE> infos;
  std::copy(begin, end, infos.begin());

  GLsizei stride = 0;
  for(auto it = begin; it != end; std::advance(it, 1)) {
    stride += it->component_count;
  }

  return VertexAttribute{num_vas, stride, MOVE(infos)};
}

inline auto
make_vertex_attribute(std::initializer_list<AttributePointerInfo> apis)
{
  return make_vertex_attribute(apis.begin(), apis.end());
}

template<std::size_t N>
auto
make_vertex_attribute(std::array<AttributePointerInfo, N> const& apis)
{
  return make_vertex_attribute(apis.begin(), apis.end());
}

auto
make_vertex_attribute(std::vector<AttributePointerInfo> const& container)
{
  return make_vertex_attribute(container.begin(), container.end());
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

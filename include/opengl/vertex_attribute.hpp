#pragma once
#include <opengl/glew.hpp>
#include <stlw/log.hpp>
#include <string>

namespace opengl
{

struct AttributePointerInfo
{
  static constexpr auto INVALID_TYPE = -1;

  GLuint index = 0;
  GLint type = INVALID_TYPE;
  GLsizei component_count = 0;

  AttributePointerInfo() = default;
private:
  void static
  invalidate(AttributePointerInfo &api)
  {
    api.index = 0;
    api.type = INVALID_TYPE;
    api.component_count = 0;
  }
public:

  COPY_DEFAULT(AttributePointerInfo);
  AttributePointerInfo(GLuint const i, GLint const t, GLsizei const cc)
    : index(i)
    , type(t)
    , component_count(cc)
  {
  }
  AttributePointerInfo(AttributePointerInfo &&other) noexcept
    : AttributePointerInfo(other.index, other.type, other.component_count)
  {
    invalidate(other);
  }
  AttributePointerInfo&
  operator=(AttributePointerInfo &&other) noexcept
  {
    index = other.index;
    type = other.type;
    component_count = other.component_count;

    invalidate(other);
    return *this;
  }

  friend std::ostream& operator<<(std::ostream&, AttributePointerInfo const&);
};

std::ostream&
operator<<(std::ostream&, AttributePointerInfo const&);

class VertexAttribute
{
public:
  static constexpr GLsizei API_BUFFER_SIZE = 4;

private:
  std::size_t num_apis_;
  GLsizei stride_;
  std::array<AttributePointerInfo, API_BUFFER_SIZE> apis_;

public:
  MOVE_DEFAULT(VertexAttribute);
  COPY_DEFAULT(VertexAttribute);
  explicit VertexAttribute(std::size_t const n_apis, GLsizei const stride_p,
      std::array<AttributePointerInfo, API_BUFFER_SIZE> &&array)
    : num_apis_(n_apis)
    , stride_(stride_p)
    , apis_(MOVE(array))
  {
  }
  void upload_vertex_format_to_glbound_vao(stlw::Logger &) const;
  auto stride() const { return stride_; }

  friend std::ostream& operator<<(std::ostream&, VertexAttribute const&);
};

std::ostream&
operator<<(std::ostream&, VertexAttribute const&);

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

inline auto
make_vertex_attribute(std::vector<AttributePointerInfo> const& container)
{
  return make_vertex_attribute(container.begin(), container.end());
}

} // ns opengl

#pragma once
#include <common/log.hpp>
#include <extlibs/glew.hpp>
#include <string>

namespace opengl
{

enum class AttributeType
{
  POSITION = 0,
  NORMAL,
  COLOR,
  UV,

  // We hardcode the others, but this allows things to stay flexible.
  OTHER
};

AttributeType
attribute_type_from_string(char const*);

struct AttributePointerInfo
{
  static constexpr auto INVALID_TYPE = -1;

  // fields
  GLuint        index           = 0;
  GLint         datatype        = INVALID_TYPE;
  AttributeType typezilla       = AttributeType::OTHER;
  GLsizei       component_count = 0;

  // constructors
  AttributePointerInfo() = default;
  AttributePointerInfo(GLuint const, GLint const, AttributeType const, GLsizei const);

  COPYMOVE_DEFAULT(AttributePointerInfo);

  // methods
  std::string to_string() const;
};

std::ostream&
operator<<(std::ostream&, AttributePointerInfo const&);

class VertexAttribute
{
public:
  static constexpr GLsizei API_BUFFER_SIZE = 4;

private:
  size_t                                            num_apis_;
  GLsizei                                           stride_;
  std::array<AttributePointerInfo, API_BUFFER_SIZE> apis_;

  COPY_DEFAULT(VertexAttribute);
public:
  MOVE_DEFAULT(VertexAttribute);
  explicit VertexAttribute(size_t const, GLsizei const,
                           std::array<AttributePointerInfo, API_BUFFER_SIZE>&&);

  void upload_vertex_format_to_glbound_vao(common::Logger&) const;
  auto stride() const { return stride_; }

  bool has_vertices() const;
  bool has_normals() const;
  bool has_colors() const;
  bool has_uvs() const;

  auto clone() const { return *this; }

  std::string to_string() const;

  BEGIN_END_FORWARD_FNS(apis_);
};

std::ostream&
operator<<(std::ostream&, VertexAttribute const&);

template <typename ITB, typename ITE>
auto constexpr
make_vertex_attribute(ITB const begin, ITE const end)
{
  // Requested to many APIs. Increase maximum number (more memory per instance required)
  size_t const num_vas = std::distance(begin, end);
  assert(num_vas <= VertexAttribute::API_BUFFER_SIZE);

  std::array<AttributePointerInfo, VertexAttribute::API_BUFFER_SIZE> infos;
  std::copy(begin, end, infos.begin());

  GLsizei stride = 0;
  for (auto it = begin; it != end; std::advance(it, 1)) {
    stride += it->component_count;
  }

  return VertexAttribute{num_vas, stride, MOVE(infos)};
}

inline auto
make_vertex_attribute(std::initializer_list<AttributePointerInfo> apis)
{
  return make_vertex_attribute(apis.begin(), apis.end());
}

template <size_t N>
auto constexpr
make_vertex_attribute(std::array<AttributePointerInfo, N> const& apis)
{
  return make_vertex_attribute(apis.cbegin(), apis.cend());
}

inline auto
make_vertex_attribute(AttributePointerInfo const& api)
{
  std::array<AttributePointerInfo, 1> const apis{{api}};
  return make_vertex_attribute(apis.cbegin(), apis.cend());
}

inline auto
make_vertex_attribute(std::vector<AttributePointerInfo> const& container)
{
  return make_vertex_attribute(container.cbegin(), container.cend());
}

} // namespace opengl

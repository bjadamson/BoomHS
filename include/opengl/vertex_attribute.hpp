#pragma once
#include <extlibs/glew.hpp>
#include <stlw/log.hpp>
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

class AttributePointerInfo
{
  static void invalidate(AttributePointerInfo&);

public:
  static constexpr auto INVALID_TYPE = -1;

  // fields
  GLuint        index           = 0;
  GLint         datatype        = INVALID_TYPE;
  AttributeType typezilla       = AttributeType::OTHER;
  GLsizei       component_count = 0;

  // constructors
  AttributePointerInfo() = default;
  COPY_DEFAULT(AttributePointerInfo);
  AttributePointerInfo(GLuint const, GLint const, AttributeType const, GLsizei const);
  AttributePointerInfo(AttributePointerInfo&&) noexcept;

  // methods
  AttributePointerInfo& operator=(AttributePointerInfo&&) noexcept;

  friend std::ostream& operator<<(std::ostream&, AttributePointerInfo const&);
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

public:
  MOVE_DEFAULT(VertexAttribute);
  COPY_DEFAULT(VertexAttribute);
  explicit VertexAttribute(size_t const, GLsizei const,
                           std::array<AttributePointerInfo, API_BUFFER_SIZE>&&);

  void upload_vertex_format_to_glbound_vao(stlw::Logger&) const;
  auto stride() const { return stride_; }

  bool has_vertices() const;
  bool has_normals() const;
  bool has_colors() const;
  bool has_uvs() const;

  BEGIN_END_FORWARD_FNS(apis_);
  friend std::ostream& operator<<(std::ostream&, VertexAttribute const&);
};

std::ostream&
operator<<(std::ostream&, VertexAttribute const&);

template <typename ITB, typename ITE>
auto
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

} // namespace opengl

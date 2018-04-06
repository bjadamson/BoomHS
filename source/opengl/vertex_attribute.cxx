#include <opengl/vertex_attribute.hpp>
#include <extlibs/fmt.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

namespace
{

using namespace opengl;

class UploadFormatState
{
  static constexpr auto INVALID_TYPE = 0;

  GLsizei const stride_;
  GLsizei components_skipped_ = 0;

public:
  GLint component_type_ = INVALID_TYPE;

  explicit UploadFormatState(GLsizei const stride)
      : stride_(stride)
  {
  }

  auto stride_size() const
  {
    assert(INVALID_TYPE != this->component_type_);
    return this->stride_ * sizeof(this->component_type_);
  }

  auto offset_size() const
  {
    assert(INVALID_TYPE != this->component_type_);
    return this->components_skipped_ * sizeof(this->component_type_);
  }

  void increase_offset(GLsizei const amount) { this->components_skipped_ += amount; }
  void set_type(GLint const type) { this->component_type_ = type; }
};

void
ensure_backend_has_enough_vertex_attributes(stlw::Logger &logger, GLint const num_apis)
{
  auto max_attribs = 0;
  glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);

  LOG_DEBUG_FMT("Max number of vertex attributes, found {}", max_attribs);

  if (max_attribs <= num_apis) {
    LOG_ERROR_SPRINTF("Error requested '%d' vertex attributes from opengl, only '%d' available",
        num_apis, max_attribs);
    std::abort();
  }
}

void
configure_and_enable_attrib_pointer(stlw::Logger &logger, AttributePointerInfo const& info,
    UploadFormatState &ufs)
{
  ensure_backend_has_enough_vertex_attributes(logger, info.component_count);

  // enable vertex attibute arrays
  glEnableVertexAttribArray(info.index);

  static auto constexpr DONT_NORMALIZE_THE_DATA = GL_FALSE;

  // clang-format off
  ufs.set_type(info.datatype);
  auto const stride_size_in_bytes = ufs.stride_size();
  auto const offset_size_in_bytes = ufs.offset_size();
  auto const offset_ptr = reinterpret_cast<GLvoid*>(offset_size_in_bytes);

  glVertexAttribPointer(
      info.index,              // global index id
      info.component_count,    // number of components per attribute
      info.datatype,           // data-type of the components
      DONT_NORMALIZE_THE_DATA, // don't normalize our data
      stride_size_in_bytes,    // byte-offset between consecutive vertex attributes
      offset_ptr);             // offset from beginning of buffer
  // clang-format on
  ufs.increase_offset(info.component_count);

  auto const make_decimals = [](auto const a0, auto const a1, auto const a2, auto const a3, auto const a4) {
    return fmt::sprintf("%-15d %-15d %-15d %-15d %-15d", a0, a1, a2, a3, a4);
  };

  auto const make_strings = [](auto const a0, auto const a1, auto const a2, auto const a3, auto const a4) {
    return fmt::sprintf("%-15s %-15s %-15s %-15s %-15s", a0, a1, a2, a3, a4);
  };

  auto const s = make_decimals(info.index, info.component_count, DONT_NORMALIZE_THE_DATA,
      stride_size_in_bytes, offset_size_in_bytes);
  auto const z = make_strings("attribute_index", "component_count", "normalize_data", "stride", "offset");

  LOG_DEBUG(z);
  LOG_DEBUG(s);
}

bool
va_has_attribute_type(VertexAttribute const& va, AttributeType const at)
{
  auto const cmp = [&at](auto const& api) {
    return api.typezilla == at;
  };
  auto const it = std::find_if(va.cbegin(), va.cend(), cmp);
  return it != va.cend();
}

} // ns anonymous

namespace opengl
{

AttributeType
attribute_type_from_string(char const* str)
{
//
// TODO: derive second argument from first somehow?
#define ATTR_STRING_TO_TYPE(STRING, ATTR_TYPE)                                                     \
  if (::strcmp(str, STRING) == 0) {                                                                \
      return AttributeType::ATTR_TYPE;                                                             \
  }

  ATTR_STRING_TO_TYPE("position", POSITION);
  ATTR_STRING_TO_TYPE("normal", NORMAL);
  ATTR_STRING_TO_TYPE("color", COLOR);
  ATTR_STRING_TO_TYPE("uv", UV);

  ATTR_STRING_TO_TYPE("other", OTHER);
#undef ATTR_STRING_TO_TYPE

  // terminal error
  std::abort();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AttributePointerInfo
AttributePointerInfo::AttributePointerInfo(GLuint const i, GLint const t, AttributeType const at,
    GLsizei const cc)
  : index(i)
  , datatype(t)
  , typezilla(at)
  , component_count(cc)
{
}

AttributePointerInfo::AttributePointerInfo(AttributePointerInfo &&other) noexcept
  : AttributePointerInfo(other.index, other.datatype, other.typezilla, other.component_count)
{
  invalidate(other);
}

AttributePointerInfo&
AttributePointerInfo::operator=(AttributePointerInfo &&other) noexcept
{
  index = other.index;
  datatype = other.datatype;
  typezilla = other.typezilla;
  component_count = other.component_count;

  invalidate(other);
  return *this;
}

void
AttributePointerInfo::invalidate(AttributePointerInfo &api)
{
  api.index = 0;
  api.datatype = INVALID_TYPE;
  api.typezilla = AttributeType::OTHER;
  api.component_count = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAttribute
VertexAttribute::VertexAttribute(size_t const n_apis, GLsizei const stride_p,
    std::array<AttributePointerInfo, API_BUFFER_SIZE> &&array)
  : num_apis_(n_apis)
  , stride_(stride_p)
  , apis_(MOVE(array))
{
}

void
VertexAttribute::upload_vertex_format_to_glbound_vao(stlw::Logger &logger) const
{
  UploadFormatState ufs{this->stride_};
  FOR(i, this->num_apis_) {
    auto const& api = this->apis_[i];
    assert(api.datatype != AttributePointerInfo::INVALID_TYPE);
    assert(api.typezilla != AttributeType::OTHER);
    assert(api.component_count > 0);

    configure_and_enable_attrib_pointer(logger, api, ufs);
  }
}

bool
VertexAttribute::has_vertices() const
{
  return va_has_attribute_type(*this, AttributeType::POSITION);
}

bool
VertexAttribute::has_normals() const
{
  return va_has_attribute_type(*this, AttributeType::NORMAL);
}

bool
VertexAttribute::has_colors() const
{
  return va_has_attribute_type(*this, AttributeType::COLOR);
}

bool
VertexAttribute::has_uvs() const
{
  return va_has_attribute_type(*this, AttributeType::UV);
}

std::string
AttributePointerInfo::to_string() const
{
  return fmt::format("(API) -- '{}' datatype: {}' component_count: '{}'",
      this->index,
      this->datatype,
      this->component_count);
}

std::ostream&
operator<<(std::ostream& stream, AttributePointerInfo const& api)
{
  stream << api.to_string();
  return stream;
}

std::string
VertexAttribute::to_string() const
{
  std::string result;
  result += fmt::format("(VA) -- num_apis: '{}' stride: '{}'\n",
      num_apis_,
      stride_);
  for(auto const& api : apis_) {
    result += "    ";
    result += api.to_string();
    result += "\n";
  }
  return result;
}

std::ostream&
operator<<(std::ostream& stream, VertexAttribute const& va)
{
  auto const print_delim = [&stream]() { stream << "-----------\n"; };
  print_delim();

  stream << va.to_string();
  print_delim();
  return stream;
}

} // ns opengl

#include <opengl/vertex_attribute.hpp>
#include <stlw/format.hpp>
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

  auto const fmt = fmt::sprintf("Max number of vertex attributes, found '%d'", max_attribs);
  LOG_TRACE(fmt);

  if (max_attribs <= num_apis) {
    auto const fmt =
        fmt::sprintf("Error requested '%d' vertex attributes from opengl, only '%d' available",
                      num_apis, max_attribs);
    LOG_ERROR(fmt);
    assert(false);
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
  ufs.set_type(info.type);
  auto const stride_size_in_bytes = ufs.stride_size();
  auto const offset_size_in_bytes = ufs.offset_size();
  auto const offset_ptr = reinterpret_cast<GLvoid*>(offset_size_in_bytes);

  glVertexAttribPointer(
      info.index,              // global index id
      info.component_count,    // number of components per attribute
      info.type,               // data-type of the components
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

} // ns anonymous

namespace opengl
{

void
VertexAttribute::upload_vertex_format_to_glbound_vao(stlw::Logger &logger) const
{
  UploadFormatState ufs{this->stride_};
  FOR(i, this->num_apis_) {
    auto const& api = this->apis_[i];
    assert(api.type != AttributePointerInfo::INVALID_TYPE);
    assert(api.component_count > 0);

    configure_and_enable_attrib_pointer(logger, api, ufs);
  }
}

std::ostream&
operator<<(std::ostream& stream, AttributePointerInfo const& api)
{
  stream << fmt::format("(API) -- '{}' type: {}' component_count: '{}'",
      api.index,
      api.type,
      api.component_count);
  return stream;
}

std::ostream&
operator<<(std::ostream& stream, VertexAttribute const& va)
{
  auto const print_delim = [&stream]() { stream << "-----------\n"; };
  print_delim();
  stream << fmt::format("(VA) -- num_apis: '{}' stride: '{}'\n",
      va.num_apis_,
      va.stride_);
  for(auto const& api : va.apis_) {
    stream << "    ";
    stream << api;
    stream << "\n";
  }
  print_delim();
  return stream;
}

} // ns opengl
